# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

RESTRICT="nomirror"
IUSE=""
DESCRIPTION="SampleCat is a program for cataloguing and auditioning audio samples."
HOMEPAGE="http://orford.org/portfolio/samplecat/"
SRC_URI="http://orford.org/assets/${P}.tar.gz"

LICENSE="GPL-2"
KEYWORDS="x86"
SLOT="0"

DEPEND=">=media-sound/jack-audio-connection-kit-0.99
	>=media-libs/libsndfile-1.0.10
	>=dev-db/mysql-3.23.0
	>=x11-libs/gtk+-2.6"
	
src_compile() {
	econf || die
	emake || die
}

src_install() {
	make DESTDIR=${D} install || die
	dodoc NEWS
}
	
