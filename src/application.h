/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include "config.h"
#include "samplecat/samplecat.h"
#include "dir-tree/view_dir_tree.h"
#include "samplecat/application.h"

G_BEGIN_DECLS

#define TYPE_APPLICATION            (application_get_type ())
#define APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_APPLICATION, Application))
#define APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_APPLICATION, ApplicationClass))
#define IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_APPLICATION))
#define IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_APPLICATION))
#define APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_APPLICATION, ApplicationClass))

typedef struct _Application         Application;
typedef struct _ApplicationClass    ApplicationClass;
typedef struct _ApplicationPrivate  ApplicationPrivate;

struct _Application
{
   SamplecatApplication parent_instance;
   ApplicationPrivate*  priv;
   bool                 loaded;

   pthread_t            gui_thread;
   bool                 no_gui;

#if 0
   GtkWidget*           msg_panel;
#endif
   GtkWidget*           statusbar;
   GtkWidget*           statusbar2;

#ifdef GTK4_TODO
   GdkColor             fg_colour;
   GdkColor             bg_colour;
   GdkColor             bg_colour_mod1;
   GdkColor             base_colour;
   GdkColor             text_colour;
#endif
};

struct _ApplicationClass
{
   SamplecatApplicationClass  parent_class;
};

#ifndef __main_c__
extern SamplecatApplication* app;
#endif


GType        application_get_type                () G_GNUC_CONST;
Application* application_new                     ();
Application* application_construct               (GType);
void         application_quit                    (Application*);
void         application_set_ready               ();
void         application_search                  ();

GList*       application_get_selection           ();

void         application_play                    (Sample*);
void         application_play_all                ();
void         application_play_path               (const char*);

#ifdef WITH_VALGRIND
void         application_free                    (Application*);
#endif

G_END_DECLS
