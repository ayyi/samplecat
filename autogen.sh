#!/bin/sh

echo 'Generating necessary files...'
aclocal
autoheader -Wall
automake --gnu --add-missing -Wall
autoconf

