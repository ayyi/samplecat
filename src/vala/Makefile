packages=--pkg config --pkg posix --pkg gtk4 --pkg gmodule-2.0 --pkg debug
vapidir=--vapidir=. --vapidir=../../lib/debug

all:
	valac -C -H spectrogram_widget.h --pkg gtk+-2.0 --pkg gmodule-2.0 spectrogram_widget.vala

spectrogram-area.c: spectrogram-area.vala
	valac -C -H spectrogram-area.h $(packages) --pkg gl \
		../../lib/waveform/lib/agl/agl/agl.vapi  ../../lib/waveform/lib/agl/agl/gtk-area.vapi $(vapidir) \
		spectrogram-area.vala

application.c: application.vala
	valac -C -H application.h $(packages) --pkg samplecat $(vapidir) application.vala

application-samplecat.c: application-samplecat.vala
	valac -C -H application-samplecat.h $(packages) $(vapidir) application-samplecat.vala

model.c: model.vala
	valac -C -H model.h $(packages) --pkg samplecat $(vapidir)
	@sed -i -e "/\#include <gobject\/gvaluecollector.h>/a#include <application.h>" model.c # append

list_store.c: list_store.vala
	valac -C -H list_store.h $(packages) --pkg samplecat $(vapidir) model.vala list_store.vala
	@sed -i -e "/^typedef struct _SamplecatListStore /d" list_store.c
	@sed -i -e "/^typedef struct _SamplecatListStoreClass/d" list_store.c
	@sed -i -e "/^typedef struct _SamplecatListStorePrivate/d" list_store.c
	@sed -i -e "s/^struct _SamplecatListStore /struct _SamplecatListStoreX /" list_store.c
	@sed -i -e "s/^struct _SamplecatListStoreClass /struct _SamplecatListStoreClassX /" list_store.c
	@sed -i -e "s/^struct _SamplecatFilter /struct _SamplecatFilterX /" list_store.h
	@sed -i -e "s/^struct _SamplecatFilterClass /struct _SamplecatFilterClassX /" list_store.h
	@sed -i -e "s/^struct _SamplecatFilters /struct _SamplecatFiltersX /" list_store.h
	@sed -i -e "s/^struct _SamplecatModel /struct _SamplecatModelX /" list_store.h
	@sed -i -e "s/^struct _SamplecatModelClass /struct _SamplecatModelClassX /" list_store.h

#check vala does not add an unneccesary include for glew.h
gl_spectrogram_view.c: gl_spectrogram_view.vala
	valac-0.24 -C -H gl_spectrogram_view.h --vapidir=../../lib/waveform/agl --pkg gtk+-2.0 --pkg gl --pkg glu --pkg gtkglext-1.0 --pkg gmodule-2.0 --pkg agl gl_spectrogram_view.vala

logger.c: logger.vala
	valac -C -H logger.h --pkg config --pkg posix --pkg gdk-2.0 --pkg gmodule-2.0 --pkg debug --vapidir=. --vapidir=../../lib/debug logger.vala

directory: directory.vala
	valac -C -H directory.h --pkg config --pkg gtk+-2.0 --pkg gdk-2.0 --pkg gmodule-2.0 --pkg debug --vapidir=. --vapidir=../../lib/debug directory.vala

inspector: inspector.vala
	valac -C -H inspector.h --pkg config --pkg gtk+-2.0 --pkg gdk-2.0 --pkg gmodule-2.0 --pkg debug --vapidir=. --vapidir=../../lib/debug inspector.vala

search.c: search.vala
	valac -C -H search.h --pkg config --pkg gtk4 --pkg gmodule-2.0 --pkg debug --vapidir=. --vapidir=../../lib/debug search.vala
