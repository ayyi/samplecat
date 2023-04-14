
if BUILD_TESTS

bin_PROGRAMS = unit_test

AM_CFLAGS = \
	-I../../src \
	-I../../lib \
	-I../../lib/waveform \
	-I../../lib/waveform/lib/agl \
	$(DBUS_CFLAGS) \
	$(GTK_CFLAGS)

unit_test_SOURCES = \
	test.c

if ENABLE_OPENGL
OPENGL_LDFLAGS = \
	$(GL_LDFLAGS)
endif

unit_test_LDADD = \
	$(top_srcdir)/lib/test/libtest.la \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/dir_tree/libdirtree.a \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/samplecat/.libs/libsamplecat.a \
	$(top_srcdir)/lib/samplecat/db/libsamplecatdb.a \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
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
	$(top_srcdir)/lib/waveform/.libs/libwaveformcore.a \
	$(top_srcdir)/lib/waveform/.libs/libwaveformui.a \
	$(top_srcdir)/lib/waveform/lib/agl/.libs/libagl.a \
	$(SNDFILE_LIBS) \
	${SAMPLERATE_LIBS} \
	$(FFTW3_LIBS) \
	$(OPENGL_LDFLAGS) \
	$(GTK_LDFLAGS) \
	$(LIBASS_LDFLAGS) \
	$(X11_LDFLAGS) \
	$(GRAPHENE_LIBS) \
	$(ZLIB_LIBS) \
	$(GMODULE_LIBS) \
	$(FFMPEG_LIBS) \
	$(MATH_LDFLAGS) \
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
	unit_test

EXTRA_DIST = \
	utils.c \
	test_*.c

suppression_dir = $(top_srcdir)/test/suppressions

valgrind:
	/usr/bin/valgrind \
		--leak-check=full \
		--gen-suppressions=yes \
		--log-fd=2 \
		--suppressions=$(suppression_dir)/suppressions_ \
		--suppressions=$(suppression_dir)/mesa.supp \
		--suppressions=$(suppression_dir)/glib.supp \
		--suppressions=$(suppression_dir)/pango.supp \
		--suppressions=$(suppression_dir)/fm.supp \
		--num-callers=50 \
		./unit_test

endif

