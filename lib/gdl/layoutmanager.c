/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2024-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <graphene-gobject.h>
#include "debug/debug.h"
#include "gdl-dock.h"
#include "layoutmanager.h"

struct _DockLayout
{
  GtkLayoutManager parent_instance;
};

struct _DockLayoutChild
{
  GtkLayoutChild parent_instance;
};

G_DEFINE_TYPE (DockLayoutChild, dock_layout_child, GTK_TYPE_LAYOUT_CHILD)


static void
dock_layout_child_class_init (DockLayoutChildClass* klass)
{
}


static void
dock_layout_child_init (DockLayoutChild* self)
{
}


G_DEFINE_TYPE (DockLayout, dock_layout, GTK_TYPE_LAYOUT_MANAGER)


static void
dock_layout_measure (GtkLayoutManager* layout_manager, GtkWidget* widget, GtkOrientation orientation, int for_size, int* minimum, int* natural, int* minimum_baseline, int* natural_baseline)
{
	if (minimum)
		*minimum = 0;
	if (natural)
		*natural = 0;
}


static void
dock_layout_allocate (GtkLayoutManager* layout_manager, GtkWidget* widget, int width, int height, int baseline)
{
	GtkWidget* main_widget = (GtkWidget*)gdl_dock_get_root (GDL_DOCK (widget));
	if (main_widget && gtk_widget_get_visible (main_widget))
		gtk_widget_size_allocate (main_widget, &(GtkAllocation) { 0, 0, width, height }, -1);

	for (GtkWidget* child = gtk_widget_get_first_child (widget); child != NULL; child = gtk_widget_get_next_sibling (child)) {
		if (child != main_widget) {
			gtk_widget_size_allocate (child, &(GtkAllocation) { 0, 0, width, height }, -1);
		}
	}
}


static void
dock_layout_class_init (DockLayoutClass* klass)
{
	GtkLayoutManagerClass* layout_class = GTK_LAYOUT_MANAGER_CLASS (klass);

	layout_class->layout_child_type = GTK_TYPE_OVERLAY_LAYOUT_CHILD;
	layout_class->measure = dock_layout_measure;
	layout_class->allocate = dock_layout_allocate;
}


static void
dock_layout_init (DockLayout* self)
{
}
