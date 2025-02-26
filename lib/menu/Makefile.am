AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"Samplecat\" \
	-I$(top_srcdir)/lib \
	$(GTK_CFLAGS)

noinst_LTLIBRARIES = libmenu.la

libmenu_la_SOURCES = \
	popovermenu.c popovermenu.h \
	modelbutton.c modelbutton.h \
	menusectionbox.c menusectionboxprivate.h \
	actionhelper.c actionhelper.h \
	actionobserver.c actionobserverprivate.h \
	actionobservable.c actionobservableprivate.h \
	actionmuxer.c actionmuxerprivate.h \
	menutracker.c menutrackerprivate.h \
	menutrackeritem.c menutrackeritemprivate.h \
	gizmo.c gizmoprivate.h \
	allocatedbitmask.c allocatedbitmaskprivate.h \
	widget.c widgetprivate.h \
	widgetfocus.c widgetfocus.h \
	utils.c utils.h

EXTRA_DIST = \
	gdkarrayimpl.c
