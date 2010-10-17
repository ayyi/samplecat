
all:
	valac -C -H spectrogram_widget.h --pkg gtk+-2.0 --pkg gmodule-2.0 spectrogram_widget.vala
	mv spectrogram_widget.c spectrogram_widget.h ..
	valac -C -H gl_spectrogram_view.h --pkg gtk+-2.0 --pkg gl --pkg glu --pkg gtkglext-1.0 --pkg gmodule-2.0 gl_spectrogram_view.vala
	mv gl_spectrogram_view.c gl_spectrogram_view.h ..
	#valac -C -H dir_model.h --pkg gtk+-2.0 --pkg gmodule-2.0 dir_model.vala
	#mv dir_model.c dir_model.h ..

