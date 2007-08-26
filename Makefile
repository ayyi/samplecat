
PACKAGE_CFLAGS = `pkg-config gtk+-2.0 --cflags` -I/usr/X11R6/include -I/usr/include/freetype2 `pkg-config libxml --cflags` `pkg-config libart --cflags` -I/usr/include/mysql -I/usr/include/gnome-vfs-2.0 -I./gqview2
PACKAGE_LIBS = -Wall,--export-dynamic `pkg-config gtk+-2.0 --libs` -lgdk-x11-2.0 -latk-1.0 -ldl -lmysqlclient -lgnomevfs-2 -lFLAC 

## eh??
MYSQL = -L/usr/local/lib/mysql

GQVIEW1_DIRTREE_OBJECTS=view_dir_tree.o view_dir_list.o filelist.o 

DIRTREE_HEADERS=gqview_view_dir_tree.h gqview2/gqview.h
##pixbuf_util.h
DIRTREE_OBJECTS=ui_fileops.o ui_tree_edit.o filelist.o layout_util.o pixbuf_util.o

HEADERS = main.h audio.h overview.h support.h pixmaps.h type.h diritem.h dh-link.h tree.h db.h dnd.h cellrenderer_hypertext.h xdgmime.h $(DIRTREE_HEADERS)
#filelist.h view_dir_tree.h view_dir_list.h 

OBJECTS = main.o audio.o overview.o support.o pixmaps.o type.o diritem.o dh-link.o tree.o db.o dnd.o cellrenderer_hypertext.o \
	xdgmime.o xdgmimecache.o xdgmimemagic.o xdgmimealias.o xdgmimeparent.o xdgmimeglob.o xdgmimeint.o \
	gqview_view_dir_tree.o $(DIRTREE_OBJECTS)

all: $(OBJECTS) 
	gcc -Wall $(OBJECTS) -o samplecat -DHAVE_GTK_2_10 $(PACKAGE_CFLAGS) $(PACKAGE_LIBS) $(MYSQL) `pkg-config jack --cflags --libs` `pkg-config sndfile --cflags --libs` `pkg-config libart --cflags --libs`

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

##----------------------------------------------------

#view_dir_list.o: view_dir_list.c $(HEADERS)
#	gcc -Wall view_dir_list.c -c $(PACKAGE_CFLAGS)

#view_dir_tree.o: view_dir_tree.c $(HEADERS)
#	gcc -Wall view_dir_tree.c -c $(PACKAGE_CFLAGS)

gqview_view_dir_tree.o: view_dir_tree.c $(HEADERS)
	gcc -Wall gqview_view_dir_tree.c -c $(PACKAGE_CFLAGS)

ui_fileops.o: gqview2/ui_fileops.c $(HEADERS)
	gcc -Wall gqview2/ui_fileops.c -c $(PACKAGE_CFLAGS)

ui_tree_edit.o: gqview2/ui_tree_edit.c $(HEADERS)
	gcc -Wall gqview2/ui_tree_edit.c -c $(PACKAGE_CFLAGS)

layout_util.o: gqview2/layout_util.c $(HEADERS)
	gcc -Wall gqview2/layout_util.c -c $(PACKAGE_CFLAGS)

filelist.o: gqview2/filelist.c $(HEADERS)
	gcc -Wall gqview2/filelist.c -c $(PACKAGE_CFLAGS)

pixbuf_util.o: gqview2/pixbuf_util.c $(HEADERS)
	gcc -Wall gqview2/pixbuf_util.c -c $(PACKAGE_CFLAGS)

#filelist.o: filelist.c $(HEADERS)
#	gcc -Wall filelist.c -c $(PACKAGE_CFLAGS)

##----------------------------------------------------

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
	if [ -f samplecat ]; then rm samplecat; fi;

install:
	cp -p samplecat /usr/bin/

TMP = ../samplecat-0.0.2

tarball:
	cp -pR ../samplecat $(TMP)
	rm $(TMP)/*.o
	tar cjvvf ../samplecat-0.0.2.tar.bz $(TMP) --exclude *.bak --exclude $(TMP)/samplecat --exclude $(TMP)/*.o --exclude *.svn* --exclude *peak?.c

#tar cjvvf ../samplecat-0.0.2.tar.bz $(TMP) --exclude *.bak --exclude samplecat/samplecat --exclude $(TMP)/*.o --exclude *.svn* --exclude *peak?.c
#tar czvvf ../samplecat-0.0.2.tgz ../samplecat --exclude *.bak --exclude samplelib/samplecat --exclude samplelib/*.o --exclude samplelib/.svn* --exclude *peak?.c
