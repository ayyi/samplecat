
all:
	valac -C -H application.h --pkg config --pkg gtk+-2.0 --pkg gmodule-2.0 --pkg samplecat --vapidir=. application.vala
	mv application.c application.h ..
	valac -C -H model.h --pkg config --pkg gtk+-2.0 --pkg gmodule-2.0 --pkg samplecat --vapidir=. model.vala
	sed -i -e "/model.h/d" model.c
	sed -i -e "/typedef struct _SamplecatModel SamplecatModel/d" model.h
	mv model.c model.h ..
	valac -C -H spectrogram_widget.h --pkg gtk+-2.0 --pkg gmodule-2.0 spectrogram_widget.vala
	mv spectrogram_widget.c spectrogram_widget.h ..
	valac -C -H gl_spectrogram_view.h --pkg gtk+-2.0 --pkg gl --pkg glu --pkg gtkglext-1.0 --pkg gmodule-2.0 gl_spectrogram_view.vala
	mv gl_spectrogram_view.c gl_spectrogram_view.h ..

