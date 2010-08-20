# Copyright 1999-2010 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

EAPI="1"

inherit eutils subversion

DESCRIPTION="FTN mailer Qico (xe)"
HOMEPAGE="http://www.sf.net/project/qico"
ESVN_REPO_URI="https://icelan.ru/svn/qicoxe/trunk/"

LICENSE="LGPL-2"
SLOT="0"
KEYWORDS="x86 amd64"
IUSE="+hydra8k +binkp +qcc +notify perl -winfs xinetd -debug"
DEPEND="perl? ( dev-lang/perl )"
RDEPEND="${DEPEND}
	xinetd? ( sys-apps/xinetd )"


S="${WORKDIR}/qicoxe"

src_unpack() {
	subversion_src_unpack
	cd "${S}"
}

src_compile() {
	./autogen.sh
	local myconf="--with-config=/etc/qico/qico.conf"
	use debug || myconf="$myconf --disable-debug"
	use hydra8k && myconf = "$myconf --enable-hydra8k"
	use binkp || myconf = "$myconf --disable-binkp"
	use qcc || myconf="$myconf --disable-qcc"
	use notify || myconf="$myconf --disable-notify"
	use perl && myconf="$myconf --enable-perl"
	use winfs && myconf="$myconf --with-lock-style-open"
	econf $myconf
	emake || die "emake failed"
}

src_install() {
	emake DESTDIR="${D}" install || die "emake install failed"
	keepdir /var/run/qico
	fowners -R fido:fido /var/run/qico
	fperms -R 775 /var/run/qico
	keepdir /var/log/qico
	fowners -R fido:fido /var/log/qico
	fperms -R 775 /var/log/qico
	insinto "/etc/qico"
	doins -r ${S}/qico.*.sample || die
	newinitd ${FILESDIR}/qico.initd qico
	if use xinetd ; then
		insinto /etc/xinetd.d
		use binkp && newins "${FILESDIR}/binkp.xinetd" binkp
		newins "${FILESDIR}/ifc.xinetd" ifc
	fi
}

pkg_preinst() {
	enewgroup fido
	enewuser fido -1 /bin/false /dev/null fido
}
