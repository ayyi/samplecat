#!/bin/sh

echo 'Generating necessary files...'
libtoolize --automake
aclocal
autoheader -Wall
automake --gnu --add-missing -Wall
autoconf

cd lib/waveform
./autogen.sh
cd ../..

#test -n "$NOCONFIGURE" || "$srcdir/configure" --enable-maintainer-mode "$@"
