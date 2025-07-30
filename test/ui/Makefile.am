
if BUILD_TESTS

bin_PROGRAMS = ui_test

AM_CFLAGS = \
	-I../../src \
	-I../../lib \
	-I../../lib/waveform \
	-I../../lib/waveform/lib/agl \
	-I$(top_srcdir)/lib/waveform/gtkglext-1.0 \
	$(DBUS_CFLAGS) \
	$(GTK_CFLAGS)

ui_test_SOURCES = \
	test.c

if ENABLE_OPENGL
if ENABLE_SYSTEM_GTKGLEXT
else
GTKGLEXT_LDFLAGS = \
	$(top_srcdir)/lib/waveform/gtkglext-1.0/.libs/libgtkglext.a
endif
OPENGL_LDFLAGS = \
	$(GTKGLEXT_LDFLAGS) \
	$(GL_LDFLAGS)
endif

if ENABLE_GDL
GDL_LDFLAGS = $(top_srcdir)/lib/gdl/libgdl.a $(XML_LIBS)
endif

if ENABLE_FFTW3
FFTW_LDADD = \
	../../src/spectrogram_widget.o \
	../../src/gl_spectrogram_view.o
endif

if ENABLE_YAML
YAML_LDFLAGS = \
	$(top_srcdir)/lib/yaml/libyaml.a \
	$(YAML_LIBS)
endif

ui_test_LDADD = \
	../../src/application.o \
	../../src/cellrenderer_hypertext.o \
	../../src/colour_box.o \
	../../src/dh_tree.o \
	../../src/dnd.o \
	../../src/editablelabelbutton.o \
	../../src/icon_theme.o \
	../../src/panels/library.o \
	../../src/progress_dialog.o \
	../../src/resources.o \
	../../src/support.o \
	../../src/table.o \
	../../src/window.o \
	$(FFTW_LDADD) \
	$(top_srcdir)/lib/test/libtest.la \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
	$(top_srcdir)/lib/dir_tree/libdirtree.a \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/samplecat/.libs/libsamplecat.a \
	$(top_srcdir)/lib/samplecat/db/libsamplecatdb.a \
	$(GDL_LDFLAGS) \
	$(top_srcdir)/lib/gtk/libgtk.la \
	$(top_srcdir)/src/panels/libpanels.la \
	$(top_srcdir)/lib/utils/libutils.a \
	$(DBUS_LIBS) \
	$(DEBUG_LDFLAGS) \
	$(OPENGL_LDFLAGS) \
	$(GMODULE_LIBS) \
	$(top_srcdir)/lib/audio_analysis/libaudioanalysis.la \
	$(top_srcdir)/lib/waveform/.libs/libwaveformcore.a \
	$(top_srcdir)/lib/waveform/.libs/libwaveformui.a \
	$(top_srcdir)/lib/waveform/lib/agl/.libs/libagl.a \
	$(SQLITE_LIBS) \
	$(MYSQL_LDFLAGS) \
	$(JACK_LIBS) \
	$(SNDFILE_LIBS) \
	${SAMPLERATE_LIBS} \
	$(FFTW3_LIBS) \
	$(OPENGL_LDFLAGS) \
	$(GTK_LIBS) \
	$(GRAPHENE_LIBS) \
	$(X11_LDFLAGS) \
	$(ZLIB_LIBS) \
	$(GMODULE_LIBS) \
	$(FFMPEG_LIBS) \
	$(MATH_LDFLAGS) \
	$(YAML_LDFLAGS) \
	-lstdc++ \
	-ldl

BUILT_SOURCES = list.c

list.c: Makefile test_*.c
	@echo > $@
	@for i in test_*.c; do echo "#include \"$$i\"" >> $@; done
	@echo >> $@
	@echo "gpointer tests[] = {" >> $@
	@for i in test_*.c; do echo "	$${i%.c}," >> $@; done
	@echo "};" >> $@

TESTS = \
	ui_test

EXTRA_DIST = \
	utils.c \
	test_*.c

endif

