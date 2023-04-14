/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <gtk/gtk.h>
#include "debug/debug.h"

#define UNDERLINE printf("-----------------------------------------------------\n")
#define MAX_DEPTH 10

#ifdef DEBUG
	static void print_children (GtkWidget* widget, int* depth);

	static void print_item (GtkWidget* widget, int* depth)
	{
		char text_content[32] = {0};
		if (GTK_IS_LABEL(widget)) {
			g_strlcpy(text_content, gtk_label_get_text((GtkLabel*)widget), 32);
		}

		char indent[128];
		snprintf(indent, 127, "%%%is%%s %%s %%p %s%%s%s %%s%%s %%s\n", *depth * 3, ayyi_bold, ayyi_white);
		GParamSpec* pspec = g_object_class_find_property (G_OBJECT_GET_CLASS(widget), "long-name");
		char* long_name = NULL;
		if (pspec) {
			g_object_get(widget, "long-name", &long_name, NULL);
		}
		char size[32];
		sprintf(size, "%i x %i", gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
		char visible[16];
		g_strlcpy(visible, gtk_widget_get_visible(widget) ? "" : " NOT VISIBLE", 15);
		if (!strcmp(G_OBJECT_TYPE_NAME(widget), "GtkPanedHandle"))
			printf("%s", ayyi_grey);
		printf(indent, " ", G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(widget)), gtk_widget_get_name(widget), widget, long_name ? long_name : "", size, visible, text_content);
		printf("%s", ayyi_white);
		if (long_name) g_free(long_name);

#ifdef GTK4_TODO
		if (GDL_IS_DOCK_ITEM(widget)) {
			GtkWidget* b = ((GdlDockItem*)widget)->child;
			if (b) {
				(*depth)++;
				print_item(b, depth);
				(*depth)--;
			}
		}
		else
#endif
			if (*depth < MAX_DEPTH) print_children(widget, depth);
	}

	static void print_children (GtkWidget* widget, int* depth)
	{
		(*depth)++;
		GtkWidget* child = gtk_widget_get_first_child(widget);
		if (child) {
			for (; child; child = gtk_widget_get_next_sibling(child)) {
				print_item(child, depth);
			}
		}
		(*depth)--;
		return;

		/*
		if (GTK_IS_CONTAINER(widget)) {
			GList* children = gtk_container_get_children(GTK_CONTAINER(widget));
			for (GList* l = children; l; l = l->next) {
				print_item(l->data, depth);
			}
			g_list_free(children);
		}
#ifdef USE_GDL
		else if (GDL_IS_DOCK_ITEM(widget)) {
			(*depth)++;
			print_item(widget, depth);
			(*depth)--;
		}
#endif
		*/
	}

void
print_widget_tree (GtkWidget* widget)
{
	g_return_if_fail(widget);

	UNDERLINE;

	int depth = 0;
	print_item(widget, &depth);

	UNDERLINE;
}
#endif

