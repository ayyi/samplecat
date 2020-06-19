/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

#include "config.h"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include "debug/debug.h"
#include "support.h"
#include "colour_box.h"
#include "listview.h"
#include "application.h"

static struct {
   GtkWidget*     hbox1;
   GtkWidget*     hbox2;
   GtkWidget*     category;
   GtkWidget*     colour;
} window;

static GtkWidget* tag_selector_new ();


/*
 *  Edit tags and colour metadata
 */
GtkWidget*
tags_new ()
{
	PF;

	GtkWidget* vbox = gtk_vbox_new(NON_HOMOGENOUS, 4);
	window.hbox1 = gtk_hbox_new(NON_HOMOGENOUS, 0);
	window.hbox2 = gtk_hbox_new(HOMOGENOUS, 0);

	window.colour = colour_box_new(window.hbox2);
	window.category = tag_selector_new();

	/*
	void on_allocate (GtkWidget* widget, GtkAllocation* allocation, gpointer user_data)
	{
		if(allocation->width < 300){
			gtk_widget_set_parent (window.colour, window.hbox2);
		}
	}

	g_signal_connect(hbox, "size-allocate", G_CALLBACK(on_allocate), NULL);
	*/

	gtk_box_pack_start(GTK_BOX(vbox), window.hbox1, EXPAND_FALSE, FILL_TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), window.hbox2, EXPAND_FALSE, FILL_FALSE, 0);

	return vbox;
}


/*
 *  Add the selected category to each selected sample.
 */
static void
on_category_set_clicked (GtkComboBox* widget, gpointer user_data)
{
	PF;

	//selected category?
	gchar* category = gtk_combo_box_get_active_text(GTK_COMBO_BOX(window.category));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->libraryview->widget));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, NULL);
	if(!selectionlist){ statusbar_print(1, "no files selected."); return; }

	GtkTreeIter iter;
	for(int i=0;i<g_list_length(selectionlist);i++){
		GtkTreePath* treepath_selection = g_list_nth_data(selectionlist, i);

		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(samplecat.store), &iter, treepath_selection)){
			gchar* fname; gchar* tags;
			int id;
			Sample* sample;
			gtk_tree_model_get(GTK_TREE_MODEL(samplecat.store), &iter, COL_SAMPLEPTR, &sample, COL_NAME, &fname, COL_KEYWORDS, &tags, COL_IDX, &id, -1);
			dbg(1, "id=%i name=%s", id, fname);

			if(!strcmp(category, "no categories")){
				if(samplecat_model_update_sample(samplecat.model, sample, COL_KEYWORDS, "")){
				}
			}else{
				if(!keyword_is_dupe(category, tags)){
					char tags_new[1024];
					snprintf(tags_new, 1024, "%s %s", tags ? tags : "", category);
					g_strstrip(tags_new); // trim

					if(samplecat_model_update_sample(samplecat.model, sample, COL_KEYWORDS, (void*)tags_new)){
						statusbar_print(1, "category set");
					}

				}else{
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
	GtkWidget* combo = gtk_combo_box_entry_new_text();

	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "no categories");
	for (int i=0;i<G_N_ELEMENTS(samplecat.model->categories);i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), samplecat.model->categories[i]);
	}

	gtk_box_pack_start(GTK_BOX(window.hbox1), combo, EXPAND_FALSE, FALSE, 0);

	// "set" button
	GtkWidget* set = gtk_button_new_with_label("Set Tag");
	gtk_box_pack_start(GTK_BOX(window.hbox1), set, EXPAND_FALSE, FALSE, 0);

	g_signal_connect(set, "clicked", G_CALLBACK(on_category_set_clicked), NULL);

	return combo;
}

