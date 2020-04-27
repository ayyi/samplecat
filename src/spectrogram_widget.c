/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2018 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include <glib.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>

#include "audio_analysis/spectrogram/spectrogram.h"


#define TYPE_SPECTROGRAM_WIDGET (spectrogram_widget_get_type ())
#define SPECTROGRAM_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SPECTROGRAM_WIDGET, SpectrogramWidget))
#define SPECTROGRAM_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SPECTROGRAM_WIDGET, SpectrogramWidgetClass))
#define IS_SPECTROGRAM_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SPECTROGRAM_WIDGET))
#define IS_SPECTROGRAM_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SPECTROGRAM_WIDGET))
#define SPECTROGRAM_WIDGET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SPECTROGRAM_WIDGET, SpectrogramWidgetClass))

typedef struct _SpectrogramWidget SpectrogramWidget;
typedef struct _SpectrogramWidgetClass SpectrogramWidgetClass;
typedef struct _SpectrogramWidgetPrivate SpectrogramWidgetPrivate;

#define _g_free0(var) (var = (g_free (var), NULL))
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

typedef void (*RenderDoneFunc) (gchar* filename, GdkPixbuf*, void* user_data_, void* user_data);

struct _SpectrogramWidget {
	GtkWidget parent_instance;
	SpectrogramWidgetPrivate * priv;
};

struct _SpectrogramWidgetClass {
	GtkWidgetClass parent_class;
};

struct _SpectrogramWidgetPrivate {
	gchar* _filename;
	GdkPixbuf* pixbuf;
};


GType spectrogram_widget_get_type (void) G_GNUC_CONST;
G_DEFINE_TYPE_WITH_PRIVATE (SpectrogramWidget, spectrogram_widget, GTK_TYPE_DRAWING_AREA)
enum  {
	SPECTROGRAM_WIDGET_DUMMY_PROPERTY
};

SpectrogramWidget* spectrogram_widget_new                (void);
SpectrogramWidget* spectrogram_widget_construct          (GType);
void               spectrogram_widget_image_ready        (SpectrogramWidget*, gchar* filename, GdkPixbuf*, void*);
void               spectrogram_widget_set_file           (SpectrogramWidget*, gchar* filename);

static void        spectrogram_widget_real_realize       (GtkWidget*);
static void        spectrogram_widget_real_unrealize     (GtkWidget*);
static void        spectrogram_widget_real_size_request  (GtkWidget*, GtkRequisition*);
static void        spectrogram_widget_real_size_allocate (GtkWidget*, GdkRectangle*);
static gboolean    spectrogram_widget_real_expose_event  (GtkWidget*, GdkEventExpose*);

static GObject*    spectrogram_widget_constructor        (GType, guint n_construct_properties, GObjectConstructParam*);
static void        spectrogram_widget_finalize           (GObject*);


void
spectrogram_widget_image_ready (SpectrogramWidget* self, gchar* filename, GdkPixbuf* _pixbuf, void* user_data)
{
	g_return_if_fail (self);

	if (self->priv->pixbuf) {
		g_object_unref ((GObject*)self->priv->pixbuf);
	}
	self->priv->pixbuf = _pixbuf;

	gtk_widget_queue_draw ((GtkWidget*)self);
}


static void
_spectrogram_widget_image_ready_render_done_func (const char* filename, GdkPixbuf* a, gpointer self)
{
	spectrogram_widget_image_ready (self, (char*)filename, a, NULL);
}


void
spectrogram_widget_set_file (SpectrogramWidget* self, gchar* filename)
{
	g_return_if_fail (self);

	gchar* _tmp0_ = filename;
	gchar* _tmp1_ = g_strdup ((const gchar*) _tmp0_);
	_g_free0 (self->priv->_filename);
	self->priv->_filename = _tmp1_;
	GdkPixbuf* _tmp2_ = self->priv->pixbuf;
	if (_tmp2_) {
		gdk_pixbuf_fill (self->priv->pixbuf, (guint32) 0x0);
	}
	cancel_spectrogram (NULL);

	void* p1_target = g_object_ref (self);
	GDestroyNotify p1_target_destroy_notify = g_object_unref;
	get_spectrogram (filename, _spectrogram_widget_image_ready_render_done_func, self);
	(p1_target_destroy_notify == NULL) ? NULL : (p1_target_destroy_notify (p1_target), NULL);
	p1_target = NULL;
	p1_target_destroy_notify = NULL;
}


static void
spectrogram_widget_real_realize (GtkWidget* base)
{
	SpectrogramWidget * self;
	GdkWindowAttr attrs = {0};
	GtkAllocation _tmp0_;
	gint _tmp1_;
	gint _tmp2_ = 0;
	GdkWindow* _tmp3_ = NULL;
	GdkWindowAttr _tmp4_;
	GdkWindow* _tmp5_;
	GdkWindow* _tmp6_;
	GtkStyle* _tmp7_;
	GtkStyle* _tmp8_;
	GdkWindow* _tmp9_;
	GtkStyle* _tmp10_ = NULL;
	GtkStyle* _tmp11_;
	GtkStyle* _tmp12_;
	GdkWindow* _tmp13_;
	GdkWindow* _tmp14_;
	GtkAllocation _tmp15_;
	gint _tmp16_;
	GtkAllocation _tmp17_;
	gint _tmp18_;
	GtkAllocation _tmp19_;
	gint _tmp20_;
	GtkAllocation _tmp21_;
	gint _tmp22_;
	self = (SpectrogramWidget*) base;
	GTK_WIDGET_SET_FLAGS ((GtkWidget*) self, GTK_REALIZED);
	memset (&attrs, 0, sizeof (GdkWindowAttr));
	attrs.window_type = GDK_WINDOW_CHILD;
	_tmp0_ = ((GtkWidget*) self)->allocation;
	_tmp1_ = _tmp0_.width;
	attrs.width = _tmp1_;
	attrs.wclass = GDK_INPUT_OUTPUT;
	_tmp2_ = gtk_widget_get_events ((GtkWidget*) self);
	attrs.event_mask = _tmp2_ | GDK_EXPOSURE_MASK;
	_tmp3_ = gtk_widget_get_parent_window ((GtkWidget*) self);
	_tmp4_ = attrs;
	_tmp5_ = gdk_window_new (_tmp3_, &_tmp4_, 0);
	_g_object_unref0 (((GtkWidget*) self)->window);
	((GtkWidget*) self)->window = _tmp5_;
	_tmp6_ = ((GtkWidget*) self)->window;
	gdk_window_set_user_data (_tmp6_, self);
	_tmp7_ = gtk_widget_get_style ((GtkWidget*) self);
	_tmp8_ = _tmp7_;
	_tmp9_ = ((GtkWidget*) self)->window;
	_tmp10_ = gtk_style_attach (_tmp8_, _tmp9_);
	gtk_widget_set_style ((GtkWidget*) self, _tmp10_);
	_tmp11_ = gtk_widget_get_style ((GtkWidget*) self);
	_tmp12_ = _tmp11_;
	_tmp13_ = ((GtkWidget*) self)->window;
	gtk_style_set_background (_tmp12_, _tmp13_, GTK_STATE_NORMAL);
	_tmp14_ = ((GtkWidget*) self)->window;
	_tmp15_ = ((GtkWidget*) self)->allocation;
	_tmp16_ = _tmp15_.x;
	_tmp17_ = ((GtkWidget*) self)->allocation;
	_tmp18_ = _tmp17_.y;
	_tmp19_ = ((GtkWidget*) self)->allocation;
	_tmp20_ = _tmp19_.width;
	_tmp21_ = ((GtkWidget*) self)->allocation;
	_tmp22_ = _tmp21_.height;
	gdk_window_move_resize (_tmp14_, _tmp16_, _tmp18_, _tmp20_, _tmp22_);
}


static void
spectrogram_widget_real_unrealize (GtkWidget* base)
{
	SpectrogramWidget * self;
	GdkWindow* _tmp0_;
	self = (SpectrogramWidget*) base;
	_tmp0_ = ((GtkWidget*) self)->window;
	gdk_window_set_user_data (_tmp0_, NULL);
}


static void
spectrogram_widget_real_size_request (GtkWidget* base, GtkRequisition* requisition)
{
	SpectrogramWidget * self;
	GtkRequisition _vala_requisition = {0};
	GtkWidgetFlags _tmp0_ = 0;
	GtkWidgetFlags flags;
	GtkWidgetFlags _tmp1_;
	self = (SpectrogramWidget*) base;
	_vala_requisition.width = 10;
	_vala_requisition.height = 80;
	_tmp0_ = GTK_WIDGET_FLAGS ((GtkWidget*) self);
	flags = _tmp0_;
	_tmp1_ = flags;
	if ((_tmp1_ & GTK_REALIZED) != 0) {
	}
	if (requisition) {
		*requisition = _vala_requisition;
	}
}


static void
spectrogram_widget_real_size_allocate (GtkWidget* base, GdkRectangle* allocation)
{
	SpectrogramWidget * self;
	GdkRectangle _tmp0_;
	GtkWidgetFlags _tmp1_ = 0;
	GdkWindow* _tmp2_;
	GtkAllocation _tmp3_;
	gint _tmp4_;
	GtkAllocation _tmp5_;
	gint _tmp6_;
	GtkAllocation _tmp7_;
	gint _tmp8_;
	GtkAllocation _tmp9_;
	gint _tmp10_;
	self = (SpectrogramWidget*) base;
	g_return_if_fail (allocation != NULL);
	_tmp0_ = *allocation;
	((GtkWidget*) self)->allocation = (GtkAllocation) _tmp0_;
	_tmp1_ = GTK_WIDGET_FLAGS ((GtkWidget*) self);
	if ((_tmp1_ & GTK_REALIZED) == 0) {
		return;
	}
	_tmp2_ = ((GtkWidget*) self)->window;
	_tmp3_ = ((GtkWidget*) self)->allocation;
	_tmp4_ = _tmp3_.x;
	_tmp5_ = ((GtkWidget*) self)->allocation;
	_tmp6_ = _tmp5_.y;
	_tmp7_ = ((GtkWidget*) self)->allocation;
	_tmp8_ = _tmp7_.width;
	_tmp9_ = ((GtkWidget*) self)->allocation;
	_tmp10_ = _tmp9_.height;
	gdk_window_move_resize (_tmp2_, _tmp4_, _tmp6_, _tmp8_, _tmp10_);
}


static gboolean
spectrogram_widget_real_expose_event (GtkWidget* base, GdkEventExpose* event)
{
	SpectrogramWidget * self;
	gboolean result = FALSE;
	GdkPixbuf* _tmp0_;
	GtkAllocation _tmp1_;
	gint _tmp2_;
	gint width;
	GtkAllocation _tmp3_;
	gint _tmp4_;
	gint height;
	GdkPixbuf* _tmp5_;
	GtkAllocation _tmp6_;
	gint _tmp7_;
	GtkAllocation _tmp8_;
	gint _tmp9_;
	GdkPixbuf* _tmp10_ = NULL;
	GdkPixbuf* scaled;
	GdkPixbuf* _tmp11_;
	guint8* _tmp12_ = NULL;
	guchar* image;
	GdkWindow* _tmp13_;
	GtkStyle* _tmp14_;
	GtkStyle* _tmp15_;
	GdkGC** _tmp16_;
	GdkGC* _tmp17_;
	gint _tmp18_;
	gint _tmp19_;
	guchar* _tmp20_;
	GdkPixbuf* _tmp21_;
	gint _tmp22_ = 0;
	self = (SpectrogramWidget*) base;
	g_return_val_if_fail (event != NULL, FALSE);
	_tmp0_ = self->priv->pixbuf;
	if (_tmp0_ == NULL) {
		result = TRUE;
		return result;
	}
	_tmp1_ = ((GtkWidget*) self)->allocation;
	_tmp2_ = _tmp1_.width;
	width = _tmp2_;
	_tmp3_ = ((GtkWidget*) self)->allocation;
	_tmp4_ = _tmp3_.height;
	height = _tmp4_;
	_tmp5_ = self->priv->pixbuf;
	_tmp6_ = ((GtkWidget*) self)->allocation;
	_tmp7_ = _tmp6_.width;
	_tmp8_ = ((GtkWidget*) self)->allocation;
	_tmp9_ = _tmp8_.height;
	_tmp10_ = gdk_pixbuf_scale_simple (_tmp5_, _tmp7_, _tmp9_, GDK_INTERP_BILINEAR);
	scaled = _tmp10_;
	_tmp11_ = scaled;
	_tmp12_ = gdk_pixbuf_get_pixels (_tmp11_);
	image = _tmp12_;
	_tmp13_ = ((GtkWidget*) self)->window;
	_tmp14_ = gtk_widget_get_style ((GtkWidget*) self);
	_tmp15_ = _tmp14_;
	_tmp16_ = _tmp15_->fg_gc;
	_tmp17_ = _tmp16_[GTK_STATE_NORMAL];
	_tmp18_ = width;
	_tmp19_ = height;
	_tmp20_ = image;
	_tmp21_ = scaled;
	_tmp22_ = gdk_pixbuf_get_rowstride (_tmp21_);
	gdk_draw_rgb_image ((GdkDrawable*) _tmp13_, _tmp17_, 0, 0, _tmp18_, _tmp19_, GDK_RGB_DITHER_MAX, (guchar*) _tmp20_, _tmp22_);
	result = TRUE;
	return result;
}


SpectrogramWidget*
spectrogram_widget_construct (GType object_type)
{
	SpectrogramWidget * self = NULL;
	self = (SpectrogramWidget*) gtk_widget_new (object_type, NULL);
	return self;
}


SpectrogramWidget*
spectrogram_widget_new (void)
{
	return spectrogram_widget_construct (TYPE_SPECTROGRAM_WIDGET);
}


static GObject*
spectrogram_widget_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (spectrogram_widget_parent_class);
	return parent_class->constructor (type, n_construct_properties, construct_properties);
}


static void
spectrogram_widget_class_init (SpectrogramWidgetClass * klass)
{
	spectrogram_widget_parent_class = g_type_class_peek_parent (klass);

	GTK_WIDGET_CLASS (klass)->realize = spectrogram_widget_real_realize;
	GTK_WIDGET_CLASS (klass)->unrealize = spectrogram_widget_real_unrealize;
	GTK_WIDGET_CLASS (klass)->size_request = spectrogram_widget_real_size_request;
	GTK_WIDGET_CLASS (klass)->size_allocate = spectrogram_widget_real_size_allocate;
	GTK_WIDGET_CLASS (klass)->expose_event = spectrogram_widget_real_expose_event;
	G_OBJECT_CLASS (klass)->constructor = spectrogram_widget_constructor;
	G_OBJECT_CLASS (klass)->finalize = spectrogram_widget_finalize;
}


static void
spectrogram_widget_init (SpectrogramWidget * self)
{
	self->priv = spectrogram_widget_get_instance_private(self);
	self->priv->pixbuf = NULL;
}


static void
spectrogram_widget_finalize (GObject* obj)
{
	SpectrogramWidget * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (obj, TYPE_SPECTROGRAM_WIDGET, SpectrogramWidget);
	_g_free0 (self->priv->_filename);
	G_OBJECT_CLASS (spectrogram_widget_parent_class)->finalize (obj);
}
