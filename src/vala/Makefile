packages=--pkg config --pkg posix --pkg gdk-2.0 --pkg gtk+-2.0 --pkg gmodule-2.0  --pkg debug
vapidir=--vapidir=. --vapidir=../../lib/debug model.vala

all:
	valac -C -H list_store.h $(packages) --pkg samplecat $(vapidir) list_store.vala
	@sed -i -e "/^typedef struct _SamplecatListStore /d" list_store.c
	@sed -i -e "/^typedef struct _SamplecatListStoreClass/d" list_store.c
	@sed -i -e "/^typedef struct _SamplecatListStorePrivate/d" list_store.c
	@sed -i -e "s/^struct _SamplecatListStore /struct _SamplecatListStoreX /" list_store.c
	@sed -i -e "s/^struct _SamplecatListStoreClass /struct _SamplecatListStoreClassX /" list_store.c
	mv list_store.c list_store.h ..
	valac -C -H spectrogram_widget.h --pkg gtk+-2.0 --pkg gmodule-2.0 spectrogram_widget.vala
	mv spectrogram_widget.c spectrogram_widget.h ..

application.c: application.vala
	valac -C -H application.h $(packages) --pkg samplecat $(vapidir) application.vala

model.c: model.vala
	valac -C -H model.h $(packages) --pkg samplecat $(vapidir)
	@sed -i -e "/model.h/d" model.c
	@sed -i -e "/typedef struct _SamplecatModel SamplecatModel/d" model.h
	@sed -i -e "/^typedef struct/d" model.c
	@sed -i -e "s/^struct _/struct x_/" model.c
	mv model.c model.h ..

gl_spectrogram_view.c: gl_spectrogram_view.vala
	valac -C -H gl_spectrogram_view.h --pkg gtk+-2.0 --pkg gl --pkg glu --pkg gtkglext-1.0 --pkg gmodule-2.0 gl_spectrogram_view.vala
	mv gl_spectrogram_view.c gl_spectrogram_view.h ..
