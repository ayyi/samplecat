if ENABLE_YAML

AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"Yaml\" \
	-I$(includedir) \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/waveform \
	$(MP3TERM_CFLAGS) \
	$(GLIB_CFLAGS)

noinst_LIBRARIES = libyaml.a

libyaml_a_SOURCES = \
	save.c save.h \
	load.c load.h

endif
