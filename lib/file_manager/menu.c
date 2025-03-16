/*
  copyright (C) 2006, Thomas Leonard and others (see Rox-filer changelog for details).
  copyright (C) 2007-2025 Tim Orford <tim@orford.org>

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
#include <gtk/gtk.h>

#include "debug/debug.h"
#include "gtk/menu.h"
#include "file_manager.h"
#include "display.h"
#include "menu.h"


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

static bool       fm__make_subdir_menu (AyyiFilemanager*, GMenuModel*);
#ifdef GTK4_TODO
static void       file_op              (gpointer, FileOp, GtkWidget*);
#endif

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

{">" N_("Enter Path..."),   "slash", mini_buffer, MINI_PATH, NULL},
{">" N_("Shell Command..."),    "<Shift>exclam", mini_buffer, MINI_SHELL, NULL},
{">" N_("Terminal Here"),   "grave", xterm_here, FALSE, NULL},
{">" N_("Switch to Terminal"),  NULL, xterm_here, TRUE, NULL},
*/
};
#endif

#ifdef GTK4_TODO
static MenuDef fm_menu_def[] = {
	{">"},
	{"Cd",              NULL,                          GTK_STOCK_DIRECTORY},
	{"<"},
	{"Filter Files...", G_CALLBACK(mini_buffer),       0, MINI_FILTER},
	{"Refresh",         G_CALLBACK(fm_menu__refresh),  GTK_STOCK_REFRESH},
	{"-"},
};
#else
MenuDef fm_menu_def[] = {
	{"Delete",          "fm.delete",       "window-close-symbolic", FILE_DELETE},
	{"-"},
	{"Go up directory", "fm.go-up-dir",    "go-up-symbolic"},
	{">"},
	{"Cd",              "fm.cd-menu",      "folder-symbolic"},
	{"<"},
	{"Filter Files...", "fm.minibuffer",   "filter_small", MINI_FILTER},
	{"Refresh",         "fm.refresh",      "view-refresh-symbolic"},
	{">"},
	{"Sort",            NULL,              "view-sort-ascending"},
	{"Sort by Name",    "fm.set-sort", 0, SORT_NAME},
	{"Sort by Type",    "fm.set-sort", 0, SORT_TYPE},
	{"Sort by Date",    "fm.set-sort", 0, SORT_DATE},
	{"Sort by Size",    "fm.set-sort", 0, SORT_SIZE},
	{"Sort by Owner",   "fm.set-sort", 0, SORT_OWNER},
	{"Sort by Group",   "fm.set-sort", 0, SORT_GROUP},
	{"Reversed",        "fm.reverse-sort"},
	{"<"},
};
#endif


static void
menu_on_dir_changed (GtkWidget* widget, char* dir, gpointer menu_item)
{
	AyyiFilemanager* fm = file_manager__get();
	dbg(2, "dir=%s name=%s", fm->real_path, dir);
	g_return_if_fail(fm->real_path);

	g_menu_item_set_label (menu_item, fm->real_path);
}


GtkWidget*
fm__make_context_menu ()
{
	AyyiFilemanager* fm = file_manager__get();

	GMenuModel* model = fm->menu.model = (GMenuModel*)g_menu_new ();

	GtkWidget* menu = fm->menu.widget = popover_menu_new_from_model (model);
	gtk_widget_set_parent (menu, fm->window);
	gtk_popover_set_has_arrow (GTK_POPOVER(menu), false);
	gtk_popover_set_position (GTK_POPOVER(menu), GTK_POS_LEFT);

	// Show the current directory name
	// Initially the dir path is not set. It is updated in the dir-changed callback.
	const char* name = fm->real_path ? fm->real_path : "Directory";
	GMenuModel* section = (GMenuModel*)g_menu_new ();
#if 0
	g_menu_append_section (G_MENU(model), name, section);
#else
	GMenuItem* section_item = g_menu_item_new_section (name, section);
	g_menu_append_item (G_MENU(model), section_item);
							// to update later: g_menu_item_set_label (GMenuItem* menu_item, const gchar* label)
#endif

	add_menu_items_from_defn (menu, section, G_N_ELEMENTS(fm_menu_def), fm_menu_def, NULL);

	fm__menu_on_view_change(fm);

	g_signal_connect(file_manager__get(), "dir_changed", G_CALLBACK(menu_on_dir_changed), section_item);

	return menu;
}


/*
 *  Add a sub-menu showing the sub-directories in the current directory.
 */
static bool
fm__make_subdir_menu (AyyiFilemanager* fm, GMenuModel* model)
{
	if (fm->real_path) {

		//TODO Use existing data instead of rescanning the directory.
		//     Why are there no directories in the hashtable?
#if 0
		void temp (gpointer key, gpointer value, gpointer data)
		{
			GPtrArray *array = (GPtrArray *) data;
			g_ptr_array_add(array, value);

			DirItem* item = (DirItem*)value;
			if (IS_DIR(item)) printf("***  %s\n", item->leafname);
			printf("  %s basetype=%i\n", item->leafname, item->base_type);
		}

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
				g_autofree gchar* filename = g_build_filename(fm->real_path, leaf, NULL);
				if (g_file_test(filename, G_FILE_TEST_IS_DIR)) {
					strncpy(escaped, leaf, 255);
					fm__escape_for_menu(escaped);

					items = g_list_append(items, g_strdup(escaped));
				}
			}

			g_dir_close(dir);

			if (items) {
				items = g_list_sort(items, (GCompareFunc)g_ascii_strcasecmp);
				int i = 0;
				for (GList* l=items;l;l=l->next,i++) {
					g_autofree gchar* name = l->data;
					add_icon_menu_item_from_defn (POPOVER_MENU(fm->menu.widget), G_MENU(model), &(MenuDef){name, "fm.go-down-dir", "inode-directory-symbolic", i});
				}
				g_list_free(items);

				return true;
			} else {
				//g_menu_item_set_submenu ( GMenuItem* menu_item, GMenuModel* submenu);
			}
		}
	}
	return false;
}


/*
 *  Allow the application to add additional context-menu items.
 */
void
fm__add_menu_item (GMenuModel* section, GAction* action, char* title)
{
	const gchar* name = g_action_get_name (action);
	g_autofree gchar* detailed_name = g_strdup_printf("app.%s", name);
	g_menu_append (G_MENU(section), title, detailed_name);
}


/*
 *  Append the given menu_item and its subtree to the file_manager context menu.
 *  The filemanager context menu is a public property so this fn can be bypassed if desired.
 */
#ifdef GTK4_TODO
void
fm__add_submenu (GtkWidget* menu_item)
{
	AyyiFilemanager* fm = file_manager__get();

	gtk_container_add(GTK_CONTAINER(fm->menu), menu_item);
	gtk_widget_show(menu_item);
}
#endif


#ifdef PRINT_MENUS
static void
print_submenu (GMenuModel* menu)
{
#if 0
	GHashTable* attributes;
	g_menu_model_get_item_attributes (menu, 0, &attributes);
	if (attributes) {
		dbg(0, "n_attrs=%i", g_hash_table_size(attributes));
	}
#endif

	gint n_items = g_menu_model_get_n_items (menu);

	for (int i=0;i<n_items;i++) {
		printf("%i\n", i);
		g_autoptr(GMenuAttributeIter) attrs = g_menu_model_iterate_item_attributes (menu, i);
		const gchar* name;
		GVariant* value;
		while (g_menu_attribute_iter_get_next (attrs, &name, &value)) {
			printf(" * attr: %s=%s\n", name, g_variant_print(value, true));
		}

		g_autoptr(GMenuLinkIter) iter = g_menu_model_iterate_item_links(menu, i);
		while (g_menu_link_iter_next (iter)) {
			const gchar* name = g_menu_link_iter_get_name (iter);
			GMenuModel* item = g_menu_link_iter_get_value (iter);
			dbg(0, "       link: %s %p", name, item);
		}
	}
}
#endif


void
fm__menu_on_view_change (AyyiFilemanager* fm)
{
	GMenuModel* model = gtk_popover_menu_get_menu_model (GTK_POPOVER_MENU(fm->menu.widget));
	GMenuModel* top = g_menu_model_get_item_link (model, 0, "section");
	gint n = g_menu_model_get_n_items (top);
	for (int j=0;j<n;j++) {
		char* label;
		g_menu_model_get_item_attribute (top, j, G_MENU_ATTRIBUTE_LABEL, "s", &label);
		if (!strcmp(label, "Cd")) {
			g_autoptr(GMenuLinkIter) iter = g_menu_model_iterate_item_links(top, j);
			g_menu_link_iter_next (iter);
			GMenuModel* submenu = g_menu_link_iter_get_value (iter);
			g_menu_remove_all (G_MENU(submenu));
			if (!fm__make_subdir_menu(fm, submenu)) {
				g_autoptr(GMenuLinkIter) iter = g_menu_model_iterate_item_links(top, j);
				while (g_menu_link_iter_next (iter)) {
					const gchar* name = g_menu_link_iter_get_name (iter);
					GMenuModel* item = g_menu_link_iter_get_value (iter);
					dbg(0, "       link: %s %p", name, item);
				}
			}
#ifdef PRINT_MENUS
			print_submenu(top);
			print_submenu(submenu);
#endif

			break;
		}
	}
}


#ifdef GTK4_TODO
static void
target_callback (AyyiFilemanager* fm, ViewIter* iter, gpointer action)
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
file_op (gpointer data, FileOp action, GtkWidget* unused)
{
	AyyiFilemanager* fm = file_manager__get();
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

	ViewIter iter;
	view_get_iter(fm->view, &iter, VIEW_ITER_SELECTED);

	DirItem* item = iter.next(&iter);
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
#endif
