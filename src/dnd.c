/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2014 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#define __dnd_c__
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "samplecat/support.h"

#include "typedefs.h"
#include "support.h"
#include "application.h"
#include "model.h"
#include "listview.h"
#include "progress_dialog.h"
#include "dnd.h"

static bool listview_item_set_colour(GtkTreePath* path, unsigned colour_index);


void
dnd_setup()
{
	gtk_drag_dest_set(app->window, GTK_DEST_DEFAULT_ALL,
	                  dnd_file_drag_types,          // const GtkTargetEntry *targets,
	                  dnd_file_drag_types_count,    // gint n_targets,
	                  (GdkDragAction)(GDK_ACTION_MOVE | GDK_ACTION_COPY));
	g_signal_connect(G_OBJECT(app->window), "drag-data-received", G_CALLBACK(drag_received), NULL);
}


gint
drag_received(GtkWidget* widget, GdkDragContext* drag_context, gint x, gint y, GtkSelectionData* data, guint info, guint time, gpointer user_data)
{
  //this receives drops for the whole window.

  if(!data || data->length < 0){ perr("no data!\n"); return -1; }

  dbg(1, "%s", data->data);

  if(g_str_has_prefix((char*)data->data, "colour:")){

    if(listview__get_mouseover_row() > -1)
    //if(widget==app->libraryview->widget)
    {
      dbg(1, "treeview!");
    }

    char* colour_string = (char*)data->data + 7;
    unsigned colour_index = atoi(colour_string) ? atoi(colour_string) - 1 : 0;

    //which row are we on?
    GtkTreePath* path;
    GtkTreeIter iter;
    gint tx, treeview_top;
    gdk_window_get_position(app->libraryview->widget->window, &tx, &treeview_top);
    dbg(2, "treeview_top=%i", y);

#ifdef HAVE_GTK_2_12
    gint bx, by;
    gtk_tree_view_convert_widget_to_bin_window_coords(GTK_TREE_VIEW(app->libraryview->widget), x, y - treeview_top, &bx, &by);
    dbg(2, "note: gtk2.12 coords: %dx%d => %dx%d", x,y,bx,by);
#else
    gint by = y - treeview_top - 20;
#endif

    if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(app->libraryview->widget), x, by, &path, NULL, NULL, NULL)){

      gtk_tree_model_get_iter(GTK_TREE_MODEL(app->store), &iter, path);
      gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(app->store), &iter);
      dbg(2, "path=%s y=%i final_y=%i", path_str, y, y - treeview_top);

      listview_item_set_colour(path, colour_index);

      gtk_tree_path_free(path);
    }
    else dbg(0, "path not found.");

    return FALSE;
  }

  if(info == GPOINTER_TO_INT(GDK_SELECTION_TYPE_STRING)) printf(" type=string.\n");

  if(info == GPOINTER_TO_INT(TARGET_URI_LIST)){
    dbg(1, "type=uri_list. len=%i", data->length);
    GList* list = uri_list_to_glist((char*)data->data);
    if(g_list_length(list) < 1) pwarn("drag drop: uri list parsing found no uri's.\n");
    int i = 0;
    ScanResults result = {0,};
    GList* l = list;
#ifdef __APPLE__
    gdk_threads_enter();
#endif
    for(;l;l=l->next){
      char* u = l->data;

      gchar* method_string;
      vfs_get_method_string(u, &method_string);
      dbg(2, "%i: %s method=%s", i, u, method_string);

      if(!strcmp(method_string, "file")){
        //we could probably better use g_filename_from_uri() here
        //http://10.0.0.151/man/glib-2.0/glib-Character-Set-Conversion.html#g-filename-from-uri
        //-or perhaps get_local_path() from rox/src/support.c

        char* uri_unescaped = vfs_unescape_string(u + strlen(method_string) + 1, NULL);

        char* uri = (strstr(uri_unescaped, "///") == uri_unescaped) ? uri_unescaped + 2 : uri_unescaped;

        if(do_progress(0,0)) break;
        if(is_dir(uri)) application_add_dir(uri, &result);
        else application_add_file(uri, &result);

        g_free(uri_unescaped);
      }
      else pwarn("drag drop: unknown format: '%s'. Ignoring.\n", u);
      i++;
    }
    hide_progress();
#ifdef __APPLE__
    gdk_threads_leave();
#endif

    statusbar_print(1, "import complete. %i files added", result.n_added);

    uri_list_free(list);
  }

  return FALSE;
}


gint
drag_motion(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data)
{

  //GdkAtom target;
  //gtk_drag_get_data(widget, drag_context, target, time);
  //printf("  target: %s\n", gdk_atom_name(target));

  return FALSE;
}


static bool
listview_item_set_colour(GtkTreePath* path, unsigned colour_index)
{
	g_return_val_if_fail(path, false);
	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(app->store), &iter, path);

	bool ok;
	Sample* s =  sample_get_by_tree_iter(&iter);
	if((ok = samplecat_model_update_sample (app->model, s, COL_COLOUR, (void*)&colour_index))){
		statusbar_print(1, "colour updated");
	}else{
		statusbar_print(1, "error! colour not updated");
	}
	sample_unref(s);

	return ok;
}


