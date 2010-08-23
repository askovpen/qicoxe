/**********************************************************
 * qico control center.
 **********************************************************/
/*
 * $Id$
 *
 * $Log: qcc.c,v $
 * Revision 1.17  2006/07/18 22:30:09  mitry
 * Delayed cyrillization
 *
 * Revision 1.16  2005/08/12 16:40:51  mitry
 * Added wktime_str()
 *
 * Revision 1.15  2005/08/12 15:36:19  mitry
 * Changed gmtoff()
 *
 * Revision 1.14  2005/05/17 19:52:34  mitry
 * Added check for broken Sys5 signals
 *
 * Revision 1.13  2005/05/16 20:25:48  mitry
 * Fixed improper speed over 64K displaying
 *
 * Revision 1.12  2005/05/06 20:41:07  mitry
 * Changed setproctitle() code
 *
 * Revision 1.11  2005/04/05 09:25:29  mitry
 * Fix for tty device name > 8 symbols
 *
 * Revision 1.10  2005/04/01 20:33:33  mitry
 * envp is back to main()
 *
 * Revision 1.9  2005/03/31 19:40:38  mitry
 * Update function prototypes and it's duplication
 *
 * Revision 1.8  2005/03/28 17:02:52  mitry
 * Pre non-blocking i/o update. Mostly non working.
 *
 * Revision 1.7  2005/02/23 21:27:59  mitry
 * Changed '* Node' to '* Address'
 *
 */

#include <config.h>
#include <stdio.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef STDC_HEADERS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <signal.h>
#include <ctype.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#ifdef HAVE_CURSES_H
#include <curses.h>
#endif
#endif

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#ifndef GWINSZ_IN_SYS_IOCTL
#include <termios.h>
#endif

#ifdef MAXPATHLEN
#  define MAX_PATH MAXPATHLEN
#else
#  define MAX_PATH 1024
#endif


#include "replace.h"
#include "types.h"
#include "qcconst.h"
#include "qslib.h"
#include "md5q.h"
#include "clserv.h"

#define MAXMH		10		/* number of lines for queue.
					   (up window of screen) */
#define CHH		256		/* max line height */
#define MAX_SLOTS	16		/* max number of slots */
#define LGMAX		255		/* max scroll lines for log */
#define HSTMAX		50		/* max input line history */
#define LOGSIZE		(LINES-MH-4)	/* max shown loglines */
#define COL		(COLS-2)	/* max shown columns */
#define QDEBUG		0		/* set to 1 for debug */


#define xbeep() if(!beepdisable){beep();}

typedef struct {
	unsigned short id;
	char tty[QCC_TTYLEN];
	int  session;
	char *lb[LGMAX];
	char *header;
	char *status;
	char *sysop;
	char *flags;
	char *phone;
	char *addrs;
	char *name;
	char *city;
	char *cl;
	int  lc;
	int  lm;
	int  chat;
	int  chats;
	int  speed;
	int  chaty;
	int  chatx;
	int  options;
	int  opt;
	pfile_t s,r;
	time_t start;
	WINDOW *wlog;
} slot_t;

typedef struct _qslot_t {
	int n;
	char *addr;
	int mail,files,flags,try;
	struct _qslot_t *next,*prev;
} qslot_t;

static RETSIGTYPE sigwinch(int sig);
static int getmessages(char *bbx);

static char *hm[]={
	"F1",", ",
	"R","escan, ",
	"K","ill, ",
	"W","/","U","/","I","/","H",", ",
	"P","oll, "
	"In","f","o, "
	"Fr","e","q, ",
	"S","end, ",
	"0-9;Tab",", ",
	"Q","uit",
NULL
};

static char *hl[]={
 	"H","angup, ",
	"\03X","\03-skip file, ",
	"\03R","\03-suspend file, ",
	"\03C","\03hat, ",
	"F8","-close, ",
	"0-9;Tab",", ",
	"Q","uit",
NULL
};

static char *hc[]={
 	"ESC",";","F8","-close, ",
	"F2","-send string, ",
	"F4","-send bell, ",
	"Tab","-change window",
NULL
};

static char *hi[]={
	"\01Ins","-insert mode, ",
	"\02Ins","-overwrite mode, ",
	"Up","/","Dn",", ",
	"BS",", ",
	"Del","ete, ",
	"PgDn","-clear, ",
 	"Esc","ape, ",
	"Enter",
NULL
};

static char *help[]={
	"\01* qico control center %s, sisoft\\\\trg edition. help about keys:",
	"\01 All windows: F10 - quit, Tab/Right - next and Left - previous window",
	"\01 Main window:                  Line window:                Chat window:",
	"\01  F1 - help,  q - quit          q - quit,  p - poll         Esc - close chat",
	"\01  0..9 - select window          0..9 - select window        F4 - send `bell'",
	"\01  W/w/I/i/U/u/h - node status   h - hangup line             F2 - send string",
	"\01  R/r - rescan cfg / queue      X - skip file               F8 - close chat",
	"\01  Space - reset waiting cycle   R - suspend file",
	"\01  F/f - info about node         c - open chat window",
	"\01  K/k - kill files for node     Space - reset wait cycle",
	"\01  S/s - send file,  p - poll    F8 - close window",
	"\01  E/e - request file",
	"\01           !Warning: all keys are case sensitive!",
NULL
};

static slot_t *slots[MAX_SLOTS];
static qslot_t *queue;
static int currslot,allslots=-9,q_pos,q_first,q_max,crey=0,crex=0;
static int sizechanged=0,quitflag=0,ins=1,edm=0,beepdisable=0,MH=MAXMH;
static char *m_header=NULL,*m_status=NULL;
static WINDOW *wlog,*wmain,*whdr,*wstat,*whelp;
static int sock=-1,hstlast=0;
static char *hst[HSTMAX+1],*myaddr=NULL;
static char qflgs[Q_MAXBIT]=Q_CHARS;
static int qclrs[Q_MAXBIT]=Q_COLORS;


char progname[] = "qcc";

#ifdef DEBUG
    #undef DEBUG
#endif
#define DEBUG(p)


void write_log(char *str, ...)
{
    int y, x;
    struct tm *t;
    char s[MAX_STRING];
    va_list args;
    time_t tim;

    tim = time( NULL );
    t = localtime( &tim );
    getyx( wlog, y, x );
    wattron( wlog, A_BOLD );
    if ( x )
        waddch( wlog, '\n' );
    if ( *str != 1 ) {
	snprintf( s, MAX_STRING, "%02d %3s %02d:%02d:%02d: ",
            t->tm_mday, engms[t->tm_mon], t->tm_hour, t->tm_min, t->tm_sec );
    } else
        s[0] = '\0';
    
    va_start( args, str );
    vsnprintf( s + strlen( s ), MAX_STRING - strlen( s ), str, args );
    va_end( args );
    waddnstr( wlog, s + ( *s == 1 ), COL );
    wattroff( wlog, A_BOLD );
    if ( currslot < 0 )
        wrefresh( wlog );
}


static void usage(char *ex)
{
	printf("usage: %s [options]\n"
               "-P port      connect to <port> (default: qicoui or %u)\n"
	       "-a host      connect to <host> (default: localhost)\n"
	       "-w password  set <password> for connect\n"
	       "-n           disable sound (silent)\n"
	       "-h           this help screen\n"
               "-v           show version\n"
	       "\n",ex,DEF_SERV_PORT);
	exit(0);
}

#ifndef CURS_HAVE_MVVLINE
static void mvvline(int y,int x,int ch,int n)
{
	move(y,x);
	vline(ch,n);
}

static void mvhline(int y,int x,int ch,int n)
{
	move(y,x);
	hline(ch,n);
}

static void mvwhline(WINDOW *win,int y,int x,int ch,int n)
{
	wmove(win,y,x);
	wvline(win,ch,n);
}
#endif

static void draw_screen(void)
{
	redrawwin(stdscr);
	attrset(COLOR_PAIR(2));
	mvvline(1,0,ACS_VLINE,LINES-3);
	mvvline(1,COL+1,ACS_VLINE,LINES-3);
	mvaddch(MH+1,0,ACS_LTEE);
	mvaddch(MH+1,COL+1,ACS_RTEE);
	mvaddch(0,0,ACS_ULCORNER);
	mvaddch(0,1,ACS_HLINE);
	mvaddch(0,COL+1,ACS_URCORNER);
	mvaddch(0,COL,ACS_HLINE);
	mvaddch(LINES-2,1,'[');
	mvaddch(LINES-2,0,ACS_LLCORNER);
	mvaddch(LINES-2,COL,']');
	mvaddch(LINES-2,COL+1,ACS_LRCORNER);
	mvhline(MH+1,1,ACS_HLINE,COL);
	attron(COLOR_PAIR(8));
	mvhline(LINES-1,0,' ',COLS);
	refresh();
}

static void initscreen(void)
{
	initscr();start_color();
	cbreak();noecho();nonl();
	nodelay(stdscr,TRUE);
	keypad(stdscr,TRUE);
	leaveok(stdscr,FALSE);
	if(LINES<MH*2)MH=LINES/2-1;
	init_pair(1,COLOR_BLUE,COLOR_BLACK);
	init_pair(2,COLOR_GREEN,COLOR_BLACK);
	init_pair(3,COLOR_CYAN,COLOR_BLACK);
	init_pair(4,COLOR_RED,COLOR_BLACK);
	init_pair(5,COLOR_MAGENTA,COLOR_BLACK);
	init_pair(6,COLOR_YELLOW,COLOR_BLACK);
	init_pair(7,COLOR_WHITE,COLOR_BLACK);
	init_pair(8,COLOR_BLACK,COLOR_WHITE);
	init_pair(9,COLOR_RED,COLOR_WHITE);
	init_pair(10,COLOR_MAGENTA,COLOR_WHITE);
	init_pair(11,COLOR_YELLOW,COLOR_WHITE);
	init_pair(12,COLOR_YELLOW,COLOR_BLUE);
	init_pair(13,COLOR_WHITE,COLOR_BLUE);
	init_pair(14,COLOR_CYAN,COLOR_BLUE);
	init_pair(15,COLOR_GREEN,COLOR_BLUE);
	init_pair(16,COLOR_BLACK,COLOR_CYAN);
	bkgd(COLOR_PAIR(2)|' ');
	draw_screen();
 	signal(SIGWINCH,sigwinch);
	wmain=newwin(MH,COL,1,1);
	scrollok(wmain,FALSE);
	wbkgd(wmain,COLOR_PAIR(6)|' ');
 	wlog=newwin(LOGSIZE,COL,MH+2,1);
	wbkgd(wlog,COLOR_PAIR(7)|' ');
	scrollok(wlog,TRUE);
	wstat=newwin(1,COL-2,LINES-2,2);
	wbkgd(wstat,COLOR_PAIR(6)|' ');
	whelp=newwin(1,COL,LINES-1,1);
	wbkgd(whelp,COLOR_PAIR(8)|' ');
	whdr=newwin(1,COL-2,0,2);
	wbkgd(whdr,COLOR_PAIR(13)|A_BOLD|' ');
	wrefresh(wmain);
	wrefresh(wstat);
	wrefresh(whdr);
 	wrefresh(wlog);
	wrefresh(whelp);
}

static void donescreen(void)
{
	int i;
	for(i=0;i<allslots;i++)delwin(slots[i]->wlog);
	delwin(wlog);
	delwin(whdr);
	delwin(wstat);
	delwin(wmain);
	delwin(whelp);
	clear();
	refresh();
	endwin();
}

static void freshhelp(void)
{
	int i,k=0;
	char **hlp=(edm?hi:((currslot<0)?hm:(slots[currslot]->chat?hc:hl)));
	werase(whelp);
	for(i=0;hlp[i];i++)if(*hlp[i]) {
		if(i%2)wattron(whelp,COLOR_PAIR(8));
		  else wattron(whelp,COLOR_PAIR(9));
		if(*hlp[i]==3&&!slots[currslot]->session)wattron(whelp,(i%2)?COLOR_PAIR(11):(COLOR_PAIR(10)));
		if(*hlp[i]<3&&*hlp[i]!=ins+1)i++;
		    else {
			mvwaddstr(whelp,0,k,hlp[i]+(*hlp[i]<4));
			k+=strlen(hlp[i]+(*hlp[i]<4));
		}
	}
	wattron(whelp,COLOR_PAIR(10));
	mvwprintw(whelp,0,COL-4-strlen(version),"qcc-%s",version);
}

static void freshhdr(void)
{
	werase(whdr);
	wattron(whdr,COLOR_PAIR(14));
	mvwprintw(whdr,0,0,"[%d/%d] ",currslot+1,allslots);
	wattron(whdr,COLOR_PAIR(13));
	waddnstr(whdr,currslot>=0?slots[currslot]->tty:"master", QCC_TTYLEN);
	waddch(whdr,' ');
	wattron(whdr,COLOR_PAIR(12));
	waddstr(whdr,currslot>=0?slots[currslot]->header:m_header);
}

static void freshstatus(void)
{
	werase(wstat);
	waddstr(wstat,currslot>=0?slots[currslot]->status:m_status);
}

static char *timestr(time_t tim)
{
	static char ts[10];
	long int hr;
	if(tim<0) tim=0;
	hr=tim/3600;
	snprintf(ts,10,"%02ld:%02i:%02i",hr,(int)(tim/60-hr*60),tim%60);
	return ts;
}

static void bar(int o, int t, int l)
{
	int i,k=t/l,x=0;
	for(i=0;i<l;i++,x+=k)waddch(wmain,(k<o-x+1)?ACS_BLOCK:ACS_CKBOARD);
}

static void freshpfile(int b, int e, pfile_t *s, int act)
{
	char bf[20];
	if(!s->ftot)return;
	if(s->cps<=0)s->cps=1;
	scrollok(wmain,FALSE);
	wattrset(wmain,(act?COLOR_PAIR(4):COLOR_PAIR(2))|A_BOLD);
	mvwprintw(wmain,4,b,"%s file %d of %d",act?"Send":"Receive",s->nf,s->allf);
	wattrset(wmain,COLOR_PAIR(6)|A_BOLD);
	mvwaddnstr(wmain,5,b,s->fname,e-b);
	wattrset(wmain,COLOR_PAIR(7)|A_BOLD);
	snprintf(bf,20," %d cps",s->cps);
	mvwaddstr(wmain,4,e-strlen(bf),bf);
	wattroff(wmain,A_BOLD);
	mvwprintw(wmain,6,b,"Current: %ld of %ld bytes",(unsigned long) s->foff,(unsigned long) s->ftot);
	wattron(wmain,A_BOLD);
	wmove(wmain,7,b);bar(s->foff,s->ftot,e-b-9);waddch(wmain,' ');
	waddstr(wmain,timestr((s->ftot-s->foff)/s->cps));
	wattrset(wmain,COLOR_PAIR(1));
	mvwprintw(wmain,8,b,"Total: %ld%c of %ld%c",(unsigned long) SIZES(s->toff+s->foff),
	    SIZEC(s->toff+s->foff),(unsigned long) SIZES(s->ttot),SIZEC(s->ttot));
	wattron(wmain,A_BOLD);
	wmove(wmain,9,b);bar(s->toff+s->foff,s->ttot,e-b-9);waddch(wmain,' ');
	waddstr(wmain,timestr((s->ttot-s->toff-s->foff)/s->cps));
}

static void freshslot(void)
{
	werase(wmain);
	if(slots[currslot]->chat) {
		scrollok(wmain,TRUE);wattrset(wmain,COLOR_PAIR(7));
		mvwaddnstr(wmain,0,0,slots[currslot]->cl,MH*CHH-1);
		getyx(wmain,crey,crex);wattron(wmain,A_BOLD|A_BLINK);
		waddch(wmain,'_');wattroff(wmain,A_BOLD|A_BLINK);
	} else {
	if(!slots[currslot]->session)return;
	scrollok(wmain,FALSE);
	wattrset(wmain,COLOR_PAIR(3)|A_BOLD);
	mvwprintw(wmain,0,1,"%s // %s",slots[currslot]->name,SS(slots[currslot]->city));
	wattroff(wmain,A_BOLD);
	mvwprintw(wmain,0,COL-9-strlen(SS(slots[currslot]->sysop))," Sysop: %s",slots[currslot]->sysop);
	mvwprintw(wmain,1,1,"[%d] %s",slots[currslot]->speed,SS(slots[currslot]->flags));
	mvwprintw(wmain,1,COL-9-strlen(SS(slots[currslot]->phone))," Phone: %s",slots[currslot]->phone);
	wattron(wmain,COLOR_PAIR(2)|A_BOLD);
	mvwaddstr(wmain,2,1,"Addr: ");
	wattroff(wmain,A_BOLD);
	waddnstr(wmain,SS(slots[currslot]->addrs),COL-14);
	wattrset(wmain,COLOR_PAIR(4));mvwaddch(wmain,2,COL-12,' ');
	mvwaddstr(wmain,2,COL-11,slots[currslot]->options&O_PWD?"[pwd]":"");
	wattrset(wmain,COLOR_PAIR(6));
	mvwaddstr(wmain,2,COLS-8,slots[currslot]->options&O_LST?"[lst]":"");
	wattrset(wmain,COLOR_PAIR(2));
	mvwhline(wmain,3,0,ACS_HLINE,COL);wattron(wmain,A_BOLD);
	mvwprintw(wmain,3,COLS/2-5,"[%s]",timestr(time(NULL)-slots[currslot]->start));
	freshpfile(1,COL/2-1,&slots[currslot]->r,0);
	freshpfile(COL/2+1,COL-1,&slots[currslot]->s,1);
	}
}

static void logit(char *str,WINDOW *w,int s)
{
	char bbf[CHH];
	int len = 0, y, x = 2, cu = 1;

	while( str && *str && x ) {
	    if ( *str == ' ' )
	        x--;
	    bbf[len++] = *str++;
	}
	while( str && *str && *str++ != ' ' );
	while( str && *str && *str != ' ' && *str != '.' )
	    bbf[len++] = *str++;

	while( str && *str && *str++ != ']' );
	xstrcpy( bbf + len, str, COL - len );
	len = strlen( bbf ) > COL ? COL : strlen( bbf );

	if(s>=0)cu=(slots[s]->lm==slots[s]->lc);
	getyx(w,y,x);
	if(cu&&x)waddch(w,'\n');
	if(cu) {
		scrollok(w,FALSE);
		waddnstr(w,bbf,COL);
		scrollok(w,TRUE);
	}
	if(s<0)return;
	slots[s]->lb[slots[s]->lm]=(char*)xmalloc(len+2);
	if(slots[s]->lb[slots[s]->lm]) {
		xstrcpy(slots[s]->lb[slots[s]->lm],bbf,len+1);
		slots[s]->lm++;
		if(cu)slots[s]->lc++;
		if(slots[s]->lm>=LGMAX) {
			if(!cu) {
				if(slots[s]->lc)slots[s]->lc--;
				    else {
					wmove(w,0,0);
					wdeleteln(w);
					wmove(w,LOGSIZE-1,0);
					scrollok(w,FALSE);
					waddnstr(w,slots[s]->lb[0],COL);
					scrollok(w,TRUE);
				}
			} else slots[s]->lc=LGMAX-1;
			xfree(slots[s]->lb[0]);
			for(len=1;len<LGMAX;len++)slots[s]->lb[len-1]=slots[s]->lb[len];
			slots[s]->lm=LGMAX-1;
			slots[s]->lb[LGMAX-1]=NULL;
		}
	}
}

static qslot_t *addqueue(qslot_t **l)
{
	qslot_t **t,*p=NULL;
	for(t=l;*t;t=&((*t)->next))p=*t;
	*t=(qslot_t*)xmalloc(sizeof(qslot_t));
	memset(*t,0,sizeof(qslot_t));
	(*t)->prev=p;
	(*t)->n=p?p->n+1:1;
	return *t;
}

static void killqueue(qslot_t **l)
{
	qslot_t *t;
	while(*l) {
		t=(*l)->next;
		xfree((*l)->addr);
		xfree(*l);
		*l=t;
	}
}

static void freshqueue(void)
{
	int i,k,l, current_selector;
	char str[MAX_STRING];
	qslot_t *q;
	werase(wmain);
	scrollok(wmain,FALSE);
	wattrset(wmain,COLOR_PAIR(6)|A_BOLD);
	mvwaddstr(wmain,0,0,"* Address");
	mvwaddstr(wmain,0,COL-19-Q_MAXBIT,"Mail   Files  Try  Flags");
	for(q=queue;q&&q->n<q_first;q=q->next);
	if(!q) {
		wattrset(wmain,COLOR_PAIR(3));
		mvwaddstr(wmain,MH/2,COL/2-8,"* Empty queue *");
	} else for(i=0;q&&i<MH-1;i++,q=q->next) {
		wattrset(wmain,COLOR_PAIR(3)|(q->flags&Q_DIAL?A_BOLD:0));
		current_selector = ( q_pos == q->n );
		if( current_selector )
		    wattrset(wmain,COLOR_PAIR(16));
		for(k=0;k<allslots;k++)if(slots[k]->session&&!strcmp(slots[k]->addrs,q->addr))k=999;
		mvwaddch(wmain,i+1,0,(k>=999)?'=':' ');
  		mvwaddch( wmain, i + 1, 1, current_selector ? '>' : ' ' );
		waddstr(wmain,q->addr);
		for(k=0;k<COL-24-Q_MAXBIT-strlen(q->addr);k++)waddch(wmain,' ');
		snprintf(str,MAX_STRING," %5lu%c %6lu%c  %3d  ",
		    SIZES(q->mail),SIZEC(q->mail),SIZES(q->files),SIZEC(q->files),q->try);
		waddstr(wmain,str);
		for(k=0,l=1;k<Q_MAXBIT;k++) {
			if(q_pos!=q->n)wattron(wmain,COLOR_PAIR(qclrs[k]));
			waddch(wmain,(q->flags&l)?qflgs[k]:'.');
			l<<=1;
		}
	}
}

static void freshall(void)
{
	freshhdr();wnoutrefresh(whdr);
	freshstatus();wnoutrefresh(wstat);
	freshhelp();wnoutrefresh(whelp);
	if(currslot>=0)freshslot();
	    else freshqueue();
	wnoutrefresh(wmain);
	redrawwin(((currslot<0)?wlog:slots[currslot]->wlog));
	wrefresh((currslot<0)?wlog:slots[currslot]->wlog);
}

static RETSIGTYPE sigwinch(int sig)
{
	sizechanged=1;
#ifdef SYS5SIGNALS
 	signal(SIGWINCH,sigwinch);
#endif
}

static RETSIGTYPE sighup(int sig)
{
	quitflag=1;
#ifdef SYS5SIGNALS
	signal(sig,sighup);
#endif
}

static int findslot(char *slt)
{
	int i;
	for(i=0;i<allslots;i++)if(!strcmp(slots[i]->tty,slt))return i;
	return -1;
}

static int createslot(char *slt, char d)
{
	slots[allslots]=xmalloc(sizeof(slot_t));
	memset(slots[allslots],0,sizeof(slot_t));
	xstrcpy(slots[allslots]->tty,slt,QCC_TTYLEN);
	if(!memcmp(slt,"CHT",3)||!memcmp(slt,"IP",2)) {
		slots[allslots]->chat=1;
		slots[allslots]->cl=(char*)xcalloc(MH+1,CHH+1);
		restrcpy(&slots[allslots]->header,"Chat");
		if(d)beep();
	}
	slots[allslots]->wlog=newwin(LOGSIZE,COL,MH+2,1);
	scrollok(slots[allslots]->wlog,TRUE);flash();
	return allslots++;
}

static void delslot(int slt)
{
	allslots--;
	if(currslot==allslots||currslot==slt)currslot--;
	delwin(slots[slt]->wlog);
	xfree(slots[slt]->cl);
	while(slots[slt]->lc) {
		xfree(slots[slt]->lb[slots[slt]->lc]);
		slots[slt]->lc--;
	}
	free(slots[slt]);
	for(;slt<=allslots;slt++)slots[slt]=slots[slt+1];
	freshall();
}

static int inputstr(char *str, char *name, int mode)
{
	WINDOW *iw;
	int ch,cp=0,sl=0,sp=0,bp=0,vl,i,getkey=0,fr,hstcurr=hstlast,ms,tmp,yp;
	struct tm *tt;time_t tim;
	struct timeval tv;fd_set rfds;
	memset(str,0,MAX_STRING);
	edm=1;freshhelp();wnoutrefresh(whelp);
	ms=mode?(mode==1?40:5):(MAX_STRING-2);
	if(ms<strlen(name))ms=strlen(name)+1;
	if((ms+2)>COL)ms=COL-2;vl=ms-1;
	yp=LINES/2-2;if(yp<=MH)yp=MH+1;
	iw=newwin(3,ms+2,yp,COL/2-ms/2);
	wbkgd(iw,COLOR_PAIR(8)|' ');
	wattron(iw,COLOR_PAIR(9));
	wborder(iw,ACS_VLINE,ACS_VLINE,ACS_HLINE,ACS_HLINE,ACS_ULCORNER,ACS_URCORNER,ACS_LLCORNER,ACS_LRCORNER);
	wattroff(iw,COLOR_PAIR(8));
	mvwaddch(iw,0,1,' ');waddstr(iw,name);
	wnoutrefresh(iw);doupdate();
	while(!getkey&&!quitflag) {
		ch=getch();fr=0;
		if(ch>=32&&ch<255) {
			if(sl>=(mode?(mode==1?40:5):(MAX_STRING-2))||(mode&&!strchr(mode==1?"icdhny:/.@1234567890 ":"1234567890",ch))){xbeep();}
			    else {
				if(ins)for(i=sl;i>=sp;i--)str[i+1]=str[i];
				str[sp]=(char)ch;
				sp++;
				if(ins||sp>sl)sl++;
				if(cp<vl)cp++;
				    else {
					bp+=8;
					cp-=7;
				}
				fr=1;
			}
		    } else switch(ch) {
			case KEY_LEFT:
				if(sp) {
					sp--;
					if(cp)cp--;
					    else if(bp) {
						if(bp<8) {
							cp=bp-1;
							bp=0;
						    } else {
							bp-=8;
							cp=7;
						}
						fr=1;
					}
				} else xbeep();
				break;
			case KEY_RIGHT:
				if(sp<sl) {
					sp++;
					if(cp<vl)cp++;
					    else {
						bp++;
						fr=1;
					}
				} else xbeep();
				break;
			case KEY_BACKSPACE:
				if(sl&&sp) {
					sl--;sp--;
					if(sp==sl) {
						if(bp) {
							if(cp>2)cp--;
							    else {
								tmp=bp;
								if(bp>(vl/3*2))bp-=vl/3*2;
								    else bp=0;
								cp+=tmp-bp-1;
							}
						} else if(cp)cp--;
					    } else {
						strncpy(str+sp,str+sp+1,sl-sp);
						if(cp)cp--;
						if(cp<3&&bp) {
							tmp=vl/3*2;
							if(bp>tmp) {
								bp-=tmp;
								cp+=tmp;
							    } else {
								cp+=bp;
								bp=0;
							}
						}
					}
					fr=1;
				} else xbeep();
				break;
			case KEY_DC:
				if(sp<sl) {
					sl--;
					if(sl)strncpy(str+sp,str+sp+1,sl-sp);
					fr=1;
				} else xbeep();
				break;
			case KEY_HOME:
				bp=cp=sp=0;
				fr=1;
				break;
			case KEY_END:
				sp=sl;
				if(sl>vl) {
					cp=vl;
					bp=sl-vl;
				    } else {
					cp=sl;
					bp=0;
				}
				fr=1;
				break;
			case KEY_PPAGE:
				if(hstcurr&&hstlast) {
					tmp=hstcurr;
					for(hstcurr=0;hstcurr<hstlast&&(*hst[hstcurr]!=mode+1);hstcurr++);
					if(hstcurr==hstlast){hstcurr=tmp;xbeep();break;}
					hstcurr++;
				}
			case KEY_UP:
				if(hstlast&&hstcurr) {
					if(!hst[HSTMAX]) {
						hst[HSTMAX]=xmalloc(strlen(str)+2);
						strcpy(hst[HSTMAX]+1,str);
						*hst[HSTMAX]=mode+1;
					}
					tmp=hstcurr;hstcurr--;
					while(hstcurr&&(*hst[hstcurr]!=mode+1))hstcurr--;
					if(*hst[hstcurr]==mode+1) {
						strcpy(str,hst[hstcurr]+1);
						sp=sl=strlen(str);
						if(sp>vl) {
							cp=vl;
							bp=sl-vl;
						    } else {
							cp=sl;
							bp=0;
						}
						fr=1;
					} else {
						hstcurr=tmp;
						if(hstcurr==hstlast)xfree(hst[HSTMAX]);
						xbeep();
					}
				} else xbeep();
				break;
			case KEY_DOWN:
				if(hstcurr<hstlast) {
					hstcurr++;
					while(hstcurr<hstlast&&hst[hstcurr]&&(*hst[hstcurr]!=mode+1))hstcurr++;
					if(hst[hstcurr])strcpy(str,hst[hstcurr]+1);
					    else {
						strcpy(str,hst[HSTMAX]+1);
						xfree(hst[HSTMAX]);
					}
					sp=sl=strlen(str);
					if(sp>vl) {
						cp=vl;
						bp=sl-vl;
					    } else {
						cp=sl;
						bp=0;
					}
				} else xbeep();
				fr=1;
				break;
			case KEY_IC:
				ins^=1;
				freshhelp();wnoutrefresh(whelp);
				break;
			case KEY_NPAGE:
				xfree(hst[HSTMAX]);
				bp=sp=cp=sl=0;
				hstcurr=hstlast;
				fr=1;
				break;
			case '\r': case '\n':
				str[sl]=0;
				tmp=hstlast?(hstlast-1):0;
				while(tmp&&(*hst[tmp]!=mode+1))tmp--;
				if(*str&&strlen(str)>1&&(!hst[tmp]||strcmp(str,hst[tmp]+1))) {
					hst[hstlast]=(char*)xmalloc(strlen(str)+2);
					if(hst[hstlast]) {
						strcpy(hst[hstlast]+1,str);
						*hst[hstlast]=mode+1;
						hstlast++;
						if(hstlast>=HSTMAX) {
							xfree(hst[0]);
							for(i=1;i<HSTMAX;i++)hst[i-1]=hst[i];
							hstlast=HSTMAX-1;
							hst[hstlast]=NULL;
						}
					}
				}
				xfree(hst[HSTMAX]);
				hstcurr=hstlast;
				getkey=fr=1;
				break;
			case 0x1b: case KEY_F(8):
				getkey=fr=1;
				bp=sp=cp=sl=0;
				break;
			default:
				while(getmessages(NULL)>0);
				tim=time(NULL);tt=localtime(&tim);
				wattron(whdr,COLOR_PAIR(15));
				mvwprintw(whdr,0,COL-11,"%02d:%02d:%02d",tt->tm_hour,tt->tm_min,tt->tm_sec);
				wnoutrefresh(whdr);
				wmove(iw,1,cp+1);
				wnoutrefresh(iw);
				doupdate();
				FD_ZERO(&rfds);FD_SET(0,&rfds);
				tv.tv_sec=0;tv.tv_usec=5000;
				select(1,&rfds,NULL,NULL,&tv);
				break;
		}
		str[sl]=0;
		if(fr) {
			mvwaddnstr(iw,1,1,"                                                                                                                                                   ",ms);
			if(str[bp])mvwaddnstr(iw,1,1,str+bp,vl);
			wnoutrefresh(iw);doupdate();
		}
	}
	delwin(iw);
	if(yp==MH+1) {
		attrset(COLOR_PAIR(2));
		mvhline(MH+1,1,ACS_HLINE,COL);
	}
	refresh();edm=0;freshall();
	freshhelp();wnoutrefresh(whelp);
	return(*(char*)str);
}

static char *getnode(char *name)
{
	int i,flv;
	char buf[MAX_STRING + 5],*nm;
	static char ou[64];
	unsigned long zone,net,node,n,point=0;
	if(!myaddr)myaddr="0:0/0";
rei:	zone=strtoul(myaddr,&nm,10);
	net=strtoul(nm+1,&nm,10);
	node=strtoul(nm+1,&nm,10);
	if(*nm=='.')point=strtoul(nm+1,NULL,10);
	inputstr(buf,name,1);
	*(long*)ou=0;flv='n';
	if(*buf>32) {
		for(i=0,n=0;i<strlen(buf)&&flv!='e';i++) {
			if(buf[i]>='0'&&buf[i]<='9'){n=n*10+buf[i]-'0';if(n>32767){beep();goto rei;}}
			  else if(buf[i]==':'){zone=n?n:zone;n=0;}
			    else if(buf[i]=='/'){net=n?n:net;n=0;}
			      else if(buf[i]=='.'){node=n?n:node;n=0;}
				else if(strchr("icdhny",buf[i]))flv=buf[i];
		    		  else if(buf[i]!=' '&&buf[i]!='@')flv='e';
		}
		if(strchr(buf,'.'))point=n?n:point;
		    else {node=n?n:node;point=0;}
		snprintf(ou,63,"%c%lu:%lu/%lu",flv,zone,net,node);
		if(point)snprintf(ou+strlen(ou),63 - strlen(ou),".%lu",point);
		if(flv=='e') {
			write_log("err: in: '%s', ident as '%s', ignored",buf,ou);
			*(int*)ou=0;
		}
#if QDEBUG==1
		else write_log("in: '%s', ident as '%s'",buf,ou);
#endif
	}
	return ou;
}

static void xcmd(char *buf, int cmd, int len)
{
	STORE16(buf,0);
	buf[2]=cmd;
#if QDEBUG==1
	write_log("send: [%d] '%s','%s'",buf[2],buf+3,buf+4+strlen(buf+3));
#endif
	if(xsend(sock,buf,len)<0)write_log("can't send to server: %s",strerror(errno));
}

static void xcmdslot(char *buf, int cmd, int len)
{
	STORE16(buf,slots[currslot]->id);
	buf[2]=cmd;
#if QDEBUG==1
	write_log("sendslot: [%d] '%s','%s'",buf[2],buf+3,buf+4+strlen(buf+3));
#endif
	if(xsend(sock,buf,len)<0)write_log("can't send to server: %s",strerror(errno));
}

static void printinfo(char *addr, int what, char *buf)
{
	int	rc;
	char	*p, *t;

	if ( strchr( "NDICH", toupper( *addr )))
		addr++;
	else if( what )
		return;

	if ( !addr || !*addr )
		return;

	xstrcpy( buf + 3, addr, 64 );
	xcmd( buf, QR_INFO, strlen( addr ) + 4 );
	while( getmessages( buf ) > 0 );

	rc = buf[2];
	if ( rc )
		return;

	for( p = buf + 3; strlen( p ); rc++ ) {
		write_log( "%s: %s", infostrs[rc], p );

		if ( rc == 5 && (t = wktime_str( p )))
			write_log( " WkTime: %s", t );

		p += strlen( p ) + 1;
	}
}


static int getmessages(char *bbx)
{
	unsigned short id;
	int len,type=0,rc;
	static int lastfirst=1,lastpos=1;
	char buf[MSG_BUFFER];
	unsigned char *data,*p;
	rc=xrecv(sock,buf,MSG_BUFFER-1,0);
	if(!rc)return -1;
	if(rc>0&&rc<MSG_BUFFER)buf[rc]=0;
#if QDEBUG==1
	if(rc>0)write_log("recv: [%d,%d] '%s','%s'",buf[2],rc,buf+3,buf+4+strlen(buf+3));
#endif
	if(bbx&&rc>=3&&buf[2]<8) {
		memcpy(bbx,buf,rc);
		return 0;
	}
	if(bbx&&allslots==-9)return 1;
	if(rc>=3) {
		id=FETCH16(buf);
		type=buf[2];
		if(type<8)return 1;
		data=(unsigned char*)(strchr(buf+3,0)+1);
		len=rc-4-strlen(buf+3);
		if(strcmp(buf+3,"master")) {
			if(!strcasecmp(buf+3,"ipline"))snprintf(buf+5,5,"%04x",id);
			rc=findslot(buf+3);
			if(type==QC_ERASE) {
				if(rc>=0&&allslots<MAX_SLOTS&&buf[3]=='i'&&buf[4]=='p') {
					xstrcpy(slots[rc]->tty,"ipline",QCC_TTYLEN);
					freshhdr();wrefresh(whdr);
					slots[rc]->session=0;
					slots[rc]->id=0;
					rc=-1;
				} else if(allslots>0&&rc>=0) {
					delslot(rc);
					rc=-1;
				}
			} else if(rc<0) {
				if(buf[3]=='i'&&buf[4]=='p') {
					for(rc=0;rc<allslots;rc++)
					    if(!slots[rc]->session&&!slots[rc]->id&&*slots[rc]->tty=='i'&&slots[rc]->tty[1]=='p') {
						xstrcpy(slots[rc]->tty,buf+3,QCC_TTYLEN);
						break;
					    }
					if(rc>=allslots)rc=-1;
				}
				if(rc<0&&allslots<MAX_SLOTS)rc=createslot(buf+3,*data);
				freshhdr();wrefresh(whdr);
			}
			if(rc>=0)slots[rc]->id=id;
			    else type=-1;
			switch(type) {
			    case QC_SLINE:
				restrcpy(&slots[rc]->status,(char*)data);
				freshstatus();wrefresh(wstat);
				break;
			    case QC_LOGIT:
				logit((char*)data,slots[rc]->wlog,rc);
				if(currslot==rc&&!edm)wrefresh(slots[rc]->wlog);
				break;
			    case QC_TITLE:
				restrcpy(&slots[rc]->header,(char*)data);
				if(currslot==rc) {
					freshhdr();wrefresh(whdr);
				}
				break;
			    case QC_EMSID:
				if(!len) {
					xfree(slots[rc]->name);
					xfree(slots[rc]->sysop);
					xfree(slots[rc]->city);
					xfree(slots[rc]->flags);
					xfree(slots[rc]->phone);
					xfree(slots[rc]->addrs);
					slots[rc]->session=0;
				} else {
					p=data;
					slots[rc]->speed=FETCH32(p);INC32(p);
					slots[rc]->opt=FETCH16(p);INC16(p);
					slots[rc]->options=FETCH32(p);INC32(p);
					slots[rc]->start=FETCH32(p);INC32(p);
					restrcpy(&slots[rc]->name,(char*)p);p+=strlen((char*)p)+1;
					restrcpy(&slots[rc]->sysop,(char*)p);p+=strlen((char*)p)+1;
					restrcpy(&slots[rc]->city,(char*)p);p+=strlen((char*)p)+1;
					restrcpy(&slots[rc]->flags,(char*)p);p+=strlen((char*)p)+1;
					restrcpy(&slots[rc]->phone,(char*)p);p+=strlen((char*)p)+1;
					restrcpy(&slots[rc]->addrs,(char*)p);
					slots[rc]->session=1;
				}
				if(currslot==rc) {
					freshslot();wrefresh(wmain);
				}
				freshhelp();wnoutrefresh(whelp);
				break;
			    case QC_LIDLE:
				if(slots[rc]->session) {
					slots[rc]->session=0;
					memset(&slots[rc]->r,0,sizeof(pfile_t));
					memset(&slots[rc]->s,0,sizeof(pfile_t));
					if(currslot==rc) {
						freshslot();wrefresh(wmain);
					}
					freshhelp();wnoutrefresh(whelp);
					if(buf[3]=='i'&&buf[4]=='p') {
						buf[3]='I';buf[4]='P';
					} else {
						buf[3]='C';buf[4]='H';
						buf[5]='T';
					}
					rc=findslot(buf+3);
					if(rc>=0&&allslots>0)delslot(rc);
				}
				break;
			    case QC_SENDD:
				if(!len) {
					memset(&slots[rc]->s,0,sizeof(pfile_t));
				} else {
					p=data;
					slots[rc]->session=1;
					xfree(slots[rc]->s.fname);
					slots[rc]->s.foff=FETCH32(p);INC32(p);
					slots[rc]->s.ftot=FETCH32(p);INC32(p);
					slots[rc]->s.toff=FETCH32(p);INC32(p);
					slots[rc]->s.ttot=FETCH32(p);INC32(p);
					slots[rc]->s.nf=FETCH16(p);INC16(p);
					slots[rc]->s.allf=FETCH16(p);INC16(p);
					slots[rc]->s.cps=FETCH32(p);INC32(p);
					slots[rc]->s.soff=FETCH32(p);INC32(p);
					slots[rc]->s.stot=FETCH32(p);INC32(p);
					slots[rc]->s.start=FETCH32(p);INC32(p);
					slots[rc]->s.mtime=FETCH32(p);INC32(p);
					slots[rc]->s.fname=xstrdup((char*)p);
					freshhelp();wnoutrefresh(whelp);
				}
				if(currslot==rc) {
					freshslot();wrefresh(wmain);
				}
				break;
			    case QC_RECVD:
				if(!len)memset(&slots[rc]->r,0,sizeof(pfile_t));
				    else {
					p=data;
					slots[rc]->session=1;
					xfree(slots[rc]->r.fname);
					slots[rc]->r.foff=FETCH32(p);INC32(p);
					slots[rc]->r.ftot=FETCH32(p);INC32(p);
					slots[rc]->r.toff=FETCH32(p);INC32(p);
					slots[rc]->r.ttot=FETCH32(p);INC32(p);
					slots[rc]->r.nf=FETCH16(p);INC16(p);
					slots[rc]->r.allf=FETCH16(p);INC16(p);
					slots[rc]->r.cps=FETCH32(p);INC32(p);
					slots[rc]->r.soff=FETCH32(p);INC32(p);
					slots[rc]->r.stot=FETCH32(p);INC32(p);
					slots[rc]->r.start=FETCH32(p);INC32(p);
					slots[rc]->r.mtime=FETCH32(p);INC32(p);
					slots[rc]->r.fname=xstrdup((char*)p);
					freshhelp();wnoutrefresh(whelp);
				}
				if(currslot==rc) {
					freshslot();wrefresh(wmain);
				}
				break;
			    case QC_CHAT:
				while((p=(unsigned char*)strchr((char*)data,7))) {
					*(char*)p='*';
					flash();beep();flash();
				}
				wattrset(wmain,COLOR_PAIR(7));
				wmove(wmain,crey,crex);
				waddch(wmain,' ');
				wmove(wmain,crey,crex);
				waddstr(wmain,(char*)data);
				getyx(wmain,crey,crex);
				wattron(wmain,A_BOLD|A_BLINK);
				waddch(wmain,'_');
				wattroff(wmain,A_BOLD|A_BLINK);
				if(strchr((char*)data,'\b')) {
					scrollok(wmain,FALSE);
					waddch(wmain,'\n');
					scrollok(wmain,TRUE);
				}
				if(currslot==rc&&!edm)wrefresh(wmain);
				strcat(slots[rc]->cl,(char*)data);
				if((p=(unsigned char*)strchr(slots[rc]->cl,'\n'))) {
					if(slots[rc]->chats>=MH)
					    xstrcpy(slots[rc]->cl,(char*)p,MH*CHH-1);
					else slots[rc]->chats++;
				} else if(strlen(slots[rc]->cl)>=(MH*(CHH-1))) {
					memcpy(slots[rc]->cl,slots[rc]->cl+CHH,strlen(slots[rc]->cl)-(CHH-2));
					slots[rc]->chats=MH;
				}
				break;
			}
		} else {
			switch(type) {
			    case QC_SLINE:
				restrcpy(&m_status,(char*)data);
				if(currslot<0) {
					freshstatus();wrefresh(wstat);
				}
				break;
			    case QC_TITLE:
				restrcpy(&m_header,(char*)data);
				if(currslot<0) {
					freshhdr();wrefresh(whdr);
				}
				break;
			    case QC_LOGIT:
				logit((char*)data,wlog,-1);
				if(currslot<0&&!edm)wrefresh(wlog);
				break;
			    case QC_MYDATA:
				restrcpy(&myaddr,(char*)data);
				break;
			    case QC_QUEUE:
				if(!len) {
					lastfirst=q_first;
					lastpos=q_pos;
					killqueue(&queue);
					q_max=0;
					q_first=1;
					q_pos=1;
				} else {
					qslot_t *q=addqueue(&queue);
					p=data;
					q_max=q->n;
					q->mail=FETCH32(p);INC32(p);
					q->files=FETCH32(p);INC32(p);
					q->flags=FETCH32(p);INC32(p);
					q->try=FETCH16(p);INC16(p);
					q->addr=xstrdup((char*)p);
					if(lastfirst>=q->n)q_first=q->n;
					if(lastpos>=q->n)q_pos=q->n;
				}
				if(currslot<0) {
					freshqueue();wrefresh(wmain);
				}
				break;
			    case QC_QUIT:
				quitflag=1;
				break;
			}
		}
	}
	return(bbx?1:(type==QC_QUEUE));
}

int main(int argc, char **argv, char **envp)
{
	int len,ch,rc;
	struct tm *tt;
	char buf[4096],*bf,c,*port=NULL,*addr=NULL,*pwd=NULL;
	unsigned char digest[16]={0};
	fd_set rfds;
	time_t tim;
	struct timeval tv;
#ifdef CURS_HAVE_RESIZETERM
	struct winsize size;
#endif

	initsetproctitle( argc, argv, envp );

 	while((c=getopt(argc,argv,"hnvP:a:w:"))!=-1) {
		switch(c) {
		    case 'P':
			if(optarg&&*optarg)port=xstrdup(optarg);
			break;
		    case 'a':
			if(optarg&&*optarg)addr=xstrdup(optarg);
			break;
		    case 'w':
			if(optarg&&*optarg)pwd=xstrdup(optarg);
			break;
		    case 'n':
			beepdisable=1;
			break;
		    case 'v':
			u_vers("qcc");
		    default:
			usage(argv[0]);
		}
	}
	setproctitle( "qico control center" );
	signal(SIGPIPE,SIG_IGN);
	signal(SIGHUP,sighup);
	signal(SIGTERM,sighup);
	signal(SIGSEGV,sighup);
	sock=cls_conn(CLS_UI,port,addr);
	if(sock<0) {
		fprintf(stderr,"can't connect to server: %s.\n",strerror(errno));
		return 1;
	}

#ifdef HAVE_SETLOCALE
	setlocale(LC_ALL, "C");
#endif

	signal(SIGALRM, sighup);
	alarm(6);
	rc=xrecv(sock,buf,MSG_BUFFER-1,1);
	if(!rc||quitflag) {
		fprintf(stderr,"connection timeout.\n");
		return 1;
	}
	alarm(0);
	signal(SIGALRM, SIG_IGN);
	if(strcmp(buf,"qs-noauth")) {
		if(pwd&&*pwd=='-'&&!pwd[1]) {
			xfree(pwd);
			pwd=getpass("password: ");
			if(!pwd) {
				fprintf(stderr,"getpass() error: %s.\n",strerror(errno));
				cls_close(sock);
				return 1;
			}
			pwd=xstrdup(pwd);
		}
		if(!pwd||(pwd&&!*pwd)) {
			fprintf(stderr,"can't connect: password required.\n");
			cls_close(sock);
			return 1;
		}
		md5_cram_get((unsigned char*)pwd,(unsigned char*)buf,10,digest);
	}
	STORE16(buf,0);
	buf[2]=QR_STYPE;
	buf[3]='e';/*events*/
	memcpy(buf+4,digest,16);
	snprintf(buf+20,MSG_BUFFER-21,"qcc-%s",version);
	xcmd(buf,QR_STYPE,strlen(buf+20)+20);
	tim=time(NULL)+6;buf[2]=1;
	while(getmessages(buf)>0&&time(NULL)<tim);
	if(buf[2]||time(NULL)>=tim) {
		fprintf(stderr,"can't connect: %s.\n",buf+3);
		cls_close(sock);
		return 1;
	}
	signal(SIGINT,sighup);
	signal(SIGKILL,sighup);
	srand(time(NULL));
/*cyr*/	printf("\033(K");
	fflush(stdout);
	initscreen();
	currslot=-1;
	allslots=0;
	freshhdr();wrefresh(whdr);
	memset(&slots,0,sizeof(slots));
	for(ch=0;ch<=HSTMAX;ch++)hst[ch]=NULL;
	hstlast=0;
	freshall();
	write_log("I'm, qcc-%s, successfully started! ;)", version );
	while(!quitflag) {
#ifdef CURS_HAVE_RESIZETERM
		if (sizechanged) {
			if(!ioctl(fileno(stdout),TIOCGWINSZ,&size)) {
				LINES=size.ws_row;COLS=size.ws_col;
				if(LINES<MH*2)MH=LINES/2-1; else MH=MAXMH;
				resizeterm(LINES,COLS);draw_screen();
				for(ch=0;ch<allslots;ch++)wresize(slots[ch]->wlog,LOGSIZE,COL);
				wresize(wmain,MH,COL);wresize(wlog,LOGSIZE,COL);
				mvwin(wstat,LINES-2,2);wresize(wstat,1,COL-2);
				mvwin(whelp,LINES-1,1);wresize(whelp,1,COL);
				wresize(whdr,1,COL-2);freshall();
			}
			sizechanged=0;
 		}
#endif
		tim=time(NULL);
		tt=localtime(&tim);wattron(whdr,COLOR_PAIR(15));
		mvwprintw(whdr,0,COL-11,"%02d:%02d:%02d",tt->tm_hour,tt->tm_min,tt->tm_sec);
		wnoutrefresh(whdr);wnoutrefresh(wstat);
		if(currslot>0&&slots[currslot]->chat) {
			wmove(slots[currslot]->wlog,slots[currslot]->chaty,slots[currslot]->chatx);
			wnoutrefresh(slots[currslot]->wlog);
		}
		doupdate();
		FD_ZERO(&rfds);
		FD_SET(0,&rfds);
		FD_SET(sock,&rfds);
		tv.tv_sec=1;
		tv.tv_usec=0;
		rc=select(sock+1,&rfds,NULL,NULL,&tv);
		if(rc<0&&errno!=EINTR)write_log("err in select: %s",strerror(errno));
		if(rc>0&&FD_ISSET(sock,&rfds))do {
			ch=getmessages(NULL);
			if(ch<0)quitflag=1;
		} while(ch>0);
		if(rc>0&&FD_ISSET(0,&rfds)) {
		ch=getch();
		if(ch==0x1b)ch=getch();
		if(ch==ERR)continue;
#ifdef KEY_RESIZE
		if(ch==KEY_RESIZE)continue;
#endif
		if(ch==('L'-'@')) {
			draw_screen();
			freshall();
		} else if ( ch == '|' ) {
		    FILE *save_view = fopen( "qcc.view", "w" );
		    if ( save_view ) {
		        putwin( stdscr, save_view );
		        fclose( save_view );
		    }
		}else if(allslots&&(ch=='\t'||ch==KEY_RIGHT)) {
			if(currslot<(allslots-1)||ch=='\t') {
				currslot++;
				if(currslot==allslots)currslot=-1;
				freshall();
			}
		} else if(ch==KEY_LEFT) {
			if(currslot>=0) {
				currslot--;
				freshall();
			}
		} else if(ch==KEY_F(10))quitflag=1;
		    else if(ch==KEY_F(1)){for(ch=0;help[ch];ch++)write_log(help[ch],version);}
			else if(ch>0&&(unsigned char)ch<255&&currslot>0&&slots[currslot]->chat) {
				if(ch!=KEY_F(2)) {
					if(ch==KEY_BACKSPACE||ch==127)ch='\b';
					if(ch==KEY_F(4))ch=7;
					if(ch=='\r')ch='\n';
					if(ch==0x1b||ch==KEY_F(8))ch=0;
					    else if(ch>255)ch=' ';
					buf[3]=ch;
					if(ch==7)ch='*';
					if(ch=='\b')waddstr(slots[currslot]->wlog,"\b ");
					if(ch)waddch(slots[currslot]->wlog,ch);
					    else waddstr(slots[currslot]->wlog,"\nClosing..\n");
					wrefresh(slots[currslot]->wlog);
					xcmdslot(buf,QR_CHAT,4);
				} else if(inputstr(buf+3,"Text: ",0)) {
					waddstr(slots[currslot]->wlog,buf+3);
					xcmdslot(buf,QR_CHAT,3+strlen(buf+3));
			}
			wrefresh(slots[currslot]->wlog);
			getyx(slots[currslot]->wlog,slots[currslot]->chaty,slots[currslot]->chatx);
		    } else {
                    if ( ch == '`' || ( ch >= '0' && ch <= '9' )) {
		        if ( ch == '`' )
		            ch = '0';
			ch=ch-'1';
			if(ch<allslots) {
				currslot=ch;
				freshall();
			}
		    } else {
			qslot_t *que=queue;
			switch(ch) {
			case 'q':
				quitflag=1;
				break;
			case ' ':
				xcmd(buf,QR_RESTMR,3);
				break;
			case 'p':
				bf=getnode("Create poll for address: ");
				if(bf&&strchr("HDNCI",toupper(*bf))&&bf[1]) {
					xstrcpy(buf+3,bf+1,64);
					buf[4+strlen(buf+3)]=toupper(*bf);
					xcmd(buf,QR_POLL,strlen(buf+3)+5);
				}
				break;
			}
			if(currslot<0)switch(ch) {
			case KEY_UP:
				if(q_pos>1)if(--q_pos<q_first)q_first--;
				freshqueue();wrefresh(wmain);
				break;
			case KEY_DOWN:
				if(q_pos<q_max)if(++q_pos>=q_first+MH-1)q_first++;
				freshqueue();wrefresh(wmain);
				break;
			case KEY_PPAGE:
				if(q_pos>1) {
					q_pos-=MH-2;
					q_first-=MH-2;
					if(q_pos<1){q_pos=1;q_first=1;}
					if(q_first<1)q_first=1;
				}
				freshqueue();wrefresh(wmain);
				break;
			case KEY_NPAGE:
				if(q_pos<q_max) {
					q_pos+=MH-2;
					q_first+=MH-2;
					if(q_pos>=q_max){q_pos=q_max;q_first=q_pos-(MH-2);}
					if(q_first<1)q_first=1;
				}
				freshqueue();wrefresh(wmain);
				break;
			case KEY_HOME:
				q_pos=q_first=1;
				freshqueue();wrefresh(wmain);
				break;
			case KEY_END:
				q_pos=q_max;
				q_first=(q_max>=MH-1)?(q_max-MH+2):1;
				freshqueue();wrefresh(wmain);
				break;
			case 'r':
				xcmd(buf,QR_SCAN,3);
				break;
			case 'R':
				xcmd(buf,QR_CONF,3);
				break;
			case 'w': case 'W': case 'u': case 'U':
			case 'i': case 'I': case 'h':
				for(;que&&que->n!=q_pos;que=que->next);if(!que)break;
				xstrcpy(buf+3,que->addr,64);
				len=strlen(buf+3)+4;
				buf[len]=ch;
				buf[len+1]=0;
				if(ch=='h') {
					if(inputstr(buf+len+4,"Set hold status to node for (minutes): ",2)) {
					    STORE16(buf+len+2,(unsigned short)strtoul(buf+len+4,NULL,10));
					    len+=2;
					} else break;
				}
				xcmd(buf,QR_STS,len+2);
				break;
			case 'k': case 'K':
				for(;que&&que->n!=q_pos;que=que->next);if(!que)break;
				bf=getnode((ch=='k')?"Kill all files for (addr+'y'): ":"Kill all files for current addr? ('y'=yes, any other=no): ");
				if(bf&&tolower(*bf)=='y'&&bf[1]) {
					xstrcpy(buf+3,(ch=='k')?(bf+1):que->addr,64);
					xcmd(buf,QR_KILL,strlen(buf+3)+4);
				}
				break;
			case 'f': case 'F':
				for(;que&&que->n!=q_pos;que=que->next);if(!que&&ch=='F')break;
				printinfo(ch=='F'?que->addr:getnode("Get info about address: "),ch=='f',buf);
				break;
			case 'e': case 'E':
				for(;que&&que->n!=q_pos;que=que->next);if(!que&&ch=='E')break;
				if(ch=='e') {
					bf=getnode("Request file(s) from node: ");
					if(!bf||*bf!='n'||!bf[1])break;
					bf++;
				} else bf=que->addr;
				len=strlen(bf);
				xstrcpy(buf+3,bf,MSG_BUFFER-3);
				bf=buf+len+4;
				if(inputstr(bf,"File(s) for request: ",0)) {
					len+=strlen(bf);
					while(bf&&*bf) {
						if(*bf=='"')bf=strchr(bf+1,'"');
						if(bf)bf=strchr(bf,' ');
						if(bf)*bf++=0;
					}
					xcmd(buf,QR_REQ,5+len);
				}
				break;
			case 's': case 'S':
				for(;que&&que->n!=q_pos;que=que->next);if(!que&&ch=='S')break;
				if(ch=='s') {
					bf=getnode("Send file to node: ");
					if(!bf||!strchr("HDNCI",toupper(*bf))||!bf[1])break;
					ch=*bf++;
				} else bf=que->addr;
				len=strlen(bf);
				xstrcpy(buf+3,bf,MSG_BUFFER-3);
				bf=buf+len+4;
				*bf++=(ch=='S')?'N':ch;*bf++=0;
				if(inputstr(bf,"Full file name: ",0)) {
					if(access(bf,R_OK)==-1)write_log("warn: can't access file '%s'",bf);
					while(*bf++)if(*bf==' ')*bf='?';
					xcmd(buf,QR_SEND,7+len+strlen(buf+len+6));
				}
				break;
			} else switch(ch) {
			case KEY_F(8):
				delslot(currslot);
				break;
			case KEY_UP: case KEY_IC:
			case KEY_PPAGE:	case KEY_HOME:
				if(slots[currslot]->lc>LOGSIZE) {
					len=(ch==KEY_UP?1:(ch==KEY_HOME?LGMAX:(ch==KEY_IC?2:(LOGSIZE-1))));
					while(len--&&slots[currslot]->lc>LOGSIZE) {
						slots[currslot]->lc--;
						bf=slots[currslot]->lb[slots[currslot]->lc-LOGSIZE];
						wmove(slots[currslot]->wlog,0,0);
						winsertln(slots[currslot]->wlog);
						waddnstr(slots[currslot]->wlog,bf,COL);
					}
					wmove(slots[currslot]->wlog,LOGSIZE-1,0);
					wrefresh(slots[currslot]->wlog);
				} else xbeep();
				break;
			case KEY_DOWN: case KEY_DC:
			case KEY_NPAGE:	case KEY_END:
				if(slots[currslot]->lc<slots[currslot]->lm) {
					len=(ch==KEY_DOWN?1:(ch==KEY_END?LGMAX:(ch==KEY_DC?2:(LOGSIZE-1))));
					while(len--&&slots[currslot]->lc<slots[currslot]->lm) {
						bf=slots[currslot]->lb[slots[currslot]->lc];
						slots[currslot]->lc++;
						wmove(slots[currslot]->wlog,0,0);
						wdeleteln(slots[currslot]->wlog);
						wmove(slots[currslot]->wlog,LOGSIZE-1,0);
						scrollok(slots[currslot]->wlog,FALSE);
						waddnstr(slots[currslot]->wlog,bf,COL);
						scrollok(slots[currslot]->wlog,TRUE);
					}
					wrefresh(slots[currslot]->wlog);
				} else xbeep();
				break;
			case 'X':
				if(slots[currslot]->session)xcmdslot(buf,QR_SKIP,3);
				    else xbeep();
				break;
			case 'R':
				if(slots[currslot]->session)xcmdslot(buf,QR_REFUSE,3);
				    else xbeep();
				break;
			case 'h':
				xstrcpy(buf+3,slots[currslot]->tty,QCC_TTYLEN);
				if(!(slots[currslot]->opt&(MO_IFC|MO_BINKP)))
					xcmd(buf,QR_HANGUP,strlen(buf+3)+4);
				xcmdslot(buf,QR_HANGUP,3);
				break;
			case 'c':
				if((slots[currslot]->opt&MO_CHAT||slots[currslot]->opt&MO_BINKP)&&slots[currslot]->session) {
					buf[3]=5;
					xcmdslot(buf,QR_CHAT,4);
				} else xbeep();
				break;
		    }
		}}
	    }
	}
	while(allslots)delslot(allslots-1);
	for(ch=0;ch<HSTMAX;ch++)xfree(hst[ch]);
	donescreen();
	cls_close(sock);
	signal(SIGHUP,SIG_DFL);
	signal(SIGTERM,SIG_DFL);
	signal(SIGINT,SIG_DFL);
	signal(SIGKILL,SIG_DFL);
	signal(SIGSEGV,SIG_DFL);
	return 0;
}
