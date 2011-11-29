#!/bin/sh

echo 'Generating necessary files...'
libtoolize --automake
aclocal
autoheader -Wall
automake --gnu --add-missing -Wall
autoconf

#test -n "$NOCONFIGURE" || "$srcdir/configure" --enable-maintainer-mode "$@"
