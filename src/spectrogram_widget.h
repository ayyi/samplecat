/* spectrogram_widget.h generated by valac 0.20.1, the Vala compiler, do not modify */


#ifndef __SPECTROGRAM_WIDGET_H__
#define __SPECTROGRAM_WIDGET_H__

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define TYPE_SPECTROGRAM_WIDGET (spectrogram_widget_get_type ())
#define SPECTROGRAM_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SPECTROGRAM_WIDGET, SpectrogramWidget))
#define SPECTROGRAM_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SPECTROGRAM_WIDGET, SpectrogramWidgetClass))
#define IS_SPECTROGRAM_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SPECTROGRAM_WIDGET))
#define IS_SPECTROGRAM_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SPECTROGRAM_WIDGET))
#define SPECTROGRAM_WIDGET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SPECTROGRAM_WIDGET, SpectrogramWidgetClass))

typedef struct _SpectrogramWidget SpectrogramWidget;
typedef struct _SpectrogramWidgetClass SpectrogramWidgetClass;
typedef struct _SpectrogramWidgetPrivate SpectrogramWidgetPrivate;

typedef void (*RenderDoneFunc) (gchar* filename, GdkPixbuf* a, void* user_data_, void* user_data);
struct _SpectrogramWidget {
	GtkWidget parent_instance;
	SpectrogramWidgetPrivate * priv;
};

struct _SpectrogramWidgetClass {
	GtkWidgetClass parent_class;
};


SpectrogramWidget* spectrogram_widget_new         (void);
SpectrogramWidget* spectrogram_widget_construct   (GType);
void               get_spectrogram_with_target    (gchar* path, RenderDoneFunc, void* on_ready, void* user_data);
void               cancel_spectrogram             (gchar* path);
GType              spectrogram_widget_get_type    (void) G_GNUC_CONST;
void               spectrogram_widget_image_ready (SpectrogramWidget*, gchar* filename, GdkPixbuf*, void* user_data);
void               spectrogram_widget_set_file    (SpectrogramWidget*, gchar* filename);


G_END_DECLS

#endif
