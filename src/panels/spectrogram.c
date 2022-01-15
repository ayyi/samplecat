/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2022 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <math.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include "debug/debug.h"
#include "agl/gtk-area.h"
#include "agl/utils.h"
#include "actors/texture.h"
#include "agl/behaviours/fullsize.h"
#include "audio_analysis/spectrogram/spectrogram.h"
#include "model.h"

#define TYPE_SPECTROGRAM_AREA            (spectrogram_area_get_type ())
#define SPECTROGRAM_AREA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SPECTROGRAM_AREA, SpectrogramArea))
#define SPECTROGRAM_AREA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SPECTROGRAM_AREA, SpectrogramAreaClass))
#define IS_SPECTROGRAM_AREA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SPECTROGRAM_AREA))
#define IS_SPECTROGRAM_AREA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SPECTROGRAM_AREA))
#define SPECTROGRAM_AREA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SPECTROGRAM_AREA, SpectrogramAreaClass))

typedef struct _SpectrogramArea SpectrogramArea;
typedef struct _SpectrogramAreaClass SpectrogramAreaClass;
typedef struct _SpectrogramAreaPrivate SpectrogramAreaPrivate;

enum  {
	SPECTROGRAM_AREA_0_PROPERTY,
	SPECTROGRAM_AREA_NUM_PROPERTIES
};

struct _SpectrogramArea {
	GlArea parent_instance;
	SpectrogramAreaPrivate* priv;
};

struct _SpectrogramAreaClass {
	GlAreaClass parent_class;
};

struct _SpectrogramAreaPrivate {
	struct _agl* agl;
	gchar* _filename;
	GdkPixbuf* pixbuf;
};

static gint SpectrogramArea_private_offset;
static gpointer spectrogram_area_parent_class = NULL;

GType spectrogram_area_get_type (void) G_GNUC_CONST;

static void spectrogram_area_real_unrealize (GtkWidget* base);
static void spectrogram_area_finalize (GObject * obj);
static GType spectrogram_area_get_type_once (void);

static void spectrogram_area_set_file (SpectrogramArea*, gchar*);


static inline gpointer
spectrogram_area_get_instance_private (SpectrogramArea* self)
{
	return G_STRUCT_MEMBER_P (self, SpectrogramArea_private_offset);
}


SpectrogramArea*
spectrogram_area_construct (GType object_type)
{
	SpectrogramArea* self = (SpectrogramArea*) gl_area_construct (object_type);
	GlArea* area = (GlArea*)self;

	AGlActor* node = agl_actor__add_child ((AGlActor*)area->scene, texture_node(NULL));
	node->behaviours[0] = fullsize();

	void spectrogram_area_on_selection_change (SamplecatModel* m, Sample* sample, gpointer self)
	{
		spectrogram_area_set_file ((SpectrogramArea*)self, sample->full_path);
	}

	g_signal_connect((gpointer)samplecat.model, "selection-changed", G_CALLBACK(spectrogram_area_on_selection_change), self);

	return self;
}


SpectrogramArea*
spectrogram_area_new (void)
{
	return spectrogram_area_construct (TYPE_SPECTROGRAM_AREA);
}


static void
spectrogram_area_real_unrealize (GtkWidget* base)
{
	SpectrogramArea* self = (SpectrogramArea*) base;
	GTK_WIDGET_CLASS (spectrogram_area_parent_class)->unrealize ((GtkWidget*) G_TYPE_CHECK_INSTANCE_CAST (self, TYPE_GL_AREA, GlArea));
}


static void
spectrogram_area_class_init (SpectrogramAreaClass* klass, gpointer klass_data)
{
	spectrogram_area_parent_class = g_type_class_peek_parent (klass);
	g_type_class_adjust_private_offset (klass, &SpectrogramArea_private_offset);
	((GtkWidgetClass *) klass)->unrealize = (void (*) (GtkWidget*)) spectrogram_area_real_unrealize;
	G_OBJECT_CLASS (klass)->finalize = spectrogram_area_finalize;
}


static void
spectrogram_area_instance_init (SpectrogramArea* self, gpointer klass)
{
	self->priv = spectrogram_area_get_instance_private (self);
	self->priv->pixbuf = NULL;
}


static void
spectrogram_area_finalize (GObject* obj)
{
	SpectrogramArea* self = G_TYPE_CHECK_INSTANCE_CAST (obj, TYPE_SPECTROGRAM_AREA, SpectrogramArea);
	g_clear_pointer (&self->priv->_filename, g_free);
	G_OBJECT_CLASS (spectrogram_area_parent_class)->finalize (obj);
}


static GType
spectrogram_area_get_type_once ()
{
	static const GTypeInfo g_define_type_info = { sizeof (SpectrogramAreaClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) spectrogram_area_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (SpectrogramArea), 0, (GInstanceInitFunc) spectrogram_area_instance_init, NULL };
	GType spectrogram_area_type_id;
	spectrogram_area_type_id = g_type_register_static (TYPE_GL_AREA, "SpectrogramArea", &g_define_type_info, 0);
	SpectrogramArea_private_offset = g_type_add_instance_private (spectrogram_area_type_id, sizeof (SpectrogramAreaPrivate));
	return spectrogram_area_type_id;
}


GType
spectrogram_area_get_type (void)
{
	static volatile gsize spectrogram_area_type_id__volatile = 0;
	if (g_once_init_enter ((gsize*)&spectrogram_area_type_id__volatile)) {
		GType spectrogram_area_type_id;
		spectrogram_area_type_id = spectrogram_area_get_type_once ();
		g_once_init_leave (&spectrogram_area_type_id__volatile, spectrogram_area_type_id);
	}
	return spectrogram_area_type_id__volatile;
}


static gpointer
_g_object_ref (gpointer self)
{
	return self ? g_object_ref (self) : NULL;
}


static void
spectrogram_area_load_texture (SpectrogramArea* self)
{
	g_return_if_fail (self);

	GlArea* area = (GlArea*)self;
	SpectrogramAreaPrivate* p = self->priv;

	unsigned* texture = &((AGlTextureNode*)((AGlActor*)area->scene)->children->data)->texture;

	if (!texture) {
		glGenTextures ((GLsizei) 1, texture);
		glBindTexture (GL_TEXTURE_2D, *texture);
	}

	GdkGLDrawable* gldrawable = _g_object_ref (gtk_widget_get_gl_drawable ((GtkWidget*) self));
	if (!gdk_gl_drawable_make_current (gldrawable, agl_get_gl_context())) {
		g_print ("gl context error!\n");
		g_clear_pointer (&gldrawable, g_object_unref);
		return;
	}

	agl_use_texture (*texture);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	GLint n_colour_components = (GLint)gdk_pixbuf_get_n_channels(p->pixbuf);
	glTexImage2D (GL_TEXTURE_2D, 0, n_colour_components, gdk_pixbuf_get_width(p->pixbuf), gdk_pixbuf_get_height(p->pixbuf), 0, gdk_pixbuf_get_n_channels (p->pixbuf) == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, gdk_pixbuf_get_pixels(p->pixbuf));

	g_object_unref (gldrawable);
}


static void
__spectrogram_ready (const char* filename, GdkPixbuf* pixbuf, gpointer _self)
{
	SpectrogramArea* self = _self;

	if (pixbuf) {
		if (self->priv->pixbuf) {
			g_object_unref ((GObject*) self->priv->pixbuf);
		}
		self->priv->pixbuf = pixbuf;
		spectrogram_area_load_texture (self);
		gtk_widget_queue_draw ((GtkWidget*) self);
	}
}


static void
spectrogram_area_set_file (SpectrogramArea* self, gchar* filename)
{
	g_return_if_fail (self);

	g_free (self->priv->_filename);
	self->priv->_filename = g_strdup ((const gchar*) filename);
	cancel_spectrogram (NULL);
	get_spectrogram (filename, __spectrogram_ready, self);
}

