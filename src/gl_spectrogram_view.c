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
#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <gtk/gtk.h>
#include <float.h>
#include <math.h>
#include <gdk/gdk.h>
#include <gdk/gdkgl.h>
#include <gtk/gtkgl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <string.h>
#include "agl/utils.h"
#include "agl/shader.h"
#include "gl_spectrogram_view.h"

typedef struct _GlSpectrogram GlSpectrogram;
typedef struct _GlSpectrogramClass GlSpectrogramClass;
typedef struct _GlSpectrogramPrivate GlSpectrogramPrivate;

#define _g_free0(var) (var = (g_free (var), NULL))
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

#ifndef USE_SYSTEM_GTKGLEXT
#undef gdk_gl_drawable_make_current
#define gdk_gl_drawable_make_current(DRAWABLE, CONTEXT) \
	((G_OBJECT_TYPE(DRAWABLE) == GDK_TYPE_GL_PIXMAP) \
		? gdk_gl_pixmap_make_context_current (DRAWABLE, CONTEXT) \
		: gdk_gl_window_make_context_current (DRAWABLE, CONTEXT) \
	)
#endif

typedef void (*SpectrogramReady) (gchar* filename, GdkPixbuf* a, void* user_data_, void* user_data);

struct _GlSpectrogramPrivate {
    AGl*       agl;
    gchar*     _filename;
    GdkPixbuf* pixbuf;
    gboolean   gl_init_done;
    GLuint     Textures[2];
};


static gpointer gl_spectrogram_parent_class = NULL;
static GlSpectrogram* gl_spectrogram_instance = NULL;
static GdkGLContext* gl_spectrogram_glcontext = NULL;

#define GL_SPECTROGRAM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_GL_SPECTROGRAM, GlSpectrogramPrivate))

enum  {
	GL_SPECTROGRAM_DUMMY_PROPERTY
};

#define GL_SPECTROGRAM_near 10.0
#define GL_SPECTROGRAM_far (-100.0)

GlSpectrogram*  gl_spectrogram_new                  (void);
GlSpectrogram*  gl_spectrogram_construct            (GType object_type);
void            gl_spectrogram_set_gl_context       (GdkGLContext* _glcontext);
static void     gl_spectrogram_load_texture         (GlSpectrogram* self);
static gboolean gl_spectrogram_real_configure_event (GtkWidget* base, GdkEventConfigure* event);
static gboolean gl_spectrogram_real_expose_event    (GtkWidget* base, GdkEventExpose* event);
static void     gl_spectrogram_set_projection       (GlSpectrogram* self);
void            gl_spectrogram_set_file             (GlSpectrogram* self, gchar* filename);
static void     gl_spectrogram_real_unrealize       (GtkWidget*);
static void     gl_spectrogram_finalize             (GObject*);


static gpointer
_g_object_ref (gpointer self)
{
	return self ? g_object_ref (self) : NULL;
}


GlSpectrogram*
gl_spectrogram_construct (GType object_type)
{
	GlSpectrogram* self = (GlSpectrogram*) g_object_new (object_type, NULL);
	gtk_widget_add_events ((GtkWidget*) self, (gint) ((GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK) | GDK_POINTER_MOTION_MASK));
	gtk_widget_set_size_request ((GtkWidget*) self, 200, 100);
	GdkGLConfig* glconfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGB | GDK_GL_MODE_DOUBLE);
	gtk_widget_set_gl_capability ((GtkWidget*) self, glconfig, gl_spectrogram_glcontext, TRUE, (gint) GDK_GL_RGBA_TYPE);
	GlSpectrogram* _tmp1_ = _g_object_ref (self);
	_g_object_unref0 (gl_spectrogram_instance);
	gl_spectrogram_instance = _tmp1_;
	_g_object_unref0 (glconfig);

	return self;
}


GlSpectrogram*
gl_spectrogram_new (void) {
	return gl_spectrogram_construct (TYPE_GL_SPECTROGRAM);
}


void
gl_spectrogram_set_gl_context (GdkGLContext* _glcontext)
{
	g_return_if_fail (_glcontext != NULL);

	GdkGLContext* _tmp0_ = _g_object_ref (_glcontext);
	_g_object_unref0 (gl_spectrogram_glcontext);
	gl_spectrogram_glcontext = _tmp0_;
}


static void
gl_spectrogram_load_texture (GlSpectrogram* self)
{
	g_return_if_fail (self);

	GdkGLDrawable* gldrawable = _g_object_ref (gtk_widget_get_gl_drawable ((GtkWidget*) self));
	gboolean _tmp3_ = gdk_gl_drawable_make_current (gldrawable, agl_get_gl_context());
	if (!_tmp3_) {
		g_print ("gl context error!\n");
		_g_object_unref0 (gldrawable);
		return;
	}
	glBindTexture (GL_TEXTURE_2D, self->priv->Textures[0]);
	GdkPixbuf* scaled = gdk_pixbuf_scale_simple (self->priv->pixbuf, 256, 256, GDK_INTERP_BILINEAR);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat) GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint) GL_LINEAR);
	GLint n_colour_components = (GLint)gdk_pixbuf_get_n_channels (scaled);
	gint _tmp7_ = gdk_pixbuf_get_n_channels (scaled);
	GLenum _tmp6_ = 0U;
	if (_tmp7_ == 4) {
		_tmp6_ = GL_RGBA;
	} else {
		_tmp6_ = GL_RGB;
	}
	GLenum format = _tmp6_;
	gint _tmp8_;
	_tmp8_ = gdk_pixbuf_get_width (scaled);
	gint _tmp9_;
	_tmp9_ = gdk_pixbuf_get_height (scaled);
	GLint _tmp11_;
	_tmp11_ = gluBuild2DMipmaps (GL_TEXTURE_2D, n_colour_components, (GLsizei) _tmp8_, (GLsizei) _tmp9_, format, GL_UNSIGNED_BYTE, gdk_pixbuf_get_pixels (scaled));
	if ((gboolean) _tmp11_) {
		g_print ("mipmap generation failed!\n");
	}
	g_object_unref ((GObject*) scaled);

	_g_object_unref0 (gldrawable);
}


static gboolean
gl_spectrogram_real_configure_event (GtkWidget* widget, GdkEventConfigure* event)
{
	GlSpectrogram* self = (GlSpectrogram*)widget;

	GdkGLDrawable* gldrawable = _g_object_ref (gtk_widget_get_gl_drawable (widget));

	if(!gdk_gl_drawable_make_current (gldrawable, agl_get_gl_context())){
		_g_object_unref0 (gldrawable);
		return FALSE;
	}

	glViewport ((GLint) 0, (GLint) 0, (GLsizei) ((GtkWidget*) self)->allocation.width, (GLsizei) ((GtkWidget*) self)->allocation.height);
	if (!self->priv->gl_init_done) {
		glGenTextures ((GLsizei) 1, self->priv->Textures);
		glEnable (GL_TEXTURE_2D);
		glBindTexture (GL_TEXTURE_2D, self->priv->Textures[0]);
		self->priv->gl_init_done = TRUE;
	}
	_g_object_unref0 (gldrawable);
	self->priv->agl = agl_get_instance();

	return TRUE;
}


static gboolean
gl_spectrogram_real_expose_event (GtkWidget* widget, GdkEventExpose* event)
{
	GlSpectrogram* self = (GlSpectrogram*)widget;

	GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable ((GtkWidget*)self);

	gboolean current;
#ifdef USE_SYSTEM_GTKGLEXT
	current = gdk_gl_drawable_make_current (gldrawable, agl_get_gl_context());
#else
	g_return_val_if_fail(G_OBJECT_TYPE(gldrawable) == GDK_TYPE_GL_WINDOW, FALSE);
	current = gdk_gl_window_make_context_current (gldrawable, agl_get_gl_context());
#endif
	if (!current) {
		return FALSE;
	}

	gl_spectrogram_set_projection (self);
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glClear ((GLbitfield) GL_COLOR_BUFFER_BIT);

	double x = 0.0;
	double w = widget->allocation.width;
	double top = widget->allocation.height;
	double botm = 0.0;

	if(self->priv->agl->use_shaders){
		self->priv->agl->shaders.texture->uniform.fg_colour = 0xffffffff;
		agl_use_program((AGlShader*)self->priv->agl->shaders.texture);
	}

	glEnable (GL_TEXTURE_2D);
	glBindTexture (GL_TEXTURE_2D, self->priv->Textures[0]);
	glBegin (GL_QUADS);
	glTexCoord2d (0.0, 0.0);
	glVertex2d (x, top);
	glTexCoord2d (1.0, 0.0);
	glVertex2d (x + w, top);
	glTexCoord2d (1.0, 1.0);
	glVertex2d (x + w, botm);
	glTexCoord2d (0.0, 1.0);
	glVertex2d (x, botm);
	glEnd ();

#ifdef USE_SYSTEM_GTKGLEXT
	if(gdk_gl_drawable_is_double_buffered (gldrawable)){
#else
	if(TRUE){
#endif
		gdk_gl_drawable_swap_buffers (gldrawable);
	} else {
		glFlush ();
	}

	return TRUE;
}


static void
gl_spectrogram_set_projection (GlSpectrogram* self)
{
	g_return_if_fail (self);

	glViewport ((GLint) 0, (GLint) 0, (GLsizei) ((GtkWidget*) self)->allocation.width, (GLsizei) ((GtkWidget*) self)->allocation.height);
	glMatrixMode ((GLenum) GL_PROJECTION);
	glLoadIdentity ();

	double left = 0.0;
	double right = (gdouble) ((GtkWidget*) self)->allocation.width;
	double top = (gdouble) ((GtkWidget*) self)->allocation.height;
	double bottom = 0.0;
	glOrtho (left, right, bottom, top, GL_SPECTROGRAM_near, GL_SPECTROGRAM_far);
}


static void
__lambda0__spectrogram_ready (gchar* filename, GdkPixbuf* pixbuf, void* user_data_, gpointer _self)
{
	GlSpectrogram* self = _self;

	if (pixbuf) {
		if (self->priv->pixbuf) {
			g_object_unref ((GObject*) self->priv->pixbuf);
		}
		self->priv->pixbuf = pixbuf;
		gl_spectrogram_load_texture (self);
		gtk_widget_queue_draw ((GtkWidget*) self);
	}
}


void
gl_spectrogram_set_file (GlSpectrogram* self, gchar* filename)
{
	g_return_if_fail (self);

	_g_free0 (self->priv->_filename);
	self->priv->_filename = g_strdup ((const gchar*) filename);
	cancel_spectrogram (NULL);
	get_spectrogram_with_target (filename, __lambda0__spectrogram_ready, self, NULL);
}


static void
gl_spectrogram_real_unrealize (GtkWidget* base)
{
	GlSpectrogram* self = (GlSpectrogram*)base;
	cancel_spectrogram (NULL);
	GTK_WIDGET_CLASS (gl_spectrogram_parent_class)->unrealize ((GtkWidget*) G_TYPE_CHECK_INSTANCE_CAST (self, GTK_TYPE_DRAWING_AREA, GtkDrawingArea));
}


static void
gl_spectrogram_class_init (GlSpectrogramClass* klass)
{
	gl_spectrogram_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GlSpectrogramPrivate));
	GTK_WIDGET_CLASS (klass)->configure_event = gl_spectrogram_real_configure_event;
	GTK_WIDGET_CLASS (klass)->expose_event = gl_spectrogram_real_expose_event;
	GTK_WIDGET_CLASS (klass)->unrealize = gl_spectrogram_real_unrealize;
	G_OBJECT_CLASS (klass)->finalize = gl_spectrogram_finalize;
}


static void
gl_spectrogram_instance_init (GlSpectrogram* self)
{
	self->priv = GL_SPECTROGRAM_GET_PRIVATE (self);
	self->priv->pixbuf = NULL;
	self->priv->gl_init_done = FALSE;
}


static void
gl_spectrogram_finalize (GObject* obj)
{
	GlSpectrogram* self = GL_SPECTROGRAM (obj);
	cancel_spectrogram (NULL);
	_g_free0 (self->priv->_filename);
	G_OBJECT_CLASS (gl_spectrogram_parent_class)->finalize (obj);
}


GType
gl_spectrogram_get_type (void)
{
	static volatile gsize gl_spectrogram_type_id__volatile = 0;
	if (g_once_init_enter (&gl_spectrogram_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (GlSpectrogramClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gl_spectrogram_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GlSpectrogram), 0, (GInstanceInitFunc) gl_spectrogram_instance_init, NULL };
		GType gl_spectrogram_type_id;
		gl_spectrogram_type_id = g_type_register_static (GTK_TYPE_DRAWING_AREA, "GlSpectrogram", &g_define_type_info, 0);
		g_once_init_leave (&gl_spectrogram_type_id__volatile, gl_spectrogram_type_id);
	}
	return gl_spectrogram_type_id__volatile;
}



