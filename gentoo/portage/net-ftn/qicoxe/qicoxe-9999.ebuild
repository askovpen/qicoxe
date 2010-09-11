# Copyright 1999-2010 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

EAPI="1"

inherit eutils subversion

DESCRIPTION="FTN mailer Qico (xe)"
HOMEPAGE="http://www.sf.net/project/qico"
ESVN_REPO_URI="https://icelan.ru/svn/qicoxe/trunk/"

LICENSE="LGPL-2"
SLOT="0"
KEYWORDS=""
IUSE="+hydra8k +binkp +qcc +notify perl -winfs xinetd -debug"
DEPEND="perl? ( dev-lang/perl )
	qcc? ( sys-libs/ncurses )"
RDEPEND="${DEPEND}
	xinetd? ( sys-apps/xinetd )"


S="${WORKDIR}/qicoxe"

CONFDIR="${CONFDIR}"
LOGDIR="${LOGDIR}"
PIDDIR="${PIDDIR}"
SPOOLDIR="${SPOOLDIR}"

pkg_setup() {
	if [ -z ${CONFDIR} ] ; then
		CONFDIR="/etc/ftn"
		ewarn "CONFDIR is undefined, setting default to ${CONFDIR}"
	fi
	elog "CONFDIR=${CONFDIR}"

	if [ -z ${LOGDIR} ] ; then
		LOGDIR="/var/log/ftn"
		ewarn "LOGDIR is undefined, setting default to ${LOGDIR}"
	fi
	elog "LOGDIR=${LOGDIR}"

	if [ -z ${PIDDIR} ] ; then
		PIDDIR="/var/run/ftn"
		ewarn "PIDDIR is undefined, setting default to ${PIDDIR}"
	fi
	elog "PIDDIR=${PIDDIR}"

	if [ -z ${SPOOLDIR} ] ; then
		SPOOLDIR="/var/spool/ftn"
		ewarn "SPOOLDIR is undefined, setting default to ${PIDDIR}"
	fi
	elog "SPOOLDIR=${SPOOLDIR}"
}

src_unpack() {
	subversion_src_unpack
	cd "${S}"
	sed -e "s:/etc/ftn:${CONFDIR}:" -i qico.conf.sample
	sed -e "s:/var/log/ftn:${LOGDIR}:" -i qico.conf.sample
	sed -e "s:/var/run:${PIDDIR}:" -i qico.conf.sample
	sed -e "s:/var/spool/ftn:${SPOOLDIR}:" -i qico.conf.sample
}

src_compile() {
	./autogen.sh
	econf \
		--with-config=${CONFDIR}/qico.conf \
		$(use_enable debug debug) \
		$(use_enable hydra8k hydra8k) \
		$(use_enable binkp binkp) \
		$(use_enable qcc qcc) \
		$(use_enable notify notify) \
		$(use_enable perl perl) \
		$(useq winfs && echo "--with-lock-style=open")
	emake || die "emake failed"
}

pkg_preinst() {
	enewgroup fido
	enewuser fido -1 /bin/false /dev/null fido
}

src_install() {
	emake DESTDIR="${D}" install || die "emake install failed"
	keepdir ${PIDDIR}
	fowners fido:fido ${PIDDIR}
	fperms 775 ${PIDDIR}
	keepdir ${LOGDIR}
	fowners fido:fido ${LOGDIR}
	fperms 775 ${LOGDIR}
	keepdir ${SPOOLDIR}
	fowners fido:fido ${SPOOLDIR}
	fperms 775 ${SPOOLDIR}
	insinto "${CONFDIR}"
	doins -r ${S}/qico.*.sample || die
	newinitd ${FILESDIR}/qico.initd qico
	sed -e "s:/var/run/ftn:${PIDDIR}:g" -i ${D}/etc/init.d/qico
	if use xinetd ; then
		insinto /etc/xinetd.d
		use binkp && newins "${FILESDIR}/binkp.xinetd" binkp
		newins "${FILESDIR}/ifc.xinetd" ifc
	fi
}
