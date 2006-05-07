/*

This software is licensed under the GPL. See accompanying file COPYING.

*/
#define IS_DND_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <libart_lgpl/libart.h>
#include <libgnomevfs/gnome-vfs.h>

#include "mysql/mysql.h"
#include "dh-link.h"
#include "main.h"
#include "support.h"
#include "dnd.h"

extern struct _app app;


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

  if(data == NULL || data->length < 0){ errprintf("drag_received(): no data!\n"); return -1; }

  printf("drag_received()! %s\n", data->data);

  if(g_str_has_prefix(data->data, "colour:")){
    //printf("drag_received(): colour!!\n");

	if(get_mouseover_row() > -1){
	//if(widget==app.view){
      printf("drag_received(): treeview!\n");
	}

	char* colour_string = data->data + 7;
	unsigned colour_index = atoi(colour_string) ? atoi(colour_string) - 1 : 0;

	//which row are we on?
	GtkTreePath* path;
	GtkTreeIter iter;
	gint x, treeview_top;
	gdk_window_get_position(app.view->window, &x, &treeview_top);
	printf("drag_received(): treeview_top=%i\n", y);

	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(app.view), x, y - treeview_top, &path, NULL, NULL, NULL)){

		gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);
		gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(app.store), &iter);
		printf("drag_received() path=%s y=%i final_y=%i\n", path_str, y, y - treeview_top);

		item_set_colour(path, colour_index);

		statusbar_print(1, "colour set");
		gtk_tree_path_free(path);
    }

    return FALSE;
  }

  if(info==GPOINTER_TO_INT(GDK_SELECTION_TYPE_STRING)) printf(" type=string.\n");

  if(info==GPOINTER_TO_INT(TARGET_URI_LIST)){
    printf("drag_received(): type=uri_list. len=%i\n", data->length);
    GList *list = gnome_vfs_uri_list_parse(data->data);
	if(g_list_length(list) < 1) warnprintf("drag_received(): drag drop: uri list parsing found no uri's.\n");
    int i, added_count=0;
    for(i=0;i<g_list_length(list); i++){
      GnomeVFSURI* u = g_list_nth_data(list, i);
      printf("drag_received(): %i: %s\n", i, u->text);

      printf("drag_received(): method=%s\n", u->method_string);

      if(!strcmp(u->method_string, "file")){
        //we could probably better use g_filename_from_uri() here
        //http://10.0.0.151/man/glib-2.0/glib-Character-Set-Conversion.html#g-filename-from-uri
        //-or perhaps get_local_path() from rox/src/support.c

        char* uri_unescaped = gnome_vfs_unescape_string(u->text, NULL);

		if(is_dir(uri_unescaped)) scan_dir(uri_unescaped);
        else if(add_file(u->text)) added_count++;

        g_free(uri_unescaped);
      }
      else warnprintf("drag_received(): drag drop: unknown format: '%s'. Ignoring.\n", u);
    }

	char msg[256];
	snprintf(msg, 256, "%i files added", added_count);
	statusbar_print(1, msg);

    gnome_vfs_uri_list_free(list);
  }

  return FALSE;
}


gint
drag_motion(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y,
                   guint time, gpointer user_data)
{

  printf("data motion!...\n");

  //GdkAtom target;
  //gtk_drag_get_data(widget, drag_context, target, time);
  //printf("  target: %s\n", gdk_atom_name(target));

  return FALSE;
}
