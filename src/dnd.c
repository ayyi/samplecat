/*

This software is licensed under the GPL. See accompanying file COPYING.

*/
#define IS_DND_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#ifdef OLD
  #include <libart_lgpl/libart.h>
#endif
//#include <libgnomevfs/gnome-vfs.h>

#include "mysql/mysql.h"
#include "dh-link.h"
#include "support.h"
#include "typedefs.h"
#include "main.h"
#include "dnd.h"

extern struct _app app;
extern unsigned debug;


void
dnd_setup(){
  gtk_drag_dest_set(app.window, GTK_DEST_DEFAULT_ALL,
                        dnd_file_drag_types,       //const GtkTargetEntry *targets,
                        dnd_file_drag_types_count,    //gint n_targets,
                        (GdkDragAction)(GDK_ACTION_MOVE | GDK_ACTION_COPY));
  g_signal_connect(G_OBJECT(app.window), "drag-data-received", G_CALLBACK(drag_received), NULL);
}


gint
drag_received(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y,
              GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
  //this receives drops for the whole window.

  if(!data || data->length < 0){ errprintf("drag_received(): no data!\n"); return -1; }

  dbg(1, "%s", data->data);

  if(g_str_has_prefix(data->data, "colour:")){
    //printf("drag_received(): colour!!\n");

	if(get_mouseover_row() > -1){
	//if(widget==app.view){
      dbg(1, "treeview!");
	}

	char* colour_string = data->data + 7;
	unsigned colour_index = atoi(colour_string) ? atoi(colour_string) - 1 : 0;

	//which row are we on?
	GtkTreePath* path;
	GtkTreeIter iter;
	gint x, treeview_top;
	gdk_window_get_position(app.view->window, &x, &treeview_top);
	dbg(2, "treeview_top=%i", y);

#ifdef HAVE_GTK_2_12
	dbg(0, "warning: untested gtk2.12 fn.");
	gint bx, by;
	gtk_tree_view_convert_widget_to_bin_window_coords(GTK_TREE_VIEW(app.view), x, y, &bx, &by);
#else
	gint by = y - treeview_top - 20;
#endif

	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(app.view), x, by, &path, NULL, NULL, NULL)){

		gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);
		gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(app.store), &iter);
		dbg(2, "path=%s y=%i final_y=%i", path_str, y, y - treeview_top);

		item_set_colour(path, colour_index);

		statusbar_print(1, "colour set");
		gtk_tree_path_free(path);
    }
	else dbg(0, "path not found.");

    return FALSE;
  }

  if(info==GPOINTER_TO_INT(GDK_SELECTION_TYPE_STRING)) printf(" type=string.\n");

  if(info==GPOINTER_TO_INT(TARGET_URI_LIST)){
    dbg(0, "type=uri_list. len=%i", data->length);
    GList* list = uri_list_to_glist(data->data);
	if(g_list_length(list) < 1) warnprintf("drag_received(): drag drop: uri list parsing found no uri's.\n");
    int i=0, added_count=0;
	GList* l = list;
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

		if(is_dir(uri_unescaped)) scan_dir(uri_unescaped);
        else if(add_file(uri_unescaped)) added_count++;

        g_free(uri_unescaped);
      }
      else warnprintf("drag_received(): drag drop: unknown format: '%s'. Ignoring.\n", u);
      i++;
    }

	char msg[256];
	snprintf(msg, 256, "%i files added", added_count);
	statusbar_print(1, msg);

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
