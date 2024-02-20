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
#ifndef __application_h__
#define __application_h__

#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include "agl/actor.h"
#include "waveform/ui-typedefs.h"
#include "samplecat/typedefs.h"
#include "behaviours/style.h"
#include "settings.h"

G_BEGIN_DECLS


#define TYPE_APPLICATION            (application_get_type ())
#define APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_APPLICATION, Application))
#define APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_APPLICATION, ApplicationClass))
#define IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_APPLICATION))
#define IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_APPLICATION))
#define APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_APPLICATION, ApplicationClass))

typedef struct _Application Application;
typedef struct _ApplicationClass ApplicationClass;
typedef struct _ApplicationPrivate ApplicationPrivate;

struct _Application
{
   GObject              parent_instance;
   ApplicationPrivate*  priv;

   ConfigContext        config_ctx;
   Config               config;
   AGlRootActor*        scene;
   WaveformContext*     wfc;
   gboolean             temp_view;
};

struct _ApplicationClass
{
   GObjectClass         parent_class;
};

#ifndef __main_c__
extern Application*     app;
#endif


GType        application_get_type                () G_GNUC_CONST;
Application* application_new                     ();
Application* application_construct               (GType);
void         application_quit                    (Application*);

void         application_menu_init               ();
void         application_add_panel               (AGlActorClass*);
void         application_remove_panel            (AGlActorClass*);

void         application_set_auditioner          ();
void         application_play                    (Sample*);
void         application_play_selected           ();

#define APP_STYLE (*(StyleBehaviour*)((AGlActor*)app->scene)->behaviours[0])

G_END_DECLS

#endif
