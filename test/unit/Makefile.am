
if BUILD_TESTS

bin_PROGRAMS = unit_test

AM_CFLAGS = \
	-I../../src \
	-I../../lib \
	-I../../lib/waveform \
	-I$(top_srcdir)/lib/waveform/gtkglext-1.0 \
	$(DBUS_CFLAGS) \
	$(GTK_CFLAGS)

unit_test_SOURCES = \
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

unit_test_LDADD = \
	$(top_srcdir)/lib/test/libtest.la \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
	$(top_srcdir)/lib/dir_tree/libdirtree.a \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/samplecat/.libs/libsamplecat.a \
	$(top_srcdir)/lib/samplecat/db/libsamplecatdb.a \
	$(GDL_LDFLAGS) \
	$(top_srcdir)/lib/gtk/libgtk.la \
	$(top_srcdir)/src/panels/libpanels.la \
	$(DBUS_LIBS) \
	$(DEBUG_LDFLAGS) \
	$(OPENGL_LDFLAGS) \
	$(GMODULE_LIBS) \
	$(SQLITE_LIBS) \
	$(MYSQL_LDFLAGS) \
	$(JACK_LIBS) \
	$(top_srcdir)/lib/audio_analysis/libaudioanalysis.la \
	$(WAVEFORM_LDFLAGS) \
	$(SNDFILE_LIBS) \
	${SAMPLERATE_LIBS} \
	$(FFTW3_LIBS) \
	$(OPENGL_LDFLAGS) \
	$(GTK_LIBS) \
	$(LIBASS_LDFLAGS) \
	$(X11_LDFLAGS) \
	$(ZLIB_LIBS) \
	$(GMODULE_LIBS) \
	$(FFMPEG_LIBS) \
	$(MATH_LDFLAGS) \
	-lstdc++ \
	-ldl

endif

