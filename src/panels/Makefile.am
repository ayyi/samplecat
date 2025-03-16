AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"Samplecat\" \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir)/lib/waveform/lib/agl \
	$(GTK_CFLAGS)

noinst_LTLIBRARIES = libpanels.la

libpanels_la_SOURCES = \
	library.c library.h \
	directories.c directories.h \
	files.c \
	inspector.c \
	player.c \
	spectrogram.c \
	search.c \
	tags.c \
	waveform.c

BUILT_SOURCES = \
	register.c

CLEANFILES = \
	register.c

if ENABLE_OPENGL
OPENGL_ITEMS = WAVEFORM
endif
if ENABLE_FFTW3
FFTW3_ITEMS = SPECTROGRAM
endif

ITEMS = LIBRARY SEARCH TAGS FILTERS INSPECTOR DIRECTORIES FILES PLAYER $(OPENGL_ITEMS) $(FFTW3_ITEMS)

register.c: register.c.in Makefile.am ../../config.status
	@echo registering panel types...
	@cp register.c.in __register.c.in && \
		for i in $(ITEMS); do echo "    "$$i && \
			sed -i -e "s/PANEL_TYPE_XX/PANEL_TYPE_$$i,\n\tPANEL_TYPE_XX/" __register.c.in ; \
		done && \
		for i in $(ITEMS); do sed -i -e "s/@REGISTER1/@REGISTER1\nextern GType $$(echo $${i} | tr [A-Z] [a-z])_get_type();/" __register.c.in ; done && \
		for i in $(ITEMS); do sed -i -e "s/@REGISTER2/panels[PANEL_TYPE_$$i].gtype = $$(echo $${i} | tr [A-Z] [a-z])_get_type();\n\t@REGISTER2/" __register.c.in ; done && \
		sed -i -e "/_XX/d" __register.c.in && \
		sed -i -e "s/spectrogram_n/spectrogram_area_n/" __register.c.in && \
		sed -i -e "s/spectrogram_g/spectrogram_area_g/" __register.c.in && \
		sed -i -e "/library_get_type/d" __register.c.in && \
		sed -i -e "/filters_get_type/d" __register.c.in && \
		sed -i -e "/player_get/d" __register.c.in && \
		sed -i -e "/directories_/d" __register.c.in && \
		sed -i -e "/files_/d" __register.c.in && \
		sed -i -e "/waveform_get_type/d" __register.c.in && \
		sed -i -e "/@REGISTER/d" __register.c.in && \
		cp __register.c.in $(@F) && \
		rm __register.c.in


EXTRA_DIST = \
	register.c.in \
	filters.c
