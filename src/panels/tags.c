/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

/*
 *  Edit tags and colour metadata
 */
#include "config.h"
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "gdl/gdl-dock-item.h"
#include "support.h"
#include "widgets/colour-box.h"
#include "application.h"

#define TYPE_TAGS            (tags_get_type ())
#define TAGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_TAGS, Tags))
#define TAGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_TAGS, TagsClass))
#define IS_TAGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_TAGS))
#define IS_TAGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_TAGS))
#define TAGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_TAGS, TagsClass))

typedef struct _Tags Tags;
typedef struct _TagsClass TagsClass;

struct _Tags {
	GdlDockItem parent_instance;
};

struct _TagsClass {
	GdlDockItemClass parent_class;
};

static gpointer tags_parent_class = NULL;

GType           tags_get_type    (void);
static GObject* tags_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties);

static struct {
   GtkWidget* hbox1;
   GtkWidget* hbox2;
   GtkWidget* category;
   GtkWidget* colour;
} panel;

static GtkWidget* tag_selector_new ();


static void
tags_class_init (TagsClass * klass, gpointer klass_data)
{
	tags_parent_class = g_type_class_peek_parent (klass);

	G_OBJECT_CLASS (klass)->constructor = tags_constructor;
}

static GObject *
tags_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (tags_parent_class);
	GObject* obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	g_object_set(obj, "expand", false, NULL);
	gtk_widget_set_size_request((GtkWidget*)obj, -1, 100);
	return obj;
}

static void
tags_instance_init (Tags* self, gpointer klass)
{
	PF;

	GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gdl_dock_item_set_child(GDL_DOCK_ITEM(self), vbox);

	panel.hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	panel.hbox2 = gtk_flow_box_new();

	panel.colour = colour_box_new(panel.hbox2);
	panel.category = tag_selector_new();

	gtk_box_append(GTK_BOX(vbox), panel.hbox1);
	gtk_box_append(GTK_BOX(vbox), panel.hbox2);
}


/*
 *  Add the selected category to each selected sample.
 */
static void
on_category_set_clicked (GtkComboBox* widget, gpointer user_data)
{
	PF;

	// get the selected category
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	gchar* category = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(panel.category));
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

	GList* selectionlist = application_get_selection();
	if (!selectionlist) { statusbar_print(1, "no files selected."); return; }

	GtkTreeIter iter;
	for (int i=0;i<g_list_length(selectionlist);i++) {
		GtkTreePath* treepath_selection = g_list_nth_data(selectionlist, i);

		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(samplecat.store), &iter, treepath_selection)) {
			gchar* fname;
			gchar* tags;
			int id;
			Sample* sample;
			gtk_tree_model_get(GTK_TREE_MODEL(samplecat.store), &iter, COL_SAMPLEPTR, &sample, COL_NAME, &fname, COL_KEYWORDS, &tags, COL_IDX, &id, -1);
			dbg(1, "id=%i name=%s", id, fname);

			if (!strcmp(category, "no categories")) {
				if (samplecat_model_update_sample(samplecat.model, sample, COL_KEYWORDS, "")) {
				}
			} else {
				if (!keyword_is_dupe(category, tags)) {
					char tags_new[1024];
					snprintf(tags_new, 1024, "%s %s", tags ? tags : "", category);
					g_strstrip(tags_new); // trim

					if (samplecat_model_update_sample(samplecat.model, sample, COL_KEYWORDS, (void*)tags_new)) {
						statusbar_print(1, "category set");
					}
				} else {
					statusbar_print(1, "ignoring duplicate keyword.");
				}
			}

		} else perr("bad iter! i=%i (<%i)\n", i, g_list_length(selectionlist));
	}
	g_list_foreach(selectionlist, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(selectionlist);

	g_free(category);
}


/*
 *  The tag _edit_ selector
 */
static GtkWidget*
tag_selector_new ()
{
	GtkWidget* combo = gtk_combo_box_text_new_with_entry();

	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "no categories");
	for (int i=0;i<G_N_ELEMENTS(samplecat.model->categories);i++) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), samplecat.model->categories[i]);
	}

	gtk_box_append(GTK_BOX(panel.hbox1), combo);

	// "set" button
	GtkWidget* set = gtk_button_new_with_label("Set Tag");
	gtk_box_append(GTK_BOX(panel.hbox1), set);

	g_signal_connect(set, "clicked", G_CALLBACK(on_category_set_clicked), NULL);

	return combo;
}


static GType
tags_get_type_once (void)
{
	static const GTypeInfo g_define_type_info = { sizeof (TagsClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) tags_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (Tags), 0, (GInstanceInitFunc) tags_instance_init, NULL };
	return g_type_register_static (gdl_dock_item_get_type (), "Tags", &g_define_type_info, 0);
}


GType
tags_get_type (void)
{
	static volatile gsize tags_type_id__once = 0;
	if (g_once_init_enter ((gsize*)&tags_type_id__once)) {
		GType tags_type_id = tags_get_type_once ();
		g_once_init_leave (&tags_type_id__once, tags_type_id);
	}
	return tags_type_id__once;
}
