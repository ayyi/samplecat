/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://samplecat.orford.org         |
 | copyright (C) 2025-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "stdint.h"
#include <glib-object.h>
#include "gdl/debug.h"
#include "registry.h"
#include "factory.h"
#include "placeholder.h"

static gpointer dock_placeholder_parent_class = NULL;

static void     dock_placeholder_dispose       (GObject*);
static void     dock_placeholder_finalize      (GObject*);
static void     dock_placeholder_remove_widgets(GdlDockObject*);
static GType    dock_placeholder_get_type_once (void);

static GObject*
dock_placeholder_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (dock_placeholder_parent_class);
	GObject* obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	return obj;
}

static void
dock_placeholder_class_init (DockPlaceholderClass* klass, gpointer klass_data)
{
	dock_placeholder_parent_class = g_type_class_peek_parent (klass);

	G_OBJECT_CLASS (klass)->constructor = dock_placeholder_constructor;
	G_OBJECT_CLASS (klass)->dispose = dock_placeholder_dispose;
	G_OBJECT_CLASS (klass)->finalize = dock_placeholder_finalize;
    GDL_DOCK_OBJECT_CLASS(klass)->remove_widgets = dock_placeholder_remove_widgets;

    gdl_dock_item_class_set_has_grip (GDL_DOCK_ITEM_CLASS(klass), FALSE);
}

static void
dock_placeholder_instance_init (DockPlaceholder* self, gpointer klass)
{
}

static void
dock_placeholder_remove_widgets (GdlDockObject* object)
{
	for (GtkWidget* child = gtk_widget_get_first_child (GTK_WIDGET(object)); child != NULL; child = gtk_widget_get_next_sibling (child)) {
		gtk_widget_unparent(child);
	}

	GDL_DOCK_OBJECT_CLASS (dock_placeholder_parent_class)->remove_widgets (object);
}

static void
dock_placeholder_dispose (GObject* object)
{
	PF;

	dock_placeholder_remove_widgets(GDL_DOCK_OBJECT(object));

    G_OBJECT_CLASS (dock_placeholder_parent_class)->dispose (object);
}

static void
dock_placeholder_finalize (GObject* obj)
{
	PF;

	G_OBJECT_CLASS (dock_placeholder_parent_class)->finalize (obj);
}

static GType
dock_placeholder_get_type_once (void)
{
	static const GTypeInfo g_define_type_info = { sizeof (DockPlaceholderClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) dock_placeholder_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (DockPlaceholder), 0, (GInstanceInitFunc) dock_placeholder_instance_init, NULL };
	GType dock_placeholder_type_id;
	dock_placeholder_type_id = g_type_register_static (gdl_dock_item_get_type (), "DockPlaceholder", &g_define_type_info, 0);
	return dock_placeholder_type_id;
}

GType
dock_placeholder_get_type (void)
{
	static volatile gsize dock_placeholder_type_id__once = 0;
	if (g_once_init_enter ((gsize*)&dock_placeholder_type_id__once)) {
		GType dock_placeholder_type_id;
		dock_placeholder_type_id = dock_placeholder_get_type_once ();
		g_once_init_leave (&dock_placeholder_type_id__once, dock_placeholder_type_id);
	}
	return dock_placeholder_type_id__once;
}

GdlDockItem*
dock_placeholder_fill (DockPlaceholder* placeholder)
{
	Stack builder = {
		.items = {
			{},
			{placeholder->spec},
		},
		.sp = 1,
		.master = GDL_DOCK_MASTER(GDL_DOCK_OBJECT(placeholder)->master),
	};

	return dock_item_factory (&builder);
}
