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

#include "support.h"
#include "rox/rox_global.h"
#include "rox/dir.h"
#include "file_manager.h"
#include "rox/display.h"
#include "mimetype.h"
#include "menu.h"

extern Filer filer;

//static gint updating_menu = 0;      // Non-zero => ignore activations

static void menu__go_down_dir(GtkMenuItem*, gpointer user_data);
static void menu__go_up_dir  (GtkMenuItem*, gpointer user_data);
static void menu__refresh    (GtkMenuItem*, gpointer user_data);
static GtkWidget* fm_make_subdir_menu();
//static void set_sort(gpointer data, guint action, GtkWidget *widget);
//static void reverse_sort(gpointer data, guint action, GtkWidget *widget);

#undef N_
#define N_(x) x

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
{">" N_("Sort by Name"),    NULL,     set_sort,         SORT_NAME,           NULL},
{">" N_("Sort by Type"),    NULL,     set_sort,         SORT_TYPE,           NULL},
{">" N_("Sort by Date"),    NULL,     set_sort,         SORT_DATE,           NULL},
{">" N_("Sort by Size"),    NULL,     set_sort,         SORT_SIZE,           NULL},
{">" N_("Sort by Owner"),   NULL,     set_sort,         SORT_OWNER,          NULL},
{">" N_("Sort by Group"),   NULL,     set_sort,         SORT_GROUP,          NULL},
{">" N_("Reversed"),        NULL,     reverse_sort,     0,                   "<ToggleItem>"},
{">",                       NULL, NULL, 0, "<Separator>"},
/*
{">" N_("Show Hidden"),     "<Ctrl>H", hidden, 0, "<ToggleItem>"},
{">" N_("Filter Files..."),     NULL, mini_buffer, MINI_FILTER, NULL},
{">" N_("Filter Directories With Files"),   NULL, filter_directories, 0, "<ToggleItem>"},
{">" N_("Show Thumbnails"), NULL, show_thumbs, 0, "<ToggleItem>"},
{">" N_("Refresh"),         NULL, refresh, 0, "<StockItem>", GTK_STOCK_REFRESH},
{">" N_("Save Current Display Settings..."),     NULL, save_settings, 0, NULL},
{N_("File"),                NULL, NULL, 0, "<Branch>"},
{">" N_("Copy..."),         "<Ctrl>C", file_op, FILE_COPY_ITEM, "<StockItem>", GTK_STOCK_COPY},
{">" N_("Rename..."),       NULL, file_op, FILE_RENAME_ITEM, NULL},
{">" N_("Link..."),         NULL, file_op, FILE_LINK_ITEM, NULL},
{">" N_("Delete"),          "<Ctrl>X", file_op, FILE_DELETE, "<StockItem>", GTK_STOCK_DELETE},
{">",                       NULL, NULL, 0, "<Separator>"},
{">" N_("Shift Open"),      NULL, file_op, FILE_OPEN_FILE},
{">" N_("Send To..."),      NULL, file_op, FILE_SEND_TO, NULL},
{">",                       NULL, NULL, 0, "<Separator>"},
{">" N_("Set Run Action..."),   "asterisk", file_op, FILE_RUN_ACTION, "<StockItem>", GTK_STOCK_EXECUTE},
{">" N_("Set Icon..."),     NULL, file_op, FILE_SET_ICON, NULL},
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
{">" N_("Customise Menu..."),   NULL, customise_new, 0, NULL},
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
{N_("Help"),            NULL, NULL, 0, "<Branch>"},
{">" N_("About ROX-Filer..."),  NULL, menu_rox_help, HELP_ABOUT, NULL},
{">" N_("Show Help Files"), "F1", menu_rox_help, HELP_DIR, "<StockItem>", GTK_STOCK_HELP},
{">" N_("Manual"),      NULL, menu_rox_help, HELP_MANUAL, NULL},
*/
};
#endif

typedef struct
{
	char*     label;
	GCallback callback;
} menu_def;


static menu_def fm_menu_def[] = {
	{"Go up directory", G_CALLBACK(menu__go_up_dir)},
	{"Refresh",         G_CALLBACK(menu__refresh)},
	//{"Delete", NULL}
};


GtkWidget*
fm_make_context_menu()
{
	GtkWidget* menu = gtk_menu_new();

	int i; for(i=0;i<A_SIZE(fm_menu_def);i++){
		dbg(2, "i=%i", i);
		menu_def* item = &fm_menu_def[i];
		GtkWidget* menu_item = gtk_menu_item_new_with_label (item->label);
		gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
		if(item->callback) g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(item->callback), NULL);
	}

	GtkWidget* cd = gtk_image_menu_item_new_with_mnemonic("Cd");
	GtkWidget* ico = gtk_image_new_from_pixbuf(mime_type_get_pixbuf(inode_directory));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(cd), ico);
	gtk_container_add(GTK_CONTAINER(menu), cd);

	GtkWidget* submenu = fm_make_subdir_menu();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(cd), submenu);

	gtk_widget_show_all(menu);
	return menu;
}


static GtkWidget*
fm_make_subdir_menu()
{
	GtkWidget* submenu = gtk_menu_new();

	if (filer.real_path) {
		GDir* dir = g_dir_open((char*)filer.real_path, 0, NULL);
		const char* leaf;
		if (dir) {
			while ((leaf = g_dir_read_name(dir))) {
				if (leaf[0] == '.') continue;
				gchar* filename = g_build_filename(filer.real_path, leaf, NULL);
				if (g_file_test(filename, G_FILE_TEST_IS_DIR)) {
					GtkWidget* item = gtk_image_menu_item_new_with_mnemonic (leaf);
					GtkWidget* ico = gtk_image_new_from_pixbuf(mime_type_get_pixbuf(inode_directory));
					gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), ico);
					gtk_container_add(GTK_CONTAINER(submenu), item);
					g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu__go_down_dir), NULL);
				}
				g_free(filename);
			}
			g_dir_close(dir);
		}
	}
	return submenu;
}


/*
 *   for application-level menu additions.
 */
void
fm_add_menu_item(GtkAction* action)
{
	GtkWidget* menu_item = gtk_action_create_menu_item(action);
	gtk_menu_shell_append(GTK_MENU_SHELL(filer.menu), menu_item);
	//GCClosure* closure = gtk_action_get_accel_closure(action);
	//dbg(0, "closure=%p callback=%p", closure, closure->callback);
	gtk_widget_show(menu_item);
}


void
fm_menu__dir_update()
{
	//update menu items related to the current directory.
	PF;
	GtkWidget* menu = filer.menu;
	GList* items = GTK_MENU_SHELL(menu)->children;
	for(;items;items=items->next){
		GtkMenuItem* item = items->data;
		GtkWidget* submenu = gtk_menu_item_get_submenu(item);
		if(submenu){
			GtkWidget* cd = fm_make_subdir_menu();
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), cd);
			gtk_widget_show_all(GTK_WIDGET(item));
			break;
		}
	}
}


static void
menu__go_down_dir(GtkMenuItem* menuitem, gpointer user_data)
{
	GList* children = gtk_container_get_children(GTK_CONTAINER(menuitem));
	for (;children;children=children->next) {
		GtkWidget* child = children->data;
		if (GTK_IS_LABEL(child)) {
			const gchar* leaf = gtk_label_get_text((GtkLabel*)child);
			gchar* filename = g_build_filename(filer.real_path, leaf, NULL);

			filer_change_to(&filer, filename, NULL);

			g_free(filename);
		}
	}
}


static void
menu__go_up_dir(GtkMenuItem* menuitem, gpointer user_data)
{
	change_to_parent(&filer);
}


static void
menu__refresh(GtkMenuItem* menuitem, gpointer user_data)
{
	PF;
	//filer_refresh(&filer);
	file_manager__update_all();
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

static void
set_sort(gpointer data, guint action, GtkWidget *widget)
{
	if (updating_menu) return;

	//g_return_if_fail(window_with_focus != NULL);

	display_set_sort_type(&filer, action, GTK_SORT_ASCENDING);
}


static void
reverse_sort(gpointer data, guint action, GtkWidget *widget)
{
    GtkSortType order;

    if (updating_menu)
        return;

    g_return_if_fail(&filer);

    order = filer.sort_order;
    if (order == GTK_SORT_ASCENDING)
        order = GTK_SORT_DESCENDING;
    else
        order = GTK_SORT_ASCENDING;

    display_set_sort_type(&filer, filer.sort_type, order);
}
#endif
