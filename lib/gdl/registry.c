/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2020-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#define __registry_c__

#include "config.h"
#include "gtk/gtk.h"
#include "debug/debug.h"
#include "registry.h"

GHashTable* registry; // map className to AGlActorClass


void
dock_registry_init ()
{
	registry = g_hash_table_new(g_str_hash, g_str_equal);
}


void
register_gtk_fn (const char* name, GtkWidget* (*constructor)())
{
	RegistryItem* item = g_new0(RegistryItem, 1);
	item->type = WIDGET_TYPE_GTK_FN;
	item->info.gtkfn = constructor;

	g_hash_table_insert(registry, (char*)name, item);
}


void
agl_actor_register_class (AGlActorClass* class)
{
	static int i = 0;
	class->type = ++i;
	g_hash_table_insert(registry, class->name, class);
}
