/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://www.ayyi.org           |
* | copyright (C) 2020-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __registry_h__
#define __registry_h__

#include "agl/actor.h"

#ifndef __registry_c__
extern GHashTable* registry;
#endif

typedef enum {
	WIDGET_TYPE_GTK_FN,
	WIDGET_TYPE_GTK_OBJECT,
	WIDGET_TYPE_AGL
} WidgetType;

typedef struct {
	WidgetType type;
	union {
		GtkWidget*     (*gtkfn) ();
		GType          gtkobject;
		AGlActorClass* agl;
	}     info;
} RegistryItem;

void registry_init            ();
void agl_actor_register_class (AGlActorClass*);
void register_gtk_fn          (const char*, GtkWidget* (*constructor)());

#endif
