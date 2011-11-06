#!/bin/sh

echo 'Generating necessary files...'
libtoolize --automake
aclocal
autoheader -Wall
automake --gnu --add-missing -Wall
autoconf

