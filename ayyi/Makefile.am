AM_CFLAGS = \
	-g -Wall \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DIMAGES_DIR=\"$(imagesdir)\" \
	-I../ \
	-I../gqview2 \
	-DHAVE_GTK_2_10 \
	-DG_LOG_DOMAIN=\"FileMan\" \
	-I$(top_srcdir)/src \
	$(DBUS_CFLAGS) $(GTK_CFLAGS)

noinst_LIBRARIES = libayyi.a

BUILT_SOURCES =

CLEANFILES =

##libfilemanager_headers = \
##	file_manager.h \
##	file_view.h

libayyi_a_SOURCES = \
	ayyi.h \
	ayyi_action.c \
	ayyi_action.h \
	ayyi_client.c \
	ayyi_client.h \
	ayyi_dbus.c \
	ayyi_dbus.h \
	ayyi_list.c \
	ayyi_list.h \
	ayyi_log.c \
	ayyi_log.h \
	ayyi_mixer.c \
	ayyi_mixer.h \
	ayyi_msg.h \
	ayyi_seg0.c \
	ayyi_seg0.h \
	ayyi_shm.c \
	ayyi_shm.h \
	ayyi_song.c \
	ayyi_song.h \
	ayyi_time.c \
	ayyi_time.h \
	ayyi_typedefs.h \
	ayyi_types.h \
	ayyi_utils.c \
	ayyi_utils.h \
	engine_proxy.c \
	engine_proxy.h \
	interface.h

EXTRA_DIST =

