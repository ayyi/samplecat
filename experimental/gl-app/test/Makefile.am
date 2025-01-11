
if BUILD_TESTS

noinst_PROGRAMS = dock

dock_SOURCES = \
	common.c \
	mocks/application.c mocks/application.h \
	../materials/icon_ring.c \
	../behaviours/style.c \
	../shader.c \
	dock.c

DEPS = \
	$(top_srcdir)/lib/waveform/lib/agl/.libs/libagl.a

EXTRA_dock_DEPENDENCIES = $(DEPS)

AM_CFLAGS = \
	-fno-common \
	-I.. \
	-I../.. \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir)/lib/waveform/gtkglext-1.0 \
	-I$(top_srcdir)/lib/waveform/lib/agl \
	$(OPENGL_CFLAGS) \
	$(GDK_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GRAPHENE_CFLAGS)

AM_LDFLAGS = \
	$(top_srcdir)/lib/test/.libs/libtest.a \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/samplecat/.libs/libsamplecat.a \
	$(top_srcdir)/lib/audio_analysis/.libs/libaudioanalysis.a \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
	$(top_srcdir)/lib/waveform/.libs/libwaveformcore.a \
	$(top_srcdir)/lib/waveform/.libs/libwaveformui.a \
	$(top_srcdir)/lib/waveform/lib/agl/.libs/libagl.a \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/gtk/libgtk.la \
	../views/libviews.la \
	$(GRAPHENE_LDFLAGS) \
	$(GMODULE_LDFLAGS) \
	$(OPENGL_LDFLAGS) \
	$(TOOLKIT_LIBS) \
	$(X11_LDFLAGS) \
	$(DBUS_LIBS) \
	$(SNDFILE_LIBS) \
	$(FFMPEG_LIBS) \
	$(XRANDR_LDFLAGS) \
	$(GTK_LDFLAGS) \
	$(GRAPHENE_LIBS) \
	$(GMODULE_LIBS) \
	$(GL_LDFLAGS) \
	-lm -lgio-2.0 -lXtst -lz \
	-lstdc++

AM_TESTS_FD_REDIRECT = 9>&2 --non-interactive

AM_TESTS_ENVIRONMENT = \
	export NON_INTERACTIVE=1;

TESTS = \
	dock

endif
