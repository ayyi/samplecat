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

#include "config.h"
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "gdl/gdl-dock-item.h"

/*
 *  Width-first search. Searches imediate descendants before recursing
 */
GtkWidget*
find_widget_by_type_wide (GtkWidget* root, GType type)
{
	GtkWidget* find_in_children (GtkWidget* widget)
	{
		for (GtkWidget* child = gtk_widget_get_first_child(root); child; child = gtk_widget_get_next_sibling(child)) {
			if (G_OBJECT_TYPE(child) == type) {
				return child;
			}
		}

		for (GtkWidget* child = gtk_widget_get_first_child(root); child; child = gtk_widget_get_next_sibling(child)) {
			GtkWidget* target = find_in_children(child);
			if (target) return target;
		}
		return NULL;
	}

	return find_in_children(root);
}


/*
 *  Depth-first search
 */
GtkWidget*
find_widget_by_type_deep (GtkWidget* root, GType type)
{
	GtkWidget* result = NULL;

	void find (GtkWidget* widget, GtkWidget** result)
	{
		GtkWidget* child = gtk_widget_get_first_child (widget);
		for (; child; child = gtk_widget_get_next_sibling (child)) {
			if (G_TYPE_CHECK_INSTANCE_TYPE((child), type)) {
				*result = child;
				break;
			}
			find(child, result);

			if (*result) break;
		}
	}

	find (root, &result);

	return result;
}


#ifdef DEBUG
#define MAX_DEPTH 14
	static void print_children (GtkWidget* widget, int* depth);

	static void print_item (GtkWidget* widget, int* depth)
	{
		char text_content[32] = {0};
		if (GTK_IS_LABEL(widget)) {
			g_strlcpy(text_content, gtk_label_get_text((GtkLabel*)widget), 32);
		}

		char indent[128];
		snprintf(indent, 127, "%%%is%%s %%s %%p %s%%s%s %%s%%s", *depth * 3, ayyi_bold, ayyi_white);
		GParamSpec* pspec = g_object_class_find_property (G_OBJECT_GET_CLASS(widget), "long-name");
		g_autofree char* long_name = NULL;
		if (pspec) {
			g_object_get(widget, "long-name", &long_name, NULL);
		}
		char size[32];
		sprintf(size, "%i x %i", gtk_widget_get_width(widget), gtk_widget_get_height(widget));
		char visible[16];
		g_strlcpy(visible, gtk_widget_get_visible(widget) ? "" : " NOT VISIBLE", 15);
		if (!strcmp(G_OBJECT_TYPE_NAME(widget), "GdlGtkPanedHandle"))
			fputs(ayyi_grey, stdout);
		if (!strcmp(G_OBJECT_TYPE_NAME(widget), "GtkBox"))
			fputs(ayyi_blue, stdout);
		if (!strcmp(G_OBJECT_TYPE_NAME(widget), "GtkLabel"))
			fputs(ayyi_green, stdout);
		if (!strcmp(G_OBJECT_TYPE_NAME(widget), "GtkImage") || !strcmp(G_OBJECT_TYPE_NAME(widget), "GdlDockItemButtonImage"))
			fputs(ayyi_pink, stdout);
		if (!strcmp(G_OBJECT_TYPE_NAME(widget), "GtkBuiltinIcon"))
			fputs(ayyi_pink, stdout);

		if (!gtk_widget_get_width(widget) || !gtk_widget_get_height(widget))
			fputs(ayyi_grey, stdout);

		printf(indent, " ", G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(widget)), gtk_widget_get_name(widget), widget, long_name ? long_name : "", size, visible);

		if (text_content[0]) printf(" %s%s", ayyi_bold, text_content);

		const char* cssname = gtk_widget_class_get_css_name (GTK_WIDGET_GET_CLASS(widget));
		if (cssname) printf(" %s%s", ayyi_yellow, cssname);
		g_auto(GStrv) classes = gtk_widget_get_css_classes (widget);
		int i = 0;
		while (classes[i]) {
			printf(".%s", classes[i]);
			i++;
		}

		printf("%s\n", ayyi_white);

#if 0 // not always linked against libgdl
		if (GDL_IS_DOCK_ITEM(widget)) {
#else
		if (false) {
#endif
			GtkWidget* child = ((GdlDockItem*)widget)->child;
			if (child) {
				(*depth)++;
				print_item(child, depth);
				(*depth)--;
			} else {
				snprintf(indent, 127, "%%%is  %s%%s%s\n", *depth * 3, ayyi_red, ayyi_white);
				printf(indent, " ", "NO CHILD");
			}
		}
		else
			if (*depth < MAX_DEPTH && strcmp(G_OBJECT_TYPE_NAME(widget), "IgnoreMe"))
				print_children(widget, depth);
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
	}

void
print_widget_tree (GtkWidget* widget)
{
	g_return_if_fail(widget);

	underline();

	int depth = 0;
	print_item(widget, &depth);

	underline();
}
#endif

