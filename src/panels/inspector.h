/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __inspector_h__
#define __inspector_h__

#include <gtk/gtk.h>
#include "typedefs.h"

#define TYPE_INSPECTOR            (inspector_get_type ())
#define INSPECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_INSPECTOR, Inspector))
#define INSPECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_INSPECTOR, InspectorClass))
#define IS_INSPECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_INSPECTOR))
#define IS_INSPECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_INSPECTOR))
#define INSPECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_INSPECTOR, InspectorClass))

typedef struct _Inspector Inspector;
typedef struct _InspectorClass InspectorClass;
typedef struct _InspectorPrivate InspectorPrivate;


struct _Inspector
{
	GtkScrolledWindow parent;
	InspectorPrivate* priv;
	gboolean          show_waveform;
	int               preferred_height;
#ifndef USE_GDL
	int               user_height;
#endif
};

struct _InspectorClass {
	GtkScrolledWindowClass parent_class;
};

GType      inspector_get_type () G_GNUC_CONST;
GtkWidget* inspector_new      ();

#endif
