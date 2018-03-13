/*
  copyright (C) 2006, Thomas Leonard and others (see Rox-filer changelog for details).
  copyright (C) 2007-2017 Tim Orford <tim@orford.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 3
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "debug/debug.h"
#include "support.h"
#include "file_manager.h"
#include "dir.h"
#include "display.h"
#include "minibuffer.h"
#include "menu.h"

//static gint updating_menu = 0;      // Non-zero => ignore activations

typedef enum {
	FILE_COPY_ITEM,
	FILE_RENAME_ITEM,
	FILE_LINK_ITEM,
	FILE_OPEN_FILE,
	FILE_PROPERTIES,
	FILE_RUN_ACTION,
	FILE_SET_ICON,
	FILE_SEND_TO,
	FILE_DELETE,
	FILE_USAGE,
	FILE_CHMOD_ITEMS,
	FILE_FIND,
	FILE_SET_TYPE,
} FileOp;

static void       menu__go_down_dir    (GtkMenuItem*, gpointer);
static void       menu__go_up_dir      (GtkMenuItem*, gpointer);
static void       fm_menu__refresh     (GtkMenuItem*, gpointer);
static GtkWidget* fm__make_subdir_menu (AyyiLibfilemanager*);
static void       fm_menu__set_sort    (GtkMenuItem*, gpointer);
static void       fm_menu__reverse_sort(GtkMenuItem*, gpointer);
static void       mini_buffer          (GtkMenuItem*, gpointer);
static GtkWidget* menu_separator_new   (GtkWidget*);
static void       file_op              (gpointer, FileOp, GtkWidget*);

#define IS_DIR(item) (item->base_type == TYPE_DIRECTORY)

#if 0
static GtkItemFactoryEntry fm_menu_def[] = {
{     N_("Display"),         NULL,    NULL,             0,                   "<Branch>"},
//{">"  N_("Icons View"),      NULL,    view_type,      VIEW_TYPE_COLLECTION, NULL},
//{">"  N_("Icons, With..."),  NULL,    NULL,             0,                   "<Branch>"},
//{">>" N_("Sizes"),           NULL,    set_with,         DETAILS_SIZE,        NULL},
//{">>" N_("Permissions"),     NULL,    set_with,         DETAILS_PERMISSIONS, NULL},
//{">>" N_("Types"),           NULL,    set_with,         DETAILS_TYPE,        NULL},
//{">>" N_("Times"),           NULL,    set_with,         DETAILS_TIMES,       NULL},
//{">"  N_("List View"),       NULL,    view_type,        VIEW_TYPE_DETAILS,   "<StockItem>", ROX_STOCK_SHOW_DETAILS},
//{">",                        NULL,    NULL,             0,                   "<Separator>"},
//{">"  N_("Bigger Icons"),    "equal", change_size,      1,                   "<StockItem>", GTK_STOCK_ZOOM_IN},
//{">"  N_("Smaller Icons"),   "minus", change_size,     -1,                   "<StockItem>", GTK_STOCK_ZOOM_OUT},
//{">"  N_("Automatic"),       NULL,    change_size_auto, 0,                   "<ToggleItem>"},
{">",                        NULL,    NULL,             0,                   "<Separator>"},
{">",                       NULL, NULL, 0, "<Separator>"},
/*
{">" N_("Filter Directories With Files"),   NULL, filter_directories, 0, "<ToggleItem>"},
{">" N_("Show Thumbnails"), NULL, show_thumbs, 0, "<ToggleItem>"},
{N_("File"),                NULL, NULL, 0, "<Branch>"},
{">" N_("Copy..."),         "<Ctrl>C", file_op, FILE_COPY_ITEM, "<StockItem>", GTK_STOCK_COPY},
{">" N_("Rename..."),       NULL, file_op, FILE_RENAME_ITEM, NULL},
{">" N_("Link..."),         NULL, file_op, FILE_LINK_ITEM, NULL},
{">",                       NULL, NULL, 0, "<Separator>"},
{">" N_("Shift Open"),      NULL, file_op, FILE_OPEN_FILE},
{">" N_("Send To..."),      NULL, file_op, FILE_SEND_TO, NULL},
{">",                       NULL, NULL, 0, "<Separator>"},
{">" N_("Set Run Action..."),   "asterisk", file_op, FILE_RUN_ACTION, "<StockItem>", GTK_STOCK_EXECUTE},
{">" N_("Properties"),      "<Ctrl>P", file_op, FILE_PROPERTIES, "<StockItem>", GTK_STOCK_PROPERTIES},
{">" N_("Count"),           NULL, file_op, FILE_USAGE, NULL},
{">" N_("Set Type..."),     NULL, file_op, FILE_SET_TYPE, NULL},
{">" N_("Permissions"),     NULL, file_op, FILE_CHMOD_ITEMS, NULL},
{">",                       NULL, NULL, 0, "<Separator>"},
{">" N_("Find"),        "<Ctrl>F", file_op, FILE_FIND, "<StockItem>", GTK_STOCK_FIND},
{N_("Select"),              NULL, NULL, 0, "<Branch>"},
{">" N_("Select All"),          "<Ctrl>A", select_all, 0, NULL},
{">" N_("Clear Selection"), NULL, clear_selection, 0, NULL},
{">" N_("Invert Selection"),    NULL, invert_selection, 0, NULL},
{">" N_("Select by Name..."),   "period", mini_buffer, MINI_SELECT_BY_NAME, NULL},
{">" N_("Select If..."),    "<Shift>question", mini_buffer, MINI_SELECT_IF, NULL},
{N_("Options..."),      NULL, menu_show_options, 0, "<StockItem>", GTK_STOCK_PREFERENCES},
{N_("New"),         NULL, NULL, 0, "<Branch>"},
{">" N_("Directory"),       NULL, new_directory, 0, NULL},
{">" N_("Blank file"),      NULL, new_file, 0, "<StockItem>", GTK_STOCK_NEW},
{N_("Window"),          NULL, NULL, 0, "<Branch>"},
{">" N_("Parent, New Window"),  NULL, open_parent, 0, "<StockItem>", GTK_STOCK_GO_UP},
{">" N_("Parent, Same Window"), NULL, open_parent_same, 0, NULL},
{">" N_("Home Directory"),  "<Ctrl>Home", home_directory, 0, "<StockItem>", GTK_STOCK_HOME},
{">" N_("Show Bookmarks"),  "<Ctrl>B", show_bookmarks, 0, "<StockItem>", ROX_STOCK_BOOKMARKS},
{">" N_("Follow Symbolic Links"),   NULL, follow_symlinks, 0, NULL},

{">" N_("Close Window"),    "<Ctrl>Q", close_window, 0, "<StockItem>", GTK_STOCK_CLOSE},
{">",               NULL, NULL, 0, "<Separator>"},
{">" N_("Enter Path..."),   "slash", mini_buffer, MINI_PATH, NULL},
{">" N_("Shell Command..."),    "<Shift>exclam", mini_buffer, MINI_SHELL, NULL},
{">" N_("Terminal Here"),   "grave", xterm_here, FALSE, NULL},
{">" N_("Switch to Terminal"),  NULL, xterm_here, TRUE, NULL},
*/
};
#endif

typedef struct
{
	char*     label;
	GCallback callback;
	char*     stock_id;
	int       callback_data;
} MenuDef;

static MenuDef fm_menu_def[] = {
	{"Delete",          G_CALLBACK(file_op),           GTK_STOCK_DELETE, FILE_DELETE},
	{"-"},
	{"Go up directory", G_CALLBACK(menu__go_up_dir),   GTK_STOCK_GO_UP},
	{">"},
	{"Cd",              NULL,                          GTK_STOCK_DIRECTORY},
	{"<"},
	{"Filter Files...", G_CALLBACK(mini_buffer),       0, MINI_FILTER},
	{"Refresh",         G_CALLBACK(fm_menu__refresh),  GTK_STOCK_REFRESH},
	{"-"},
	{">"},
	{"Sort",            NULL,                          GTK_STOCK_SORT_ASCENDING},
	{"Sort by Name",    G_CALLBACK(fm_menu__set_sort), 0, SORT_NAME},
	{"Sort by Type",    G_CALLBACK(fm_menu__set_sort), 0, SORT_TYPE},
	{"Sort by Date",    G_CALLBACK(fm_menu__set_sort), 0, SORT_DATE},
	{"Sort by Size",    G_CALLBACK(fm_menu__set_sort), 0, SORT_SIZE},
	{"Sort by Owner",   G_CALLBACK(fm_menu__set_sort), 0, SORT_OWNER},
	{"Sort by Group",   G_CALLBACK(fm_menu__set_sort), 0, SORT_GROUP},
	{"Reversed",        G_CALLBACK(fm_menu__reverse_sort)},
	{"<"},
};


static void
fm_menu__item_image_from_stock(GtkWidget* menu, GtkWidget* item, char* stock_id)
{
	GtkIconSet* set = gtk_style_lookup_icon_set(gtk_widget_get_style(menu), stock_id);
	GdkPixbuf* pixbuf = gtk_icon_set_render_icon(set, gtk_widget_get_style(menu), GTK_TEXT_DIR_LTR, GTK_STATE_NORMAL, GTK_ICON_SIZE_MENU, menu, NULL);

	GtkWidget* icon = gtk_image_new_from_pixbuf(pixbuf);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), icon);
	g_object_unref(pixbuf);
}


	static void menu_on_dir_changed(GtkWidget* widget, char* dir, gpointer menu_item)
	{
		AyyiLibfilemanager* fm = file_manager__get();
		dbg(2, "dir=%s name=%s", fm->real_path, dir);
		g_return_if_fail(fm->real_path);
		GtkLabel* label = (GtkLabel*)gtk_bin_get_child((GtkBin*)menu_item);
		if(label){
			gtk_label_set_text(label, fm->real_path);
		}
	}
GtkWidget*
fm__make_context_menu()
{
	GtkWidget* menu = gtk_menu_new();

	//show the current directory name
	//FIXME initially the dir path is not set. We need to set it in a callback.
	AyyiLibfilemanager* fm = file_manager__get();
	const char* name = fm->real_path ? fm->real_path : "Directory";
	GtkWidget* title = gtk_menu_item_new_with_label(name);
	gtk_container_add(GTK_CONTAINER(menu), title);

	menu_separator_new(menu);

	GtkWidget* parent = menu;
	GtkWidget* a = NULL;

	int i; for(i=0;i<G_N_ELEMENTS(fm_menu_def);i++){
		MenuDef* item = &fm_menu_def[i];
		switch(item->label[0]){
			case '-':
				menu_separator_new(menu);
				break;
			case '<':
				gtk_menu_item_set_submenu(GTK_MENU_ITEM(a), parent);
				parent = menu;
				break;
			case '>':
				item = &fm_menu_def[++i]; // following item must be the submenu parent item.
				parent = gtk_menu_new();

				GtkWidget* sub = a = gtk_image_menu_item_new_with_label(item->label);
				//GtkWidget* ico = gtk_image_new_from_pixbuf(mime_type_get_pixbuf(inode_directory)); // TODO *****
				//gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(sub), ico);
				if(item->stock_id){
					fm_menu__item_image_from_stock(menu, a, item->stock_id);
				}
				gtk_container_add(GTK_CONTAINER(menu), sub);

				break;
			default:
				;GtkWidget* menu_item = gtk_image_menu_item_new_with_label (item->label);
				gtk_menu_shell_append (GTK_MENU_SHELL(parent), menu_item);
				if(item->stock_id){
					fm_menu__item_image_from_stock(menu, menu_item, item->stock_id);
				}
				if(item->callback) g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(item->callback), GINT_TO_POINTER(item->callback_data));
		}
	}

	fm__menu_on_view_change(menu);

	g_signal_connect(file_manager__get(), "dir_changed", G_CALLBACK(menu_on_dir_changed), title);

	gtk_widget_show_all(menu);
	return menu;
}


#if 0
static void
temp(gpointer key, gpointer value, gpointer data)
{
	GPtrArray *array = (GPtrArray *) data;
	g_ptr_array_add(array, value);

	DirItem* item = (DirItem*)value;
	if (IS_DIR(item)) printf("***  %s\n", item->leafname);
	printf("  %s basetype=%i\n", item->leafname, item->base_type);
}
#endif


/*
 *  Add a sub-menu showing the sub-directories in the current directory.
 */
static GtkWidget*
fm__make_subdir_menu(AyyiLibfilemanager* fm)
{
	GtkWidget* submenu = gtk_menu_new();

	if (fm->real_path) {

		//TODO we should use existing data instead of recanning the directory.
		//     Why are there no directories in the hashtable?
#if 0
		GPtrArray* array = g_ptr_array_new();
		g_hash_table_foreach(filer.directory->known_items, temp, array);
#endif

		GDir* dir = g_dir_open((char*)fm->real_path, 0, NULL);
		const char* leaf;
		char escaped[256];
		if (dir) {
			GList* items = NULL;
			while ((leaf = g_dir_read_name(dir))) {
				if (leaf[0] == '.') continue;
				gchar* filename = g_build_filename(fm->real_path, leaf, NULL);
				if (g_file_test(filename, G_FILE_TEST_IS_DIR)) {
					strncpy(escaped, leaf, 255);
					fm__escape_for_menu(escaped);

					items = g_list_append(items, g_strdup(escaped));
				}
				g_free(filename);
			}

			g_dir_close(dir);

			items = g_list_sort(items, (GCompareFunc)g_ascii_strcasecmp);

			GList* l = items;
			for(;l;l=l->next){
				gchar* name = l->data;
				GtkWidget* item = gtk_image_menu_item_new_with_mnemonic(name);
				GtkWidget* ico = gtk_image_new_from_pixbuf(mime_type_get_pixbuf(inode_directory));
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), ico);
				gtk_container_add(GTK_CONTAINER(submenu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu__go_down_dir), NULL);
				g_free(name);
			}
			g_list_free(items);
		}
	}
	return submenu;
}


/*
 *  For application-level menu additions.
 */
void
fm__add_menu_item(GtkAction* action)
{
	AyyiLibfilemanager* fm = file_manager__get();

	GtkWidget* menu_item = gtk_action_create_menu_item(action);
	gtk_menu_shell_append(GTK_MENU_SHELL(fm->menu), menu_item);
	gtk_widget_show(menu_item);
}


/*
 *  Append the given menu_item and its subtree to the file_manager context menu.
 *  The filemanager context menu is a public property so this fn can be bypassed if desired.
 */
void
fm__add_submenu(GtkWidget* menu_item)
{
	AyyiLibfilemanager* fm = file_manager__get();

	gtk_container_add(GTK_CONTAINER(fm->menu), menu_item);
	gtk_widget_show(menu_item);
}


void
fm__menu_on_view_change(GtkWidget* menu)
{
	GList* items = GTK_MENU_SHELL(menu)->children;
	for(;items;items=items->next){
		GtkMenuItem* item = items->data;
		GtkWidget* submenu = gtk_menu_item_get_submenu(item);
		if(submenu){
			GtkWidget* cd = fm__make_subdir_menu(file_manager__get());
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), cd);
			gtk_widget_show_all(GTK_WIDGET(item));
			break;
		}
	}
}


static void
menu__go_down_dir(GtkMenuItem* menuitem, gpointer user_data)
{
	AyyiLibfilemanager* fm = file_manager__get();

	GList* children = gtk_container_get_children(GTK_CONTAINER(menuitem));
	for (;children;children=children->next) {
		GtkWidget* child = children->data;
		if (GTK_IS_LABEL(child)) {
			const gchar* leaf = gtk_label_get_text((GtkLabel*)child);
			gchar* filename = g_build_filename(fm->real_path, leaf, NULL);

			fm__change_to(file_manager__get(), filename, NULL);

			g_free(filename);
		}
	}
}


static void
menu__go_up_dir(GtkMenuItem* menuitem, gpointer user_data)
{
	fm__change_to_parent(file_manager__get());
}


static void
fm_menu__refresh(GtkMenuItem* menuitem, gpointer user_data)
{
	PF;
	//filer_refresh(&filer);
	file_manager__update_all();
}


static GtkWidget*
menu_separator_new(GtkWidget* container)
{
	GtkWidget* separator = gtk_menu_item_new();
	gtk_widget_set_sensitive(separator, FALSE);
	gtk_container_add(GTK_CONTAINER(container), separator);
	return separator;
}


#if 0
/* Returns TRUE if the keys were installed (first call only) */
gboolean
ensure_filer_menu()
{
	GList           *items;
	guchar          *tmp;
	GtkWidget       *item;

	//if (!filer_keys_need_init) return FALSE;
	//filer_keys_need_init = FALSE;

	GtkItemFactory* item_factory = menu_create(fm_menu_def, sizeof(fm_menu_def) / sizeof(*fm_menu_def), "<filer>", NULL);

	/*
	GET_MENU_ITEM(filer_menu, "filer");
	GET_SMENU_ITEM(filer_file_menu, "filer", "File");
	GET_SSMENU_ITEM(filer_hidden_menu, "filer", "Display", "Show Hidden");
	GET_SSMENU_ITEM(filer_filter_dirs_menu, "filer", "Display", "Filter Directories With Files");
	GET_SSMENU_ITEM(filer_reverse_menu, "filer", "Display", "Reversed");
	GET_SSMENU_ITEM(filer_auto_size_menu, "filer", "Display", "Automatic");
	GET_SSMENU_ITEM(filer_thumb_menu, "filer", "Display", "Show Thumbnails");
	GET_SSMENU_ITEM(item, "filer", "File", "Set Type...");
	filer_set_type = GTK_BIN(item)->child;

	GET_SMENU_ITEM(filer_new_menu, "filer", "New");
	GET_SSMENU_ITEM(item, "filer", "Window", "Follow Symbolic Links");
	filer_follow_sym = GTK_BIN(item)->child;

	// File '' label...
	items = gtk_container_get_children(GTK_CONTAINER(filer_menu));
	filer_file_item = GTK_BIN(g_list_nth(items, 1)->data)->child;
	g_list_free(items);

	// Shift Open... label
	items = gtk_container_get_children(GTK_CONTAINER(filer_file_menu));
	file_shift_item = GTK_BIN(g_list_nth(items, 5)->data)->child;
	g_list_free(items);

	GET_SSMENU_ITEM(item, "filer", "Window", "New Window");
	filer_new_window = GTK_BIN(item)->child;

	g_signal_connect(filer_menu, "unmap_event", G_CALLBACK(menu_closed), NULL);
	g_signal_connect(filer_file_menu, "unmap_event", G_CALLBACK(menu_closed), NULL);
	g_signal_connect(filer_keys, "accel_changed", G_CALLBACK(save_menus), NULL);
	*/

	return TRUE;
}


/* Name is in the form "<panel>" */
GtkItemFactory*
menu_create(GtkItemFactoryEntry *def, int n_entries, const gchar *name, GtkAccelGroup *keys)
{
	if (!keys) {
		keys = gtk_accel_group_new();
		gtk_accel_group_lock(keys);
	}

	GtkItemFactory* item_factory = gtk_item_factory_new(GTK_TYPE_MENU, name, keys);

	GtkItemFactoryEntry* translated = def;//translate_entries(def, n_entries);
	gtk_item_factory_create_items(item_factory, n_entries, translated, NULL);
	//free_translated_entries(translated, n_entries);

	return item_factory;
}
#endif


static void
fm_menu__set_sort (GtkMenuItem* menuitem, gpointer user_data)
{
	//if (updating_menu) return;

	//g_return_if_fail(window_with_focus != NULL);

	display_set_sort_type(file_manager__get(), GPOINTER_TO_INT(user_data), GTK_SORT_ASCENDING);
}


static void
fm_menu__reverse_sort (GtkMenuItem* menuitem, gpointer user_data)
{

	//if (updating_menu) return;

	AyyiLibfilemanager* fm = file_manager__get();

	GtkSortType order = fm->sort_order;
	if (order == GTK_SORT_ASCENDING)
		order = GTK_SORT_DESCENDING;
	else
		order = GTK_SORT_ASCENDING;

	display_set_sort_type(fm, fm->sort_type, order);
}


static void
mini_buffer (GtkMenuItem* menuitem, gpointer user_data)
{
	MiniType type = (MiniType)GPOINTER_TO_INT(user_data);

	AyyiLibfilemanager* fm = file_manager__get();

	// Item needs to remain selected...
	if (type == MINI_SHELL)
		fm->temp_item_selected = FALSE;

	minibuffer_show(fm, type);
}


static void
target_callback(AyyiLibfilemanager* fm, ViewIter* iter, gpointer action)
{
	g_return_if_fail(fm);

//	window_with_focus = filer_window;

	/* Don't grab the primary selection */
	fm->temp_item_selected = TRUE;

	//view_wink_item(fm->view, iter);
	view_select_only(fm->view, iter);
	file_op(NULL, GPOINTER_TO_INT(action), NULL);

	view_clear_selection(fm->view);
	fm->temp_item_selected = FALSE;
}


static void
delete(AyyiLibfilemanager* fm)
{
	GList* paths = fm__selected_items(fm);
	GList* l = paths;
	for(;l;l=l->next){
		dbg(1, "deleting file: %s\n", (char*)l->data);
		g_unlink((char*)l->data);
	}
	destroy_glist(&paths);
}


static void
file_op (gpointer data, FileOp action, GtkWidget* unused)
{
	DirItem	*item;
	ViewIter iter;

	AyyiLibfilemanager* fm = file_manager__get();
	int n_selected = view_count_selected(fm->view);

	if (n_selected < 1)
	{
		const char *prompt;

		switch (action)
		{
			case FILE_COPY_ITEM:
				prompt = "Copy ... ?";
				break;
			case FILE_RENAME_ITEM:
				prompt = "Rename ... ?";
				break;
			case FILE_LINK_ITEM:
				prompt = "Symlink ... ?";
				break;
			case FILE_OPEN_FILE:
				prompt = "Shift Open ... ?";
				break;
			case FILE_PROPERTIES:
				prompt = "Properties of ... ?";
				break;
			case FILE_SET_TYPE:
				prompt = "Set type of ... ?";
				break;
			case FILE_RUN_ACTION:
				prompt = "Set run action for ... ?";
				break;
			case FILE_SET_ICON:
				prompt = "Set icon for ... ?";
				break;
			case FILE_SEND_TO:
				prompt = "Send ... to ... ?";
				break;
			case FILE_DELETE:
				prompt = "DELETE ... ?";
				break;
			case FILE_USAGE:
				prompt = "Count the size of ... ?";
				break;
			case FILE_CHMOD_ITEMS:
				prompt = "Set permissions on ... ?";
				break;
			case FILE_FIND:
				prompt = "Search inside ... ?";
				break;
			default:
				g_warning("Unknown action!");
				return;
		}
		fm__target_mode(fm, target_callback, GINT_TO_POINTER(action), prompt);
		return;
	}

	switch (action)
	{
		case FILE_SEND_TO:
			//send_to(window_with_focus);
			return;
		case FILE_DELETE:
			delete(fm);
			return;
		case FILE_USAGE:
			//usage(window_with_focus);
			return;
		case FILE_CHMOD_ITEMS:
			//chmod_items(window_with_focus);
			return;
		case FILE_SET_TYPE:
			//set_type_items(window_with_focus);
			return;
		case FILE_FIND:
			//find(window_with_focus);
			return;
		case FILE_PROPERTIES:
		{
			//GList* items = filer_selected_items(window_with_focus);
			//infobox_show_list(items);
			//destroy_glist(&items);
			return;
		}
		case FILE_RENAME_ITEM:
			if (n_selected > 1) {
#if 0
				GList* items = NULL;
				ViewIter iter;

				view_get_iter(fm->view, &iter, VIEW_ITER_SELECTED);
				while ((item = iter.next(&iter))){
					items = g_list_prepend(items, item->leafname);
				}
				items = g_list_reverse(items);

				bulk_rename(window_with_focus->sym_path, items);
				g_list_free(items);
#endif
				return;
			}
			break;	// Not a bulk rename... see below
		default:
			break;
	}

	// All the following actions require exactly one file selected

	if (n_selected > 1)
	{
		g_error("You cannot do this to more than one item at a time");
		return;
	}

	view_get_iter(fm->view, &iter, VIEW_ITER_SELECTED);

	item = iter.next(&iter);
	g_return_if_fail(item);
	// iter may be passed to filer_openitem...

	if (item->base_type == TYPE_UNKNOWN)
		item = dir_update_item(fm->directory, item->leafname);

	if (!item) {
		g_error("Item no longer exists!");
		return;
	}

	//const guchar* path = make_path(fm->sym_path, item->leafname);

	switch (action) {
		case FILE_COPY_ITEM:
			//src_dest_action_item(path, di_image(item), _("Copy"), copy_cb, GDK_ACTION_COPY);
			break;
		case FILE_RENAME_ITEM:
			//src_dest_action_item(path, di_image(item), _("Rename"), rename_cb, GDK_ACTION_MOVE);
			break;
		case FILE_LINK_ITEM:
			//src_dest_action_item(path, di_image(item), _("Symlink"), link_cb, GDK_ACTION_LINK);
			break;
		case FILE_OPEN_FILE:
			//filer_openitem(window_with_focus, &iter, OPEN_SAME_WINDOW | OPEN_SHIFT);
			break;
		case FILE_RUN_ACTION:
			//run_action(item);
			break;
		case FILE_SET_ICON:
			//icon_set_handler_dialog(item, path);
			break;
		default:
			g_warning("Unknown action!");
			return;
	}
}

