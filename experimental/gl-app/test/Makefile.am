
SUBDIRS = mocks

if BUILD_TESTS

noinst_PROGRAMS = dock dock-h

dock_SOURCES = \
	common.c \
	mocks/application.c mocks/application.h \
	../materials/icon_ring.c \
	../behaviours/style.c \
	../shader.c \
	dock.c

dock_h_SOURCES = \
	dock-h.c

DEPS = \
	../views/libviews.la \
	$(top_srcdir)/lib/waveform/lib/agl/.libs/libagl.a

EXTRA_dock_DEPENDENCIES = $(DEPS)
EXTRA_dock_h_DEPENDENCIES = $(DEPS)

AM_CFLAGS = \
	-fno-common \
	-I.. \
	-I../.. \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/samplecat \
	-I$(top_srcdir)/lib/waveform \
	-I$(top_srcdir)/lib/waveform/lib/agl/gtkglext-1.0 \
	-I$(top_srcdir)/lib/waveform/lib \
	-I$(top_srcdir)/lib/waveform/lib/agl \
	$(OPENGL_CFLAGS) \
	$(GDK_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GRAPHENE_CFLAGS)

if !ENABLE_SYSTEM_GTKGLEXT
LOCAL_GTK_LIBS = \
	$(top_srcdir)/lib/waveform/lib/agl/gtkglext-1.0/.libs/libgtkglext.a
endif

AM_LDFLAGS = \
	$(top_srcdir)/lib/test/.libs/libtest.a \
	$(top_srcdir)/lib/utils/libutils.a \
	$(top_srcdir)/lib/samplecat/.libs/libsamplecat.a \
	$(top_srcdir)/lib/audio_analysis/.libs/libaudioanalysis.a \
	$(top_srcdir)/lib/file_manager/libfilemanager.a \
	$(top_srcdir)/lib/waveform/.libs/libwaveformcore.a \
	$(top_srcdir)/lib/waveform/.libs/libwaveformui.a \
	$(top_srcdir)/lib/waveform/lib/ayyi-utils/.libs/libayyiutils.a \
	$(top_srcdir)/lib/debug/libdebug.a \
	$(top_srcdir)/lib/gtk/libgtk.la \
	../views/libviews.la \
	$(top_srcdir)/lib/waveform/lib/agl/.libs/libagl.a \
	$(LOCAL_GTK_LIBS) \
	$(GRAPHENE_LDFLAGS) \
	$(GMODULE_LDFLAGS) \
	$(OPENGL_LDFLAGS) \
	$(TOOLKIT_LIBS) \
	$(X11_LDFLAGS) \
	$(DBUS_LIBS) \
	$(SNDFILE_LIBS) \
	$(FFMPEG_LIBS) \
	$(XRANDR_LDFLAGS) \
	$(GTK_LIBS) \
	$(GRAPHENE_LIBS) \
	$(GMODULE_LIBS) \
	$(GL_LDFLAGS) \
	-lm -lgio-2.0 -lXtst -lz \
	-lstdc++

dock_h_LDFLAGS = \
	$(AM_LDFLAGS) \
	mocks/libmock.a

AM_TESTS_FD_REDIRECT = 9>&2 --non-interactive

AM_TESTS_ENVIRONMENT = \
	export NON_INTERACTIVE=1;

TESTS = \
	dock-h \
	dock

BUILT_SOURCES = dock-h.h

define build_header =
	echo > $@
	echo '#include "config.h"' >> $@
	echo '#include "test/runner.h"' >> $@
	echo >> $@
	@echo "TestFn x" >> $@
	@cat $(@:.h=.c) | grep ^test_ | sed 's/\(.*\) .*/  ,\1/' >> $@
	@echo "  ;" >> $@
	@echo >> $@
	@echo "gpointer tests[] = {" >> $@
	@cat $(@:.h=.c) | grep ^test_ | sed 's/\(.*\) .*/  \1,/' >> $@
	@echo "};" >> $@
	@echo >> $@
	@echo "int n_tests () { return "`cat $(@:.h=.c) | grep ^test_ | wc -l`"; }" >> $@
endef

dock-h.h: dock-h.c Makefile
	@$(build_header)

endif
