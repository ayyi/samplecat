#!/bin/sh

SETCOLOR_WARN="echo -en \\033[1;33m"
SETCOLOR_FAILURE="echo -en \\033[1;31m"
SETCOLOR_NORMAL="echo -en \\033[0;39m"

echo 'Generating files...'
libtoolize --automake
aclocal
autoheader -Wall
automake --gnu --add-missing -Wall -Wno-override
autoconf

if [ ! -f lib/waveform/autogen.sh ]; then
	${SETCOLOR_WARN}
	echo "libwaveform submodule missing, will fetch"
	${SETCOLOR_NORMAL}
	if [ ! -d .git ]; then
		echo "attempting to download libwaveform ..."
		dir=`pwd`
		cd lib/waveform && git clone https://github.com/ayyi/libwaveform.git || exit 1
		if [ ! -f libwaveform/autogen.sh ]; then
			${SETCOLOR_FAILURE}
			echo "failed to download libwaveform submodule"
			${SETCOLOR_NORMAL}
			exit 1
		fi
		mv libwaveform/* libwaveform/.git . && rmdir libwaveform
		cd "$dir"
	else
		git submodule update --init
	fi
fi

cd lib/waveform && \
	./autogen.sh && \
	cd ../..

#test -n "$NOCONFIGURE" || "$srcdir/configure" --enable-maintainer-mode "$@"
