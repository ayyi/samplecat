
AM_CFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DG_LOG_DOMAIN=\"Samplecat\" \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/waveform \
	$(DBUS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GDK_CFLAGS)

noinst_LTLIBRARIES = libplayer.la

if ENABLE_AYYIDBUS
AUDITIONER_AYYI = auditioner.c
endif

if ENABLE_JACK
AUDITIONER_JACK = jack_player.c jack_player.h ladspa_proc.c ladspa_proc.h
endif

if ENABLE_GPLAYER
AUDITIONER_GPLAYER = gplayer.c
endif

libplayer_la_SOURCES = \
	player.c player.h \
	$(AUDITIONER_AYYI) \
	$(AUDITIONER_JACK) \
	$(AUDITIONER_GPLAYER)

player.vala.c: player.vala.vala
	valac-0.36 -C -H player.vala.h player.vala.vala
