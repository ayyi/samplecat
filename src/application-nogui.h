/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2023-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h> 
#include "samplecat/application.h"

G_BEGIN_DECLS

#define TYPE_NO_GUI_APPLICATION            (no_gui_application_get_type ())
#define NO_GUI_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_NO_GUI_APPLICATION, NoGuiApplication))
#define NO_GUI_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_NO_GUI_APPLICATION, NoGuiApplicationClass))
#define IS_NO_GUI_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_NO_GUI_APPLICATION))
#define IS_NO_GUI_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_NO_GUI_APPLICATION))
#define NO_GUI_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_NO_GUI_APPLICATION, NoGuiApplicationClass))

typedef struct _NoGuiApplication           NoGuiApplication;
typedef struct _NoGuiApplicationClass      NoGuiApplicationClass;
typedef struct _NoGuiApplicationPrivate    NoGuiApplicationPrivate;

struct _NoGuiApplication {
	SamplecatApplication     parent_instance;
	NoGuiApplicationPrivate* priv;
};

struct _NoGuiApplicationClass {
	SamplecatApplicationClass parent_class;
};

GType             no_gui_application_get_type  (void) G_GNUC_CONST ;
NoGuiApplication* no_gui_application_new       (void);
NoGuiApplication* no_gui_application_construct (GType);

G_END_DECLS
