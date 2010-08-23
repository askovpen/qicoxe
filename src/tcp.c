/**********************************************************
 * ip routines
 **********************************************************/
/*
 * $Id: tcp.c,v 1.15 2007/01/28 17:55:00 mitry Exp $
 *
 * $Log: tcp.c,v $
 * Revision 1.15  2007/01/28 17:55:00  mitry
 * Add support for INA flag and IBN/IFC optional port number.
 *
 * Revision 1.14  2005/12/03 02:25:03  mitry
 * Fixed socks5 authentication
 *
 * Revision 1.13  2005/08/16 10:32:19  mitry
 * Fixed broken calls of loginscript
 *
 * Revision 1.12  2005/05/06 20:47:37  mitry
 * Misc code cleanup
 *
 * Revision 1.11  2005/04/14 15:10:26  mitry
 * Fixed uninitialized tty_fd
 *
 * Revision 1.10  2005/04/14 14:48:11  mitry
 * Misc changes
 *
 * Revision 1.9  2005/04/05 09:36:10  mitry
 * Changed arg 2 of shutdown()
 *
 * Revision 1.8  2005/03/28 16:48:23  mitry
 * Code cleaning
 *
 * Revision 1.7  2005/02/22 15:50:33  mitry
 * Commit non-blocking mode. Code cleaning.
 *
 * Revision 1.6  2005/02/21 21:59:43  mitry
 * Added preliminary support for non-blocking I/O
 *
 * Revision 1.5  2005/02/21 16:33:42  mitry
 * Changed tty_hangedup to tty_online
 *
 */

#include "headers.h"
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include "qipc.h"
#include "tty.h"
#include "crc.h"
#include "tcp.h"


#define H_BUF 4096

#define IFOFFLINE \
	if ( rc < 0 || tty_gothup ) { \
		write_log("connection lost %d,%d (%s)", rc, errno, strerror( errno )); \
		return (FALSE); \
	}



/* Returns hostname or ip address */
char *get_hostname(struct sockaddr_in *addr, char *host, int len)
{
	struct hostent		*he;
	struct sockaddr_in	sa;

	memcpy( &sa, addr, sizeof( struct sockaddr_in ) );
	if ( cfgi( CFG_RESOLVEHOSTNAME )) {
		he = gethostbyaddr( (char *) &sa.sin_addr, sizeof sa.sin_addr, AF_INET );
		if ( he ) {
			snprintf( host, len, "%s (%s)", he->h_name, inet_ntoa( sa.sin_addr ));
			return host;
		}
	}

	xstrcpy( host, inet_ntoa( sa.sin_addr ), len );

	return host;
}


int tcp_setsockopts(int fd)
{
	struct linger	lopt;
	int		rc = 0;

	rc = fd_set_nonblock( fd, true );

	/* Set SO_LINGER socket option */
	memset( &lopt, 0, sizeof( struct linger ));

	lopt.l_onoff  = 1;
	lopt.l_linger = 200; /* 2 seconds */

	if ( setsockopt( fd, SOL_SOCKET, SO_LINGER,
	    (char*) &lopt, sizeof( struct linger )) == -1 )
	{
		rc--;
		DEBUG(('I',1,"can't set SO_LINGER socket option on stdout: %s", strerror( errno )));
	}

	return ( rc == 0 );
}


/*
 * Open connection via proxy. Returns TRUE if OK, FALSE if fail.
 */
static int
tcp_connect_proxy(char *name)
{
	int	rc, i;
	time_t	t1;
	char	buf[H_BUF];
	char	*n, *p = NULL, *dp = "";

	if ( cfgs( CFG_PROXY ) && ( p = strchr( ccs, ' ' )))
		*p++ = 0;

	if ( !strchr( name, ':' ))
		dp = bink ? ":24554" : ":60179";

	i = snprintf( buf, H_BUF, "CONNECT %s%s HTTP/1.0\r\n", name, dp );
	if ( p ) {
		if (( n = strchr( p,' ' )))
			*n = ':';
		i += snprintf( buf + i, H_BUF - i, "Proxy-Authorization: basic " );
		i += base64( p, strlen( p ), buf + i );
		buf[i++] = '\r';
		buf[i++] = '\n';
	}

	buf[i++] = '\r';
	buf[i++] = '\n';
	PUTBLK( (unsigned char*) buf, i );

	t1 = timer_set( cfgi( CFG_HSTIMEOUT ));

	for( i = 0; i < H_BUF; i++ ) {
		while(( rc = GETCHAR(1)) == TIMEOUT && !timer_expired( t1 ))
			getevt();

		if ( rc == RCDO || tty_gothup ) {
			write_log( "got hangup" );
			return (FALSE);
		}

		if ( rc == TIMEOUT ) {
			write_log( "proxy timeout" );
			return (FALSE);
		} else if ( rc < 0 ) {
			write_log("connection is closed by proxy");
			return (FALSE);
		}

		buf[i] = rc;
		buf[i + 1] = '\0';

		if (( i >= H_BUF ) || (( p = strstr( buf, "\r\n\r\n" )))) {
			if (( p = strchr( buf, '\n' ))) {
				*p-- = '\0';
				if ( *p == '\r' )
					*p = '\0';
			}

			p = buf;
			if ( strstr( buf, " 200 " ))
				break;

			if ( !strncasecmp( buf, "HTTP", 4 ))
				p = strchr( buf, ' ' ) + 1;
			write_log( "connect rejected by proxy (%s)", p );
			return (FALSE);
		}
	}
	return (TRUE);
}


/*
 * Open connection via socks. Returns TRUE if OK, FALSE if fail.
 */
static int
tcp_connect_socks(char *name)
{
	int		rc, i, port;
	char		*n, *p = NULL, *auth = NULL;
	char		buf[MAX_STRING + 5];
	struct in_addr	da;
	struct hostent	*he;

	if ( cfgs( CFG_SOCKS ) && ( p = strchr( ccs, ' ' )))
		*p++ = 0;

	if ( !( n = strchr( name, ':' )))
		port = bink ? 24554 : 60179;
	else
		port = atoi( n + 1 );

	if ( p ) {
		auth = xstrdup( p );
		*buf = 5;
		buf[2] = '\0';
		n = strchr( auth, ' ' );
		if ( *auth == '*' ) {
			buf[1] = 1;
			PUTBLK( (unsigned char*) buf, 3 );
		} else {
			buf[1] = 2;
			buf[3]= n ? 2 : 1;
			PUTBLK( (unsigned char*) buf, 4 );
		}

		rc = GETCHAR( 60 );
		if ( rc > 0 ) {
			*buf = rc;
			rc = GETCHAR( 10 );
			buf[1] = rc;
		}

		IFOFFLINE

		if ( buf[1] && buf[1] != 1 && buf[1] != 2 ) {
			write_log("Auth. method is not supported by server");
			xfree( auth );
			return (FALSE);
		}

		DEBUG(('I',2,"socks5 auth=%d", buf[1]));
        
		if ( buf[1] == 2) {
			*buf = 1;
			if ( !n )
				i = strlen( auth );
			else
				i = n - auth;
            
			buf[1] = i;
			memcpy( buf + 2, auth, i );
			i += 2;
			if ( !n ) {
				buf[i++] = '\0';
			} else {
				n = skip_blanks( n );
				rc = strlen( n );
				buf[i++] = rc;
				memcpy( buf + i, n, rc );
				i += rc;
			}

			PUTBLK( (unsigned char*) buf, i );

			rc = GETCHAR( 40 );
			if( rc > 0) {
				*buf = rc;
				rc = GETCHAR( 10 );
				buf[1] = rc;
			}
            
			IFOFFLINE

			if ( buf[1] ) {
				write_log("Auth. failed (socks5 returns %02x%02x)",
					(unsigned char) buf[0],
					(unsigned char) buf[1] );
				xfree( auth );
				return (FALSE);
			}
		}
	}

	if ( !p ) {
		buf[0] = 4;
		buf[1] = 1;
		buf[2] = (unsigned char) (( port >> 8) & 0xff );
		buf[3] = (unsigned char) ( port & 0xff );

		if ( !( he = gethostbyname( name ))) {
			write_log("can't resolve addr: %s", strerror( errno ));
			xfree( auth );
			return (FALSE);
		}

		memcpy( buf + 4, he->h_addr, 4 );
		buf[8] = '\0';
		PUTBLK( (unsigned char*) buf, 9);
	} else {
		buf[0] = 5;
		buf[1] = 1;
		buf[2] = 0;
        
		if ( isdigit( *name ) && (da.s_addr = inet_addr( name )) != -1 ) {
			buf[3] = 1;
			memcpy( buf + 4, &da, 4 );
			n = buf + 8;
		} else {
			buf[3] = 3;
			i = strlen( name );
			buf[4] = (unsigned char) i;
			memcpy( buf + 5, name, i );
			n = buf + 5 + i;
		}

		*n++ = (unsigned char) (( port >> 8 ) & 0xff );
		*n++ = (unsigned char) ( port & 0xff );
		PUTBLK( (unsigned char*) buf, n - buf );
	}

	for( i = 0; i < MAX_STRING; i++ ) {
		getevt();
		rc = GETCHAR( 45 );
        
		IFOFFLINE
        
		buf[i] = rc;
		if ( !p && i > 6 ) {
			xfree( auth );
			if ( *buf ) {
				write_log( "bad reply from server" );
				return (FALSE);
			}

			if( buf[1] != 90 ) {
				write_log( "connection rejected by socks4 server (%d)",
					(unsigned char) buf[1] );
				return (FALSE);
			}
			return (TRUE);
		} else if ( p && i > 5) {
			if ( *buf != 5) {
				write_log( "bad reply from server" );
				xfree( auth );
				return (FALSE);
			}

			if (( buf[3] == 1 && i < 9 )
			    || ( buf[3] == 3 && i < ( 6 + (unsigned char) buf[4] ))
			    || ( buf[3] == 4 && i < 21 ))
				continue;

			if ( !buf[1] ) {
				xfree( auth );
				return (TRUE);
			}

			p = NULL;
			switch( buf[1] ) {
			case 1:
				p = "General SOCKS5 server failure";
				break;

			case 2:
				p = "Connection is not allowed by ruleset (socks5)";
				break;

			case 3:
				p = "Network is unreachable (socks5)";
				break;

			case 4:
				p = "Host is unreachable (socks5)";
				break;

			case 5:
				p = "Connection refused (socks5)";
				break;

			case 6:
				p = "TTL expired (socks5)";
				break;

			case 7:
				p = "Command is not supported by socks5 server";
				break;

			case 8:
				p = "Address type is not supported";
				break;

			default:
				write_log( "Unknown socks5 server reply (0x%02x)",
					(unsigned char) buf[1] );
			}

			if ( p )
				write_log( p );
			xfree( auth );
			return (FALSE);
		}
	}
	write_log( "answer is too long" );
	xfree( auth );
	return (FALSE);
}


#define GET_PORT() ( proxy ? ( sp ? 1080 : 3128 ) : ( bink ? 24554 : 60179 ))

/*
 * Open tcp/ip connection to host. Arguments are:
 *    name:  remote host name ( fqdn.address.dom[:port]/ip.ad.dr.es[:port] )
 *    proxy: proxy/socks server to connect via ( see name format )
 *    sp:    0 - connect via proxy server
 *           1 - connect via socks server
 * Returns:
 *    On success: descriptor of opened connection
 *    On failure: -1
 */
static int
tcp_connect(char *name, char *proxy, int sp)
{
	char			*portname, *p;
	int			misc;
	unsigned short		portnum;
	struct servent		*se;
	struct hostent		*he;
	struct sockaddr_in	server;

	memset( &server, 0, sizeof( server ));
	server.sin_family = AF_INET;

	if (( portname = strchr( proxy ? proxy : name, ':' ))) {
		*portname++ = '\0';
		if (( p = strchr( portname, ' ' )))
			*p = '\0';

		if (( portnum = atoi( portname ))) {
			server.sin_port = htons( portnum );
		} else {
			if (( se = getservbyname( portname, "tcp" )))
				server.sin_port = se->s_port;
			else
				server.sin_port = htons( GET_PORT() );
		}
	} else {
		if (( se = getservbyname( proxy ? ( sp ? "socks" : "proxy" ) :
		    ( bink ? "binkp" : "fido" ), "tcp" )))
			server.sin_port = se->s_port;
		else {
			if ( proxy && !sp && ( se = getservbyname( "squid", "tcp" )))
				server.sin_port = se->s_port;
			else
				server.sin_port = htons( GET_PORT() );
		}
	}

	if ( sscanf( proxy ? proxy : name, "%d.%d.%d.%d", &misc, &misc, &misc, &misc ) == 4 )
		server.sin_addr.s_addr = inet_addr( proxy ? proxy : name );
	else {
		if (( he = gethostbyname( proxy ? proxy : name )))
			memcpy( &server.sin_addr, he->h_addr, he->h_length );
		else {
			write_log("can't resolve ip for %s%s",
				proxy ? ( sp ? "socks " : "proxy " ) : "",
				proxy ? proxy : name );
			return -1;
		}
	}

	sline("Connecting to %s%s%s%s:%d",
		proxy ? name : "",
		proxy ? " via " : "",
		proxy ? ( sp ? "socks " : "proxy " ) : "",
		inet_ntoa( server.sin_addr ),
		(int) ntohs( server.sin_port ));

	signal( SIGTERM, tty_sighup );
	signal( SIGPIPE, tty_sighup );
	signal( SIGHUP, tty_sighup );

	if (( tty_fd = socket( AF_INET, SOCK_STREAM, 0)) < 0 ) {
		write_log("can't create socket: %s", strerror( errno ));
		return -1;
	}

	if ( fd_make_stddev( tty_fd ) != OK ) {
		write_log( "tcp_connect: can't make stdin/stdout/stderr" );
		close( tty_fd );
		return ERROR;
	}

	tty_fd = 0;

	if ( connect( tty_fd, (struct sockaddr*) &server, sizeof( server )) < 0 ) {
		write_log( "can't connect to %s: %s", inet_ntoa( server.sin_addr ), strerror( errno ));
		tcp_done();
		return ERROR;
	}

	if ( !tcp_setsockopts( tty_fd )) {
		write_log( "can't set socket options" );
		tcp_done();
		return ERROR;
	}

	write_log( "TCP/IP connection with %s%s:%d",
		proxy ? ( sp ? "socks " : "proxy " ) : "" ,
		inet_ntoa( server.sin_addr ),
		(int) ntohs( server.sin_port ));

	tty_online = TRUE;

	if ( proxy ) {
		sline("%s server found. waiting for reply...", sp ? "Socks" : "Proxy");
		if ( !sp ) {
			if ( !tcp_connect_proxy( name )) {
				tcp_done();
				return ERROR;
			}
		} else if ( !tcp_connect_socks( name )) {
			tcp_done();
			return ERROR;
		}
	}

	sline("FTN server found. waiting for reply...");
	
	return tty_fd;
}

int tcp_dial(ftnaddr_t *fa, char *host)
{
	int	fd, s = 0;
	char	*proxy = NULL, *t = NULL;

	if ( cfgs( CFG_PROXY ))
		proxy = xstrdup( ccs );
	else if( cfgs( CFG_SOCKS )) {
		proxy = xstrdup( ccs );
		s = 1;
	}

	if ( proxy && ( t = strchr( proxy, ' ' )))
		*t = '\0';

	write_log("connecting to %s at %s%s%s%s [%s]",
		ftnaddrtoa( fa ), host,
		proxy ? " via " : "",
		proxy ? ( s ? "socks " : "proxy " ) : "",
		SS( proxy ),
		bink ? "binkp" : "ifcico" );
	if ( t )
		*t = ' ';
    
	fd = tcp_connect( host, proxy, s );
	xfree( proxy );
	return fd;
}

void tcp_done(void)
{
#ifdef HAVE_SHUTDOWN
	shutdown( tty_fd, SHUT_RDWR );
#endif
	(void) close( tty_fd );
	tty_online = FALSE;
	signal( SIGTERM, SIG_DFL );
	signal( SIGPIPE, SIG_DFL );
	signal( SIGHUP, SIG_DFL );
}
