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
#include <gtk/gtk.h>
#include "samplecat/settings.h"

G_BEGIN_DECLS

#define TYPE_SAMPLECAT_APPLICATION            (samplecat_application_get_type ())
#define SAMPLECAT_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SAMPLECAT_APPLICATION, SamplecatApplication))
#define SAMPLECAT_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SAMPLECAT_APPLICATION, SamplecatApplicationClass))
#define IS_SAMPLECAT_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SAMPLECAT_APPLICATION))
#define IS_SAMPLECAT_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SAMPLECAT_APPLICATION))
#define SAMPLECAT_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SAMPLECAT_APPLICATION, SamplecatApplicationClass))

typedef struct _SamplecatApplication          SamplecatApplication;
typedef struct _SamplecatApplicationClass     SamplecatApplicationClass;
typedef struct _SamplecatApplicationPrivate   SamplecatApplicationPrivate;

struct _SamplecatApplication {
   GtkApplication               parent_instance;
   SamplecatApplicationPrivate* priv;

   struct _args {
      char* add;
      char* layout;
   }                            args;

   enum {
      NONE = 0,
      SCANNING,
   }                            state;

   const char*                  cache_dir;
   ConfigContext                configctx;
   Config                       config;

   GList*                       players;

   bool                         temp_view;
};

struct _SamplecatApplicationClass {
	GtkApplicationClass parent_class;
};

GType                 samplecat_application_get_type  (void) G_GNUC_CONST ;
SamplecatApplication* samplecat_application_new       (void);
SamplecatApplication* samplecat_application_construct (GType);

void                  samplecat_application_scan      (const char* path, ScanResults*);
bool                  samplecat_application_add_file  (const char* path, ScanResults*);
void                  samplecat_application_add_dir   (const char* path, ScanResults*);

G_END_DECLS
