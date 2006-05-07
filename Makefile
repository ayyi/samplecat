
PACKAGE_CFLAGS = `pkg-config gtk+-2.0 --cflags` -I/usr/X11R6/include -I/usr/include/freetype2 `pkg-config libxml --cflags` `pkg-config libart --cflags` -I/usr/include/mysql -I/usr/include/gnome-vfs-2.0
PACKAGE_LIBS = -Wall,--export-dynamic `pkg-config gtk+-2.0 --libs` -lgdk-x11-2.0 -latk-1.0 -ldl -lmysqlclient -lgnomevfs-2 -lFLAC 
MYSQL = -L/usr/local/lib/mysql

HEADERS = main.h audio.h overview.h support.h pixmaps.h type.h diritem.h dh-link.h tree.h db.h dnd.h cellrenderer_hypertext.h xdgmime.h

OBJECTS = main.o audio.o overview.o support.o pixmaps.o type.o diritem.o dh-link.o tree.o db.o dnd.o cellrenderer_hypertext.o xdgmime.o xdgmimecache.o xdgmimemagic.o xdgmimealias.o xdgmimeparent.o xdgmimeglob.o xdgmimeint.o

all: $(OBJECTS) 
	gcc -Wall $(OBJECTS) -o samplecat $(PACKAGE_CFLAGS) $(PACKAGE_LIBS) $(MYSQL) `pkg-config jack --cflags --libs` `pkg-config sndfile --cflags --libs` `pkg-config libart --cflags --libs`

main.o: main.c $(HEADERS)
	gcc -Wall main.c -c $(PACKAGE_CFLAGS)

audio.o: audio.c $(HEADERS)
	gcc -Wall audio.c -c $(PACKAGE_CFLAGS)

overview.o: overview.c $(HEADERS)
	gcc -Wall overview.c -c $(PACKAGE_CFLAGS)

support.o: support.c $(HEADERS)
	gcc -Wall support.c -c $(PACKAGE_CFLAGS)

pixmaps.o: pixmaps.c $(HEADERS)
	gcc -Wall pixmaps.c -c $(PACKAGE_CFLAGS)

type.o: type.c $(HEADERS)
	gcc -Wall type.c -c $(PACKAGE_CFLAGS)

diritem.o: diritem.c $(HEADERS)
	gcc -Wall diritem.c -c $(PACKAGE_CFLAGS)

dh-link.o: dh-link.c $(HEADERS)
	gcc -Wall dh-link.c -c $(PACKAGE_CFLAGS)

tree.o: tree.c $(HEADERS)
	gcc -Wall tree.c -c $(PACKAGE_CFLAGS)

db.o: db.c $(HEADERS)
	gcc -Wall db.c -c $(PACKAGE_CFLAGS)

dnd.o: dnd.c $(HEADERS)
	gcc -Wall dnd.c -c $(PACKAGE_CFLAGS)

cellrenderer_hypertext.o: cellrenderer_hypertext.c $(HEADERS)
	gcc -Wall cellrenderer_hypertext.c -c $(PACKAGE_CFLAGS)

xdgmime.o: xdgmime.c $(HEADERS)
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

clean:
	rm *.o
	rm samplecat

tarball:
	rm *.o
	rm samplecat
	tar czvvf ../samplecat-0.0.1.tgz ../samplelib
