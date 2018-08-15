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
#ifndef __gl_spectrogram_view_h__
#define __gl_spectrogram_view_h__

#include <glib.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_GL_SPECTROGRAM            (gl_spectrogram_get_type ())
#define GL_SPECTROGRAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_GL_SPECTROGRAM, GlSpectrogram))
#define GL_SPECTROGRAM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GL_SPECTROGRAM, GlSpectrogramClass))
#define IS_GL_SPECTROGRAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_GL_SPECTROGRAM))
#define IS_GL_SPECTROGRAM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GL_SPECTROGRAM))
#define GL_SPECTROGRAM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GL_SPECTROGRAM, GlSpectrogramClass))

typedef struct _GlSpectrogram GlSpectrogram;
typedef struct _GlSpectrogramClass GlSpectrogramClass;
typedef struct _GlSpectrogramPrivate GlSpectrogramPrivate;

typedef void (*SpectrogramReady) (gchar* filename, GdkPixbuf* a, void* user_data_, void* user_data);

struct _GlSpectrogram {
    GtkDrawingArea        parent_instance;
    GlSpectrogramPrivate* priv;
};

struct _GlSpectrogramClass {
    GtkDrawingAreaClass parent_class;
};

GType          gl_spectrogram_get_type       (void) G_GNUC_CONST;
GlSpectrogram* gl_spectrogram_new            (void);
GlSpectrogram* gl_spectrogram_construct      (GType);
void           gl_spectrogram_set_gl_context (GdkGLContext*);
void           gl_spectrogram_set_file       (GlSpectrogram*, gchar* filename);

void           get_spectrogram_with_target   (gchar* path, SpectrogramReady, void* on_ready_target, void* user_data);
void           cancel_spectrogram            (gchar* path);

G_END_DECLS

#endif
