
PACKAGE_CFLAGS = -I/usr/include/gtk-2.0 -I/usr/lib/gtk-2.0/include -I/usr/X11R6/include -I/usr/include/atk-1.0 -I/usr/include/pango-1.0 -I/usr/include/freetype2 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include  
PACKAGE_LIBS = -Wall,--export-dynamic -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangoxft-1.0 -lpangox-1.0 -lpango-1.0 -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0  


all: 
	gcc -Wall main.c support.c audio.c -o samplecat $(PACKAGE_CFLAGS) $(PACKAGE_LIBS) -L/usr/local/lib/mysql -lmysqlclient -I/usr/include/mysql -I/usr/include/gnome-vfs-2.0 -lgnomevfs-2 `pkg-config jack --cflags --libs` `pkg-config sndfile --cflags --libs` `pkg-config libart --cflags --libs`
