
PACKAGE_CFLAGS = `pkg-config gtk+-2.0 --cflags` -I/usr/X11R6/include -I/usr/include/freetype2 `pkg-config libxml --cflags` `pkg-config libart --cflags` -I/usr/include/mysql -I/usr/include/gnome-vfs-2.0
PACKAGE_LIBS = -Wall,--export-dynamic `pkg-config gtk+-2.0 --libs` -lgdk-x11-2.0 -latk-1.0 -ldl -lmysqlclient -lgnomevfs-2 
MYSQL = -L/usr/local/lib/mysql

OBJECTS = audio.o overview.o support.o pixmaps.o type.o diritem.o cellrenderer_hypertext.o xdgmime.o xdgmimecache.o xdgmimemagic.o xdgmimealias.o xdgmimeparent.o xdgmimeglob.o xdgmimeint.o

all: $(OBJECTS) 
	gcc -Wall main.c $(OBJECTS) -o samplecat $(PACKAGE_CFLAGS) $(PACKAGE_LIBS) $(MYSQL) `pkg-config jack --cflags --libs` `pkg-config sndfile --cflags --libs` `pkg-config libart --cflags --libs`

audio.o: audio.c audio.h
	gcc -Wall audio.c -c $(PACKAGE_CFLAGS)

overview.o: overview.c overview.h
	gcc -Wall overview.c -c $(PACKAGE_CFLAGS)

support.o: support.c support.h
	gcc -Wall support.c -c $(PACKAGE_CFLAGS)

pixmaps.o: pixmaps.h
	gcc -Wall pixmaps.c -c $(PACKAGE_CFLAGS)

type.o: type.c type.h
	gcc -Wall type.c -c $(PACKAGE_CFLAGS)

diritem.o: diritem.c diritem.h
	gcc -Wall diritem.c -c $(PACKAGE_CFLAGS)

xdgmime.o: xdgmime.h
	gcc -Wall xdgmime.c -c $(PACKAGE_CFLAGS)

xdgmimecache.o: xdgmimecache.c xdgmimecache.h
	gcc -Wall xdgmimecache.c -c $(PACKAGE_CFLAGS)

xdgmimemagic.o: xdgmimemagic.c xdgmimemagic.h
	gcc -Wall xdgmimemagic.c -c $(PACKAGE_CFLAGS)

xdgmimealias.o: xdgmimealias.c xdgmimealias.h
	gcc -Wall xdgmimealias.c -c $(PACKAGE_CFLAGS)

xdgmimeparent.o: xdgmimeparent.c xdgmimeparent.h
	gcc -Wall xdgmimeparent.c -c $(PACKAGE_CFLAGS)

xdgmimeglob.o: xdgmimeglob.c xdgmimeglob.h
	gcc -Wall xdgmimeglob.c -c $(PACKAGE_CFLAGS)

xdgmimeint.o: xdgmimeint.c xdgmimeint.h
	gcc -Wall xdgmimeint.c -c $(PACKAGE_CFLAGS)

cellrenderer_hypertext.o: cellrenderer_hypertext.c cellrenderer_hypertext.h
	gcc -Wall cellrenderer_hypertext.c -c $(PACKAGE_CFLAGS)
