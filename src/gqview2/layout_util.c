/*
 * GQview
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */
#include <gtk/gtk.h>
#include "../typedefs.h"
#include "support.h"

#include "gqview.h"
#include "layout_util.h"

//#include "bar_info.h"
//#include "bar_exif.h"
//#include "bar_sort.h"
//#include "cache_maint.h"
//#include "collect.h"
//#include "collect-dlg.h"
//#include "dupe.h"
//#include "editors.h"
//#include "info.h"
//#include "layout_image.h"
#include "pixbuf_util.h"
//#include "preferences.h"
//#include "print.h"
//#include "search.h"
//#include "utilops.h"
//#include "ui_bookmark.h"
//#include "ui_fileops.h"
//#include "ui_menu.h"
//#include "ui_misc.h"
//#include "ui_tabcomp.h"

#include <gdk/gdkkeysyms.h> /* for keyboard values */

//#include "icons/icon_thumb.xpm"
//#include "icons/icon_float.xpm"


#define MENU_EDIT_ACTION_OFFSET 16


/*
 *-----------------------------------------------------------------------------
 * keyboard handler
 *-----------------------------------------------------------------------------
 */

#if 0
static guint tree_key_overrides[] = {
	GDK_Page_Up,	GDK_KP_Page_Up,
	GDK_Page_Down,	GDK_KP_Page_Down,
	GDK_Home,	GDK_KP_Home,
	GDK_End,	GDK_KP_End
};

static gint layout_key_match(guint keyval)
{
	gint i;

	for (i = 0; i < sizeof(tree_key_overrides) / sizeof(guint); i++)
		{
		if (keyval == tree_key_overrides[i]) return TRUE;
		}

	return FALSE;
}

static gint layout_key_press_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	LayoutWindow *lw = data;
	gint stop_signal = FALSE;
	gint x = 0;
	gint y = 0;

	if (lw->path_entry && GTK_WIDGET_HAS_FOCUS(lw->path_entry))
		{
		if (event->keyval == GDK_Escape && lw->path)
			{
			gtk_entry_set_text(GTK_ENTRY(lw->path_entry), lw->path);
			}

		/* the gtkaccelgroup of the window is stealing presses before they get to the entry (and more),
		 * so when the some widgets have focus, give them priority (HACK)
		 */
		if (gtk_widget_event(lw->path_entry, (GdkEvent *)event))
			{
			return TRUE;
			}
		}
	if (lw->vdt && GTK_WIDGET_HAS_FOCUS(lw->vdt->treeview) &&
	    !layout_key_match(event->keyval) &&
	    gtk_widget_event(lw->vdt->treeview, (GdkEvent *)event))
		{
		return TRUE;
		}
	if (lw->bar_info &&
	    bar_info_event(lw->bar_info, (GdkEvent *)event))
		{
		return TRUE;
		}

	if (lw->image &&
	    (GTK_WIDGET_HAS_FOCUS(lw->image->widget) || (lw->tools && widget == lw->window)) )
		{
		switch (event->keyval)
			{
			case GDK_Left: case GDK_KP_Left:
				x -= 1;
				stop_signal = TRUE;
				break;
			case GDK_Right: case GDK_KP_Right:
				x += 1;
				stop_signal = TRUE;
				break;
			case GDK_Up: case GDK_KP_Up:
				y -= 1;
				stop_signal = TRUE;
				break;
			case GDK_Down: case GDK_KP_Down:
				y += 1;
				stop_signal = TRUE;
				break;
			case GDK_BackSpace:
			case 'B': case 'b':
				layout_image_prev(lw);
				stop_signal = TRUE;
				break;
			case GDK_space:
			case 'N': case 'n':
				layout_image_next(lw);
				stop_signal = TRUE;
				break;
			case GDK_Menu:
				layout_image_menu_popup(lw);
				stop_signal = TRUE;
				break;
			}
		}

	if (!stop_signal && !(event->state & GDK_CONTROL_MASK) )
	    switch (event->keyval)
		{
		case '+': case GDK_KP_Add:
			layout_image_zoom_adjust(lw, get_zoom_increment());
			stop_signal = TRUE;
			break;
		case GDK_KP_Subtract:
			layout_image_zoom_adjust(lw, -get_zoom_increment());
			stop_signal = TRUE;
			break;
		case GDK_KP_Multiply:
			layout_image_zoom_set(lw, 0.0);
			stop_signal = TRUE;
			break;
		case GDK_KP_Divide:
		case '1':
			layout_image_zoom_set(lw, 1.0);
			stop_signal = TRUE;
			break;
		case '2':
			layout_image_zoom_set(lw, 2.0);
			stop_signal = TRUE;
			break;
		case '3':
			layout_image_zoom_set(lw, 3.0);
			stop_signal = TRUE;
			break;
		case '4':
			layout_image_zoom_set(lw, 4.0);
			stop_signal = TRUE;
			break;
		case '7':
			layout_image_zoom_set(lw, -4.0);
			stop_signal = TRUE;
			break;
		case '8':
			layout_image_zoom_set(lw, -3.0);
			stop_signal = TRUE;
			break;
		case '9':
			layout_image_zoom_set(lw, -2.0);
			stop_signal = TRUE;
			break;
		case 'W': case 'w':
			layout_image_zoom_set_fill_geometry(lw, FALSE);
			break;
		case 'H': case 'h':
			layout_image_zoom_set_fill_geometry(lw, TRUE);
			break;
		case GDK_Page_Up: case GDK_KP_Page_Up:
			layout_image_prev(lw);
			stop_signal = TRUE;
			break;
		case GDK_Page_Down: case GDK_KP_Page_Down:
			layout_image_next(lw);
			stop_signal = TRUE;
			break;
		case GDK_Home: case GDK_KP_Home:
			layout_image_first(lw);
			stop_signal = TRUE;
			break;
		case GDK_End: case GDK_KP_End:
			layout_image_last(lw);
			stop_signal = TRUE;
			break;
		case GDK_Delete: case GDK_KP_Delete:
			if (enable_delete_key)
				{
				file_util_delete(NULL, layout_selection_list(lw), widget);
				stop_signal = TRUE;
				}
			break;
		case GDK_Escape:
			/* FIXME:interrupting thumbs no longer allowed */
#if 0
			interrupt_thumbs();
#endif
			stop_signal = TRUE;
			break;
		case 'P': case 'p':
			layout_image_slideshow_pause_toggle(lw);
			break;
		case 'V': case 'v':
			if (!(event->state & GDK_MOD1_MASK)) layout_image_full_screen_toggle(lw);
			break;
		}

	if (event->state & GDK_SHIFT_MASK)
		{
		x *= 3;
		y *= 3;
		}

	if (x != 0 || y!= 0)
		{
		keyboard_scroll_calc(&x, &y, event);
		layout_image_scroll(lw, x, y);
		}

	if (stop_signal) g_signal_stop_emission_by_name(G_OBJECT(widget), "key_press_event");

	return stop_signal;
}

void layout_keyboard_init(LayoutWindow *lw, GtkWidget *window)
{
	g_signal_connect(G_OBJECT(window), "key_press_event",
			 G_CALLBACK(layout_key_press_cb), lw);
}

/*
 *-----------------------------------------------------------------------------
 * menu callbacks
 *-----------------------------------------------------------------------------
 */

static void layout_menu_new_window_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;
	LayoutWindow *nw;

	nw = layout_new(NULL, FALSE, FALSE);
	layout_sort_set(nw, file_sort_method, file_sort_ascending);
	layout_set_path(nw, layout_get_path(lw));
}

static void layout_menu_new_cb(GtkAction *action, gpointer data)
{
	collection_window_new(NULL);
}

static void layout_menu_open_cb(GtkAction *action, gpointer data)
{
	collection_dialog_load(NULL);
}

static void layout_menu_search_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	search_new(lw->path, layout_image_get_path(lw));
}

static void layout_menu_dupes_cb(GtkAction *action, gpointer data)
{
	dupe_window_new(DUPE_MATCH_NAME);
}

static void layout_menu_print_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	print_window_new(layout_image_get_path(lw), layout_selection_list(lw), layout_list(lw), lw->window);
}

static void layout_menu_dir_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	file_util_create_dir(lw->path, lw->window);
}

static void layout_menu_copy_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	file_util_copy(NULL, layout_selection_list(lw), NULL, lw->window);
}

static void layout_menu_move_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	file_util_move(NULL, layout_selection_list(lw), NULL, lw->window);
}

static void layout_menu_rename_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	file_util_rename(NULL, layout_selection_list(lw), lw->window);
}

static void layout_menu_delete_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	file_util_delete(NULL, layout_selection_list(lw), lw->window);
}

static void layout_menu_close_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_close(lw);
}

static void layout_menu_exit_cb(GtkAction *action, gpointer data)
{
	exit_gqview();
}

static void layout_menu_alter_90_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_alter(lw, ALTER_ROTATE_90);
}

static void layout_menu_alter_90cc_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_alter(lw, ALTER_ROTATE_90_CC);
}

static void layout_menu_alter_180_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_alter(lw, ALTER_ROTATE_180);
}

static void layout_menu_alter_mirror_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_alter(lw, ALTER_MIRROR);
}

static void layout_menu_alter_flip_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_alter(lw, ALTER_FLIP);
}

static void layout_menu_info_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;
	GList *list;
	const gchar *path = NULL;

	list = layout_selection_list(lw);
	if (!list) path = layout_image_get_path(lw);

	info_window_new(path, list);
}

static void layout_menu_select_all_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_select_all(lw);
}

static void layout_menu_unselect_all_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_select_none(lw);
}

static void layout_menu_config_cb(GtkAction *action, gpointer data)
{
	show_config_window();
}

static void layout_menu_remove_thumb_cb(GtkAction *action, gpointer data)
{
	cache_manager_show();
}

static void layout_menu_wallpaper_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_to_root(lw);
}

static void layout_menu_zoom_in_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_zoom_adjust(lw, get_zoom_increment());
}

static void layout_menu_zoom_out_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_zoom_adjust(lw, -get_zoom_increment());
}

static void layout_menu_zoom_1_1_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_zoom_set(lw, 1.0);
}

static void layout_menu_zoom_fit_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_zoom_set(lw, 0.0);
}

static void layout_menu_thumb_cb(GtkToggleAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_thumb_set(lw, gtk_toggle_action_get_active(action));
}

static void layout_menu_list_cb(GtkRadioAction *action, GtkRadioAction *current, gpointer data)
{
	LayoutWindow *lw = data;

	layout_views_set(lw, lw->tree_view, (gtk_radio_action_get_current_value(action) == 1));
}

#if 0
static void layout_menu_icon_cb(gpointer data, guint action, GtkWidget *widget)
{
	LayoutWindow *lw = data;

	if (!GTK_CHECK_MENU_ITEM(widget)->active) return;

	layout_views_set(lw, lw->tree_view, TRUE);
}
#endif

static void layout_menu_tree_cb(GtkToggleAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_views_set(lw, gtk_toggle_action_get_active(action), lw->icon_view);
}

static void layout_menu_fullscreen_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_full_screen_toggle(lw);
}

static void layout_menu_refresh_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_refresh(lw);
}

static void layout_menu_float_cb(GtkToggleAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	if (lw->tools_float != gtk_toggle_action_get_active(action))
		{
		layout_tools_float_toggle(lw);
		}
}

static void layout_menu_hide_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_tools_hide_toggle(lw);
}

static void layout_menu_toolbar_cb(GtkToggleAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	if (lw->toolbar_hidden != gtk_toggle_action_get_active(action))
		{
		layout_toolbar_toggle(lw);
		}
}

static void layout_menu_bar_info_cb(GtkToggleAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	if (lw->bar_info_enabled != gtk_toggle_action_get_active(action))
		{
		layout_bar_info_toggle(lw);
		}
}

static void layout_menu_bar_exif_cb(GtkToggleAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	if (lw->bar_exif_enabled != gtk_toggle_action_get_active(action))
		{
		layout_bar_exif_toggle(lw);
		}
}

static void layout_menu_bar_sort_cb(GtkToggleAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	if (lw->bar_sort_enabled != gtk_toggle_action_get_active(action))
		{
		layout_bar_sort_toggle(lw);
		}
}

static void layout_menu_slideshow_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_slideshow_toggle(lw);
}

static void layout_menu_help_cb(GtkAction *action, gpointer data)
{
	help_window_show("html_contents");
}

static void layout_menu_help_keys_cb(GtkAction *action, gpointer data)
{
	help_window_show("documentation");
}

static void layout_menu_notes_cb(GtkAction *action, gpointer data)
{
	help_window_show("release_notes");
}

static void layout_menu_about_cb(GtkAction *action, gpointer data)
{
	show_about_window();
}

/*
 *-----------------------------------------------------------------------------
 * edit menu
 *-----------------------------------------------------------------------------
 */ 

static void layout_menu_edit_cb(GtkAction *action, gpointer data)
{
	LayoutWindow *lw = data;
	GList *list;
	gint n;

	n = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "edit_index"));

	list = layout_selection_list(lw);
	start_editor_from_path_list(n, list);
	path_list_free(list);
}

static void layout_menu_edit_update(LayoutWindow *lw)
{
	gint i;

	/* main edit menu */

	if (!lw->action_group) return;

	for (i = 0; i < 10; i++)
		{
		gchar *key;
		GtkAction *action;

		key = g_strdup_printf("Editor%d", i);

		action = gtk_action_group_get_action(lw->action_group, key);
		g_object_set_data(G_OBJECT(action), "edit_index", GINT_TO_POINTER(i));

		if (editor_command[i] && strlen(editor_command[i]) > 0)
			{
			gchar *text;

			if (editor_name[i] && strlen(editor_name[i]) > 0)
				{
				text = g_strdup_printf(_("in %s..."), editor_name[i]);
				}
			else
				{
				text = g_strdup(_("in (unknown)..."));
				}
			g_object_set(action, "label", text,
					     "sensitive", TRUE, NULL);
			g_free(text);
			}
		else
			{
			g_object_set(action, "label", _("empty"),
					     "sensitive", FALSE, NULL);
			}

		g_free(key);
		}
}

void layout_edit_update_all(void)
{
	GList *work;

	work = layout_window_list;
	while (work)
		{
		LayoutWindow *lw = work->data;
		work = work->next;

		layout_menu_edit_update(lw);
		}
}

/*
 *-----------------------------------------------------------------------------
 * recent menu
 *-----------------------------------------------------------------------------
 */

static void layout_menu_recent_cb(GtkWidget *widget, gpointer data)
{
	gint n;
	gchar *path;

	n = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "recent_index"));

	path = g_list_nth_data(history_list_get_by_key("recent"), n);

	if (!path) return;

	/* make a copy of it */
	path = g_strdup(path);
	collection_window_new(path);
	g_free(path);
}

static void layout_menu_recent_update(LayoutWindow *lw)
{
	GtkWidget *menu;
	GtkWidget *recent;
	GtkWidget *item;
	GList *list;
	gint n;

	if (!lw->ui_manager) return;

	list = history_list_get_by_key("recent");
	n = 0;

	menu = gtk_menu_new();

	while (list)
		{
		item = menu_item_add_simple(menu, filename_from_path((gchar *)list->data),
					    G_CALLBACK(layout_menu_recent_cb), lw);
		g_object_set_data(G_OBJECT(item), "recent_index", GINT_TO_POINTER(n));
		list = list->next;
		n++;
		}

	if (n == 0)
		{
		menu_item_add(menu, _("Empty"), NULL, NULL);
		}

	recent = gtk_ui_manager_get_widget(lw->ui_manager, "/MainMenu/FileMenu/OpenRecent");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(recent), menu);
	gtk_widget_set_sensitive(recent, (n != 0));
}

void layout_recent_update_all(void)
{
	GList *work;

	work = layout_window_list;
	while (work)
		{
		LayoutWindow *lw = work->data;
		work = work->next;

		layout_menu_recent_update(lw);
		}
}

void layout_recent_add_path(const gchar *path)
{
	if (!path) return;

	history_list_add_to_key("recent", path, recent_list_max);

	layout_recent_update_all();
}

/*
 *-----------------------------------------------------------------------------
 * menu
 *-----------------------------------------------------------------------------
 */ 

#define CB G_CALLBACK

static GtkActionEntry menu_entries[] = {
  { "FileMenu",		NULL,		N_("_File") },
  { "EditMenu",		NULL,		N_("_Edit") },
  { "AdjustMenu",	NULL,		N_("_Adjust") },
  { "ViewMenu",		NULL,		N_("_View") },
  { "HelpMenu",		NULL,		N_("_Help") },

  { "NewWindow",	GTK_STOCK_NEW,	N_("New _window"),	NULL,		NULL,	CB(layout_menu_new_window_cb) },
  { "NewCollection",	GTK_STOCK_INDEX,N_("_New collection"),	"C",		NULL,	CB(layout_menu_new_cb) },
  { "OpenCollection",	GTK_STOCK_OPEN,	N_("_Open collection..."),"O",		NULL,	CB(layout_menu_open_cb) },
  { "OpenRecent",	NULL,		N_("Open _recent") },
  { "Search",		GTK_STOCK_FIND,	N_("_Search..."),	"F3",		NULL,	CB(layout_menu_search_cb) },
  { "FindDupes",	GTK_STOCK_FIND,	N_("_Find duplicates..."),"D",		NULL,	CB(layout_menu_dupes_cb) },
  { "Print",		GTK_STOCK_PRINT,N_("_Print..."),	"<shift>P",	NULL,	CB(layout_menu_print_cb) },
  { "NewFolder",	NULL,		N_("N_ew folder..."),	"<control>F",	NULL,	CB(layout_menu_dir_cb) },
  { "Copy",		NULL,		N_("_Copy..."),		"<control>C",	NULL,	CB(layout_menu_copy_cb) },
  { "Move",		NULL,		N_("_Move..."),		"<control>M",	NULL,	CB(layout_menu_move_cb) },
  { "Rename",		NULL,		N_("_Rename..."),	"<control>R",	NULL,	CB(layout_menu_rename_cb) },
  { "Delete",	GTK_STOCK_DELETE,	N_("_Delete..."),	"<control>D",	NULL,	CB(layout_menu_delete_cb) },
  { "CloseWindow",	GTK_STOCK_CLOSE,N_("C_lose window"),	"<control>W",	NULL,	CB(layout_menu_close_cb) },
  { "Quit",		GTK_STOCK_QUIT, N_("_Quit"),		"<control>Q",	NULL,	CB(layout_menu_exit_cb) },

  { "Editor0",		NULL,		"editor0",		"<control>1",	NULL,	CB(layout_menu_edit_cb) },
  { "Editor1",		NULL,		"editor1",		"<control>2",	NULL,	CB(layout_menu_edit_cb) },
  { "Editor2",		NULL,		"editor2",		"<control>3",	NULL,	CB(layout_menu_edit_cb) },
  { "Editor3",		NULL,		"editor3",		"<control>4",	NULL,	CB(layout_menu_edit_cb) },
  { "Editor4",		NULL,		"editor4",		"<control>5",	NULL,	CB(layout_menu_edit_cb) },
  { "Editor5",		NULL,		"editor5",		"<control>6",	NULL,	CB(layout_menu_edit_cb) },
  { "Editor6",		NULL,		"editor6",		"<control>7",	NULL,	CB(layout_menu_edit_cb) },
  { "Editor7",		NULL,		"editor7",		"<control>8",	NULL,	CB(layout_menu_edit_cb) },
  { "Editor8",		NULL,		"editor8",		"<control>9",	NULL,	CB(layout_menu_edit_cb) },
  { "Editor9",		NULL,		"editor9",		"<control>0",	NULL,	CB(layout_menu_edit_cb) },
  { "RotateCW",		NULL,	N_("_Rotate clockwise"),	"bracketright",	NULL,	CB(layout_menu_alter_90_cb) },
  { "RotateCCW",	NULL,	N_("Rotate _counterclockwise"),	"bracketleft",	NULL,	CB(layout_menu_alter_90cc_cb) },
  { "Rotate180",	NULL,		N_("Rotate 1_80"),	"<shift>R",	NULL,	CB(layout_menu_alter_180_cb) },
  { "Mirror",		NULL,		N_("_Mirror"),		"<shift>M",	NULL,	CB(layout_menu_alter_mirror_cb) },
  { "Flip",		NULL,		N_("_Flip"),		"<shift>F",	NULL,	CB(layout_menu_alter_flip_cb) },
  { "Properties",GTK_STOCK_PROPERTIES,	N_("_Properties"),	"<control>P",	NULL,	CB(layout_menu_info_cb) },
  { "SelectAll",	NULL,		N_("Select _all"),	"<control>A",	NULL,	CB(layout_menu_select_all_cb) },
  { "SelectNone",	NULL,		N_("Select _none"), "<control><shift>A",NULL,	CB(layout_menu_unselect_all_cb) },
  { "Preferences",GTK_STOCK_PREFERENCES,N_("P_references..."),	"<control>O",	NULL,	CB(layout_menu_config_cb) },
  { "Maintenance",	NULL,		N_("_Thumbnail maintenance..."),NULL,	NULL,	CB(layout_menu_remove_thumb_cb) },
  { "Wallpaper",	NULL,		N_("Set as _wallpaper"),NULL,		NULL,	CB(layout_menu_wallpaper_cb) },

  { "ZoomIn",	GTK_STOCK_ZOOM_IN,	N_("Zoom _in"),		"equal",	NULL,	CB(layout_menu_zoom_in_cb) },
  { "ZoomOut",	GTK_STOCK_ZOOM_OUT,	N_("Zoom _out"),	"minus",	NULL,	CB(layout_menu_zoom_out_cb) },
  { "Zoom100",	GTK_STOCK_ZOOM_100,	N_("Zoom _1:1"),	"Z",		NULL,	CB(layout_menu_zoom_1_1_cb) },
  { "ZoomFit",	GTK_STOCK_ZOOM_FIT,	N_("_Zoom to fit"),	"X",		NULL,	CB(layout_menu_zoom_fit_cb) },
  { "FullScreen",	NULL,		N_("F_ull screen"),	"F",		NULL,	CB(layout_menu_fullscreen_cb) },
  { "HideTools",	NULL,		N_("_Hide file list"),	"<control>H",	NULL,	CB(layout_menu_hide_cb) },
  { "SlideShow",	NULL,		N_("Toggle _slideshow"),"S",		NULL,	CB(layout_menu_slideshow_cb) },
  { "Refresh",	GTK_STOCK_REFRESH,	N_("_Refresh"),		"R",		NULL,	CB(layout_menu_refresh_cb) },

  { "HelpContents",	GTK_STOCK_HELP,	N_("_Contents"),	"F1",		NULL,	CB(layout_menu_help_cb) },
  { "HelpShortcuts",	NULL,		N_("_Keyboard shortcuts"),NULL,		NULL,	CB(layout_menu_help_keys_cb) },
  { "HelpNotes",	NULL,		N_("_Release notes"),	NULL,		NULL,	CB(layout_menu_notes_cb) },
  { "About",		NULL,		N_("_About"),		NULL,		NULL,	CB(layout_menu_about_cb) }
};

static GtkToggleActionEntry menu_toggle_entries[] = {
  { "Thumbnails",	NULL,		N_("_Thumbnails"),	"T",		NULL,	CB(layout_menu_thumb_cb) },
  { "FolderTree",	NULL,		N_("Tr_ee"),		"<control>T",	NULL,	CB(layout_menu_tree_cb) },
  { "FloatTools",	NULL,		N_("_Float file list"),	"L",		NULL,	CB(layout_menu_float_cb) },
  { "HideToolbar",	NULL,		N_("Hide tool_bar"),	NULL,		NULL,	CB(layout_menu_toolbar_cb) },
  { "SBarKeywords",	NULL,		N_("_Keywords"),	"<control>K",	NULL,	CB(layout_menu_bar_info_cb) },
  { "SBarExif",		NULL,		N_("E_xif data"),	"<control>E",	NULL,	CB(layout_menu_bar_exif_cb) },
  { "SBarSort",		NULL,		N_("Sort _manager"),	"<control>S",	NULL,	CB(layout_menu_bar_sort_cb) }
};

static GtkRadioActionEntry menu_radio_entries[] = {
  { "ViewList",		NULL,		N_("_List"),		"<control>L",	NULL,	0 },
  { "ViewIcons",	NULL,		N_("I_cons"),		"<control>I",	NULL,	1 }
};

#undef CB

static const char *menu_ui_description =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='FileMenu'>"
"      <menuitem action='NewWindow'/>"
"      <menuitem action='NewCollection'/>"
"      <menuitem action='OpenCollection'/>"
"      <menuitem action='OpenRecent'/>"
"      <separator/>"
"      <menuitem action='Search'/>"
"      <menuitem action='FindDupes'/>"
"      <separator/>"
"      <menuitem action='Print'/>"
"      <menuitem action='NewFolder'/>"
"      <separator/>"
"      <menuitem action='Copy'/>"
"      <menuitem action='Move'/>"
"      <menuitem action='Rename'/>"
"      <menuitem action='Delete'/>"
"      <separator/>"
"      <menuitem action='CloseWindow'/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='EditMenu'>"
"      <menuitem action='Editor0'/>"
"      <menuitem action='Editor1'/>"
"      <menuitem action='Editor2'/>"
"      <menuitem action='Editor3'/>"
"      <menuitem action='Editor4'/>"
"      <menuitem action='Editor5'/>"
"      <menuitem action='Editor6'/>"
"      <menuitem action='Editor7'/>"
"      <menuitem action='Editor8'/>"
"      <menuitem action='Editor9'/>"
"      <separator/>"
"      <menu action='AdjustMenu'>"
"        <menuitem action='RotateCW'/>"
"        <menuitem action='RotateCCW'/>"
"        <menuitem action='Rotate180'/>"
"        <menuitem action='Mirror'/>"
"        <menuitem action='Flip'/>"
"      </menu>"
"      <menuitem action='Properties'/>"
"      <separator/>"
"      <menuitem action='SelectAll'/>"
"      <menuitem action='SelectNone'/>"
"      <separator/>"
"      <menuitem action='Preferences'/>"
"      <menuitem action='Maintenance'/>"
"      <separator/>"
"      <menuitem action='Wallpaper'/>"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <separator/>"
"      <menuitem action='ZoomIn'/>"
"      <menuitem action='ZoomOut'/>"
"      <menuitem action='Zoom100'/>"
"      <menuitem action='ZoomFit'/>"
"      <separator/>"
"      <menuitem action='Thumbnails'/>"
"      <menuitem action='ViewList'/>"
"      <menuitem action='ViewIcons'/>"
"      <separator/>"
"      <menuitem action='FolderTree'/>"
"      <menuitem action='FullScreen'/>"
"      <separator/>"
"      <menuitem action='FloatTools'/>"
"      <menuitem action='HideTools'/>"
"      <menuitem action='HideToolbar'/>"
"      <separator/>"
"      <menuitem action='SBarKeywords'/>"
"      <menuitem action='SBarExif'/>"
"      <menuitem action='SBarSort'/>"
"      <separator/>"
"      <menuitem action='SlideShow'/>"
"      <menuitem action='Refresh'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <separator/>"
"      <menuitem action='HelpContents'/>"
"      <menuitem action='HelpShortcuts'/>"
"      <menuitem action='HelpNotes'/>"
"      <separator/>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"</ui>";


static gchar *menu_translate(const gchar *path, gpointer data)
{
	return _(path);
}

void layout_actions_setup(LayoutWindow *lw)
{
	GError *error;

	if (lw->ui_manager) return;

	lw->action_group = gtk_action_group_new ("MenuActions");
	gtk_action_group_set_translate_func(lw->action_group, menu_translate, NULL, NULL);

	gtk_action_group_add_actions(lw->action_group,
				     menu_entries, G_N_ELEMENTS(menu_entries), lw);
	gtk_action_group_add_toggle_actions(lw->action_group,
					    menu_toggle_entries, G_N_ELEMENTS(menu_toggle_entries), lw);
	gtk_action_group_add_radio_actions(lw->action_group,
					   menu_radio_entries, G_N_ELEMENTS(menu_radio_entries),
					   0, G_CALLBACK(layout_menu_list_cb), lw);

	lw->ui_manager = gtk_ui_manager_new();
	gtk_ui_manager_set_add_tearoffs(lw->ui_manager, TRUE);
	gtk_ui_manager_insert_action_group(lw->ui_manager, lw->action_group, 0);

	error = NULL;
	if (!gtk_ui_manager_add_ui_from_string(lw->ui_manager, menu_ui_description, -1, &error))
		{
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
		exit (EXIT_FAILURE);
		}
}

void layout_actions_add_window(LayoutWindow *lw, GtkWidget *window)
{
	GtkAccelGroup *group;

	if (!lw->ui_manager) return;

	group = gtk_ui_manager_get_accel_group(lw->ui_manager);
	gtk_window_add_accel_group(GTK_WINDOW(window), group);
}

GtkWidget *layout_actions_menu_bar(LayoutWindow *lw)
{
	return gtk_ui_manager_get_widget(lw->ui_manager, "/MainMenu");
}


/*
 *-----------------------------------------------------------------------------
 * toolbar
 *-----------------------------------------------------------------------------
 */ 

static void layout_button_thumb_cb(GtkWidget *widget, gpointer data)
{
	LayoutWindow *lw = data;

	layout_thumb_set(lw, GTK_TOGGLE_BUTTON(widget)->active);
}

static void layout_button_home_cb(GtkWidget *widget, gpointer data)
{
	LayoutWindow *lw = data;
	const gchar *path = homedir();

	if (path) layout_set_path(lw, path);
}

static void layout_button_refresh_cb(GtkWidget *widget, gpointer data)
{
	LayoutWindow *lw = data;

	layout_refresh(lw);
}

static void layout_button_zoom_in_cb(GtkWidget *widget, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_zoom_adjust(lw, get_zoom_increment());
}

static void layout_button_zoom_out_cb(GtkWidget *widget, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_zoom_adjust(lw, -get_zoom_increment());
}

static void layout_button_zoom_fit_cb(GtkWidget *widget, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_zoom_set(lw, 0.0);
}

static void layout_button_zoom_1_1_cb(GtkWidget *widget, gpointer data)
{
	LayoutWindow *lw = data;

	layout_image_zoom_set(lw, 1.0);
}

static void layout_button_config_cb(GtkWidget *widget, gpointer data)
{
	show_config_window();
}

static void layout_button_float_cb(GtkWidget *widget, gpointer data)
{
	LayoutWindow *lw = data;

	layout_tools_float_toggle(lw);
}

GtkWidget *layout_button(GtkWidget *box, gchar **pixmap_data, const gchar *stock_id, gint toggle,
			 GtkTooltips *tooltips, const gchar *tip_text,
			 GCallback func, gpointer data)
{
	GtkWidget *button;
	GtkWidget *icon;

	if (toggle)
		{
		button = gtk_toggle_button_new();
		}
	else
		{
		button = gtk_button_new();
		}

	g_signal_connect(G_OBJECT(button), "clicked", func, data);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
	gtk_widget_show(button);
	gtk_tooltips_set_tip(tooltips, button, tip_text, NULL);

	if (stock_id)
		{
		icon = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_BUTTON);
		}
	else
		{
		GdkPixbuf *pixbuf;

		pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)pixmap_data);
		icon = gtk_image_new_from_pixbuf(pixbuf);
		gdk_pixbuf_unref(pixbuf);
		}

	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

	gtk_container_add(GTK_CONTAINER(button), icon);
	gtk_widget_show(icon);

	return button;
}

GtkWidget *layout_button_bar(LayoutWindow *lw)
{
	GtkWidget *box;
	GtkTooltips *tooltips;

	tooltips = lw->tooltips;

	box = gtk_hbox_new(FALSE, 0);

	lw->thumb_button = layout_button(box, (gchar **)icon_thumb_xpm, NULL, TRUE,
		      tooltips, _("Show thumbnails"), G_CALLBACK(layout_button_thumb_cb), lw);
	layout_button(box, NULL, GTK_STOCK_HOME, FALSE,
		      tooltips, _("Change to home folder"), G_CALLBACK(layout_button_home_cb), lw);
	layout_button(box, NULL, GTK_STOCK_REFRESH, FALSE,
		      tooltips, _("Refresh file list"), G_CALLBACK(layout_button_refresh_cb), lw);
	layout_button(box, NULL, GTK_STOCK_ZOOM_IN, FALSE,
		      tooltips, _("Zoom in"), G_CALLBACK(layout_button_zoom_in_cb), lw);
	layout_button(box, NULL, GTK_STOCK_ZOOM_OUT, FALSE,
		      tooltips, _("Zoom out"), G_CALLBACK(layout_button_zoom_out_cb), lw);
	layout_button(box, NULL, GTK_STOCK_ZOOM_FIT, FALSE,
		      tooltips, _("Fit image to window"), G_CALLBACK(layout_button_zoom_fit_cb), lw);
	layout_button(box, NULL, GTK_STOCK_ZOOM_100, FALSE,
		      tooltips, _("Set zoom 1:1"), G_CALLBACK(layout_button_zoom_1_1_cb), lw);
	layout_button(box, NULL, GTK_STOCK_PREFERENCES, FALSE,
		      tooltips, _("Configure options"), G_CALLBACK(layout_button_config_cb), lw);
	layout_button(box, (gchar **)icon_float_xpm, NULL, FALSE,
		      tooltips, _("Float Controls"), G_CALLBACK(layout_button_float_cb), lw);

	return box;
}

/*
 *-----------------------------------------------------------------------------
 * misc
 *-----------------------------------------------------------------------------
 */

static void layout_util_sync_views(LayoutWindow *lw)
{
	GtkAction *action;

	if (!lw->action_group) return;

	action = gtk_action_group_get_action(lw->action_group, "FolderTree");
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), lw->tree_view);

	action = gtk_action_group_get_action(lw->action_group, "ViewIcons");
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), lw->icon_view);

	action = gtk_action_group_get_action(lw->action_group, "FloatTools");
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), lw->tools_float);

	action = gtk_action_group_get_action(lw->action_group, "SBarKeywords");
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), lw->bar_info_enabled);

	action = gtk_action_group_get_action(lw->action_group, "SBarExif");
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), lw->bar_exif_enabled);

	action = gtk_action_group_get_action(lw->action_group, "SBarSort");
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), lw->bar_sort_enabled);

	action = gtk_action_group_get_action(lw->action_group, "HideToolbar");
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), lw->toolbar_hidden);
}

void layout_util_sync_thumb(LayoutWindow *lw)
{
	GtkAction *action;

	if (!lw->action_group) return;

	action = gtk_action_group_get_action(lw->action_group, "Thumbnails");
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), lw->thumbs_enabled);
	g_object_set(action, "sensitive", !lw->icon_view, NULL);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lw->thumb_button), lw->thumbs_enabled);
	gtk_widget_set_sensitive(lw->thumb_button, !lw->icon_view);
}

void layout_util_sync(LayoutWindow *lw)
{
	layout_util_sync_views(lw);
	layout_util_sync_thumb(lw);
	layout_menu_recent_update(lw);
	layout_menu_edit_update(lw);
}
#endif

/*
 *-----------------------------------------------------------------------------
 * icons (since all the toolbar icons are included here, best place as any)
 *-----------------------------------------------------------------------------
 */

PixmapFolders *folder_icons_new(void)
{
	PixmapFolders *pf;

	pf = g_new0(PixmapFolders, 1);

	pf->close = pixbuf_inline(PIXBUF_INLINE_FOLDER_CLOSED);
	pf->open = pixbuf_inline(PIXBUF_INLINE_FOLDER_OPEN);
	pf->deny = pixbuf_inline(PIXBUF_INLINE_FOLDER_LOCKED);
	pf->parent = pixbuf_inline(PIXBUF_INLINE_FOLDER_UP);

	return pf;
}

void folder_icons_free(PixmapFolders *pf)
{
	if (!pf) return;

	g_object_unref(pf->close);
	g_object_unref(pf->open);
	g_object_unref(pf->deny);
	g_object_unref(pf->parent);

	g_free(pf);
}

/*
 *-----------------------------------------------------------------------------
 * sidebars
 *-----------------------------------------------------------------------------
 */

#if 0
#define SIDEBAR_WIDTH 288

static void layout_bar_info_destroyed(GtkWidget *widget, gpointer data)
{
	LayoutWindow *lw = data;

	lw->bar_info = NULL;

	if (lw->utility_box)
		{
		/* destroyed from within itself */
		lw->bar_info_enabled = FALSE;
		layout_util_sync_views(lw);
		}
}

static GList *layout_bar_info_list_cb(gpointer data)
{
	LayoutWindow *lw = data;

	return layout_selection_list(lw);
}

static void layout_bar_info_new(LayoutWindow *lw)
{
	if (!lw->utility_box) return;
                                                                                                                    
	lw->bar_info = bar_info_new(layout_image_get_path(lw), FALSE, lw->utility_box);
	bar_info_set_selection_func(lw->bar_info, layout_bar_info_list_cb, lw);
	bar_info_selection(lw->bar_info, layout_selection_count(lw, NULL) - 1);
	bar_info_size_request(lw->bar_info, SIDEBAR_WIDTH * 3 / 4);
	g_signal_connect(G_OBJECT(lw->bar_info), "destroy",
			 G_CALLBACK(layout_bar_info_destroyed), lw);
	lw->bar_info_enabled = TRUE;

	gtk_box_pack_start(GTK_BOX(lw->utility_box), lw->bar_info, FALSE, FALSE, 0);
	gtk_widget_show(lw->bar_info);
}
                                                                                                                    
static void layout_bar_info_close(LayoutWindow *lw)
{
	if (lw->bar_info)
		{
		bar_info_close(lw->bar_info);
		lw->bar_info = NULL;
		}
	lw->bar_info_enabled = FALSE;
}

void layout_bar_info_toggle(LayoutWindow *lw)
{
	if (lw->bar_info_enabled)
		{
		layout_bar_info_close(lw);
		}
	else
		{
		layout_bar_info_new(lw);
		}
}

static void layout_bar_info_new_image(LayoutWindow *lw)
{
	if (!lw->bar_info || !lw->bar_info_enabled) return;

	bar_info_set(lw->bar_info, layout_image_get_path(lw));
}

static void layout_bar_info_new_selection(LayoutWindow *lw, gint count)
{
	if (!lw->bar_info || !lw->bar_info_enabled) return;

	bar_info_selection(lw->bar_info, count - 1);
}

static void layout_bar_info_maint_renamed(LayoutWindow *lw)
{
	if (!lw->bar_info || !lw->bar_info_enabled) return;

	bar_info_maint_renamed(lw->bar_info, layout_image_get_path(lw));
}

static void layout_bar_exif_destroyed(GtkWidget *widget, gpointer data)
{
	LayoutWindow *lw = data;

	if (lw->bar_exif)
		{
		lw->bar_exif_advanced = bar_exif_is_advanced(lw->bar_exif);
		}

	lw->bar_exif = NULL;
	if (lw->utility_box)
		{
		/* destroyed from within itself */
		lw->bar_exif_enabled = FALSE;
		layout_util_sync_views(lw);
		}
}

static void layout_bar_exif_sized(GtkWidget *widget, GtkAllocation *allocation, gpointer data)
{
	LayoutWindow *lw = data;

	if (lw->bar_exif)
		{
		lw->bar_exif_size = allocation->width;
		}
}

static void layout_bar_exif_new(LayoutWindow *lw)
{
	if (!lw->utility_box) return;

	lw->bar_exif = bar_exif_new(TRUE, layout_image_get_path(lw),
				    lw->bar_exif_advanced, lw->utility_box);
	g_signal_connect(G_OBJECT(lw->bar_exif), "destroy",
			 G_CALLBACK(layout_bar_exif_destroyed), lw);
	g_signal_connect(G_OBJECT(lw->bar_exif), "size_allocate",
			 G_CALLBACK(layout_bar_exif_sized), lw);
        lw->bar_exif_enabled = TRUE;

	if (lw->bar_exif_size < 1) lw->bar_exif_size = SIDEBAR_WIDTH;
	gtk_widget_set_size_request(lw->bar_exif, lw->bar_exif_size, -1);
	gtk_box_pack_start(GTK_BOX(lw->utility_box), lw->bar_exif, FALSE, FALSE, 0);
	if (lw->bar_info) gtk_box_reorder_child(GTK_BOX(lw->utility_box), lw->bar_exif, 1);
	gtk_widget_show(lw->bar_exif);
}

static void layout_bar_exif_close(LayoutWindow *lw)
{
	if (lw->bar_exif)
		{
		bar_exif_close(lw->bar_exif);
		lw->bar_exif = NULL;
		}
        lw->bar_exif_enabled = FALSE;
}

void layout_bar_exif_toggle(LayoutWindow *lw)
{
	if (lw->bar_exif_enabled)
		{
		layout_bar_exif_close(lw);
		}
	else
		{
		layout_bar_exif_new(lw);
		}
}

static void layout_bar_exif_new_image(LayoutWindow *lw)
{
	if (!lw->bar_exif || !lw->bar_exif_enabled) return;

	bar_exif_set(lw->bar_exif, layout_image_get_path(lw));
}

static void layout_bar_sort_destroyed(GtkWidget *widget, gpointer data)
{
	LayoutWindow *lw = data;

	lw->bar_sort = NULL;

	if (lw->utility_box)
		{
		/* destroyed from within itself */
		lw->bar_sort_enabled = FALSE;

		layout_util_sync_views(lw);
		}
}

static void layout_bar_sort_new(LayoutWindow *lw)
{
	if (!lw->utility_box) return;

	lw->bar_sort = bar_sort_new(lw);
	g_signal_connect(G_OBJECT(lw->bar_sort), "destroy",
			 G_CALLBACK(layout_bar_sort_destroyed), lw);
	lw->bar_sort_enabled = TRUE;

	gtk_box_pack_end(GTK_BOX(lw->utility_box), lw->bar_sort, FALSE, FALSE, 0);
	gtk_widget_show(lw->bar_sort);
}

static void layout_bar_sort_close(LayoutWindow *lw)
{
	if (lw->bar_sort)
		{
		bar_sort_close(lw->bar_sort);
		lw->bar_sort = NULL;
		}
	lw->bar_sort_enabled = FALSE;
}

void layout_bar_sort_toggle(LayoutWindow *lw)
{
	if (lw->bar_sort_enabled)
		{
		layout_bar_sort_close(lw);
		}
	else
		{
		layout_bar_sort_new(lw);
		}
}

void layout_bars_new_image(LayoutWindow *lw)
{
	layout_bar_info_new_image(lw);
	layout_bar_exif_new_image(lw);
}

void layout_bars_new_selection(LayoutWindow *lw, gint count)
{
	layout_bar_info_new_selection(lw, count);
}

GtkWidget *layout_bars_prepare(LayoutWindow *lw, GtkWidget *image)
{
	lw->utility_box = gtk_hbox_new(FALSE, PREF_PAD_GAP);
	gtk_box_pack_start(GTK_BOX(lw->utility_box), image, TRUE, TRUE, 0);
	gtk_widget_show(image);

	if (lw->bar_sort_enabled)
		{
		layout_bar_sort_new(lw);
		}

	if (lw->bar_info_enabled)
		{
		layout_bar_info_new(lw);
		}

	if (lw->bar_exif_enabled)
		{
		layout_bar_exif_new(lw);
		}

	return lw->utility_box;
}

void layout_bars_close(LayoutWindow *lw)
{
	layout_bar_sort_close(lw);
	layout_bar_exif_close(lw);
	layout_bar_info_close(lw);
}

void layout_bars_maint_renamed(LayoutWindow *lw)
{
	layout_bar_info_maint_renamed(lw);
}
#endif

