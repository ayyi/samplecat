#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <gtk/gtk.h>
#include "file_manager.h"
#include "gqview_view_dir_tree.h"
#include "typedefs.h"
#include <gimp/gimpaction.h>
#include <gimp/gimpactiongroup.h>
#include "support.h"
#include "mysql/mysql.h"
#include "dh-link.h"
#include "rox/rox_global.h"
#include "rox/dir.h"
#include "main.h"
#include "listview.h"
#include "menu.h"
#include "dnd.h"
#include "file_view.h"
#include "inspector.h"
#include "dh_tree.h"
#include "db/mysql.h"
#include "colour_box.h"
#include "window.h"

extern struct _app app;
extern Filer filer;
extern unsigned debug;

char* categories[] = {"drums", "perc", "bass", "keys", "synth", "strings", "brass", "fx", "impulses"};
 
GtkWidget*        view_details_new                ();
void              view_details_dnd_get            (GtkWidget*, GdkDragContext*, GtkSelectionData*, guint info, guint time, gpointer data);

static gboolean   window_on_destroy               (GtkWidget*, gpointer);
static void       window_on_realise               (GtkWidget*, gpointer);
static void       window_on_allocate              (GtkWidget*, gpointer);
static gboolean   window_on_configure             (GtkWidget*, GdkEventConfigure*, gpointer);
static gboolean   filter_new();
static GtkWidget* scrolled_window_new             ();
static void       window_on_fileview_row_selected (GtkTreeView*, gpointer);
static void       menu__add_to_db                 (GtkMenuItem*, gpointer);
static void       make_fm_menu_actions();
static gboolean   dir_tree_on_link_selected       (GObject*, DhLink*, gpointer data);
static GtkWidget* message_panel__new              ();


struct _accel menu_keys[] = {
	{"Add to database",NULL,        {{(char)'a',    0               },  {0, 0}}, menu__add_to_db, GINT_TO_POINTER(0)},
};

struct _accel window_keys[] = {
	{"Quit",           NULL,        {{(char)'q',    GDK_CONTROL_MASK},  {0, 0}}, on_quit,         GINT_TO_POINTER(0)},
	{"Close",          NULL,        {{(char)'w',    GDK_CONTROL_MASK},  {0, 0}}, on_quit,         GINT_TO_POINTER(0)},
};

static GtkAccelGroup* accel_group = NULL;


gboolean
window_new()
{
/*
GtkWindow
+--GtkVbox
   +--search box
   |  +--label
   |  +--text entry
   |
   +--edit metadata hbox
   |
   +--GtkAlignment
   |  +-GtkHPaned
   |    +--vpaned (main left pane)
   |    |  +--directory tree
   |    |  +--inspector
   |    | 
   |    +--vpaned (main right pane)
   |       +--GtkVBox
   |          +--GtkLabel
   |          +--GtkVPaned
   |             +--scrollwin
   |             |  +--treeview file manager
   |             |
   |             +--scrollwin (right pane)
   |                +--treeview
   |
   +--statusbar hbox
      +--statusbar
      +--statusbar2

*/
	GtkWidget *window = app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT(window), "delete_event", G_CALLBACK(on_quit), NULL);
	g_signal_connect(window, "destroy", G_CALLBACK(window_on_destroy), NULL);

	GtkWidget *vbox = app.vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	GtkWidget *hbox_statusbar = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox_statusbar);
	gtk_box_pack_end(GTK_BOX(vbox), hbox_statusbar, EXPAND_FALSE, FALSE, 0);

	//alignment to give top border to main hpane.
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align1), 2, 1, 0, 0); //top, bottom, left, right.
	gtk_widget_show(align1);
	gtk_box_pack_end(GTK_BOX(vbox), align1, EXPAND_TRUE, TRUE, 0);

	GtkWidget *hpaned = app.hpaned = gtk_hpaned_new();
	gtk_paned_set_position(GTK_PANED(hpaned), 160);
	//gtk_box_pack_end(GTK_BOX(vbox), hpaned, EXPAND_TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(align1), hpaned);
	gtk_widget_show(hpaned);

	GtkWidget* left = left_pane();
	gtk_paned_add1(GTK_PANED(hpaned), left);

	GtkWidget* rhs_vbox = gtk_vbox_new(NON_HOMOGENOUS, 0);
	gtk_widget_show(rhs_vbox);
	gtk_paned_add2(GTK_PANED(hpaned), rhs_vbox);

	gtk_box_pack_start(GTK_BOX(rhs_vbox), message_panel__new(), EXPAND_FALSE, FILL_FALSE, 0);

	//split the rhs in two:
	GtkWidget* r_vpaned = gtk_vpaned_new();
	gtk_paned_set_position(GTK_PANED(r_vpaned), 300);
	gtk_box_pack_start(GTK_BOX(rhs_vbox), r_vpaned, EXPAND_TRUE, TRUE, 0);
	gtk_widget_show(r_vpaned);

	GtkWidget* scroll = app.scroll = scrolled_window_new();
	gtk_paned_add1(GTK_PANED(r_vpaned), scroll);

	listview__new();
	if(0 && !mysql__is_connected()) gtk_widget_set_no_show_all(app.view, true); //dont show main view if no database.
	gtk_container_add(GTK_CONTAINER(app.scroll), app.view);

	//--------

	dbg(2, "making fileview pane...");
	void
	make_fileview_pane()
	{
		GtkWidget* scroll2 = scrolled_window_new();
		gtk_paned_add2(GTK_PANED(r_vpaned), scroll2);

		GtkWidget* file_view = app.fm_view = file_manager__new_window(g_get_home_dir());
		gtk_container_add(GTK_CONTAINER(scroll2), file_view);
		gtk_widget_show(file_view);
		g_signal_connect(G_OBJECT(file_view), "cursor-changed", G_CALLBACK(window_on_fileview_row_selected), NULL);

		make_fm_menu_actions();

		//set up fileview as dnd source:
		gtk_drag_source_set(file_view, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
				dnd_file_drag_types, dnd_file_drag_types_count,
				GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK);
		g_signal_connect(G_OBJECT(file_view), "drag_data_get", G_CALLBACK(view_details_dnd_get), NULL);
	}
	make_fileview_pane();

	GtkWidget* statusbar = app.statusbar = gtk_statusbar_new();
	//gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), TRUE);	//why does give a warning??????
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(statusbar), 5);
	gtk_box_pack_start(GTK_BOX(hbox_statusbar), statusbar, EXPAND_TRUE, FILL_TRUE, 0);
	gtk_widget_show(statusbar);

	GtkWidget *statusbar2 = app.statusbar2 = gtk_statusbar_new();
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar2), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(statusbar2), 5);
	gtk_box_pack_start(GTK_BOX(hbox_statusbar), statusbar2, EXPAND_TRUE, FILL_TRUE, 0);
	gtk_widget_show(statusbar2);

	g_signal_connect(G_OBJECT(window), "realize", G_CALLBACK(window_on_realise), NULL);
	g_signal_connect(G_OBJECT(window), "size-allocate", G_CALLBACK(window_on_allocate), NULL);
	g_signal_connect(G_OBJECT(window), "configure_event", G_CALLBACK(window_on_configure), NULL);

	filter_new();
	tagshow_selector_new();
	tag_selector_new();

	GtkAccelGroup* accel_group = gtk_accel_group_new();
	gboolean mnemonics = FALSE;
	GimpActionGroupUpdateFunc update_func = NULL;
	GimpActionGroup* action_group = gimp_action_group_new("Samplecat-window", "Samplecat-window", "gtk-paste", mnemonics, NULL, update_func);
	make_accels(accel_group, action_group, window_keys, G_N_ELEMENTS(window_keys), NULL);
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	gtk_widget_show_all(window);

	dnd_setup();

	return TRUE;
}


static void
window_on_realise(GtkWidget *win, gpointer user_data)
{
	gtk_tree_view_column_set_resizable(app.col_name, TRUE);
	gtk_tree_view_column_set_resizable(app.col_path, TRUE);
	//gtk_tree_view_column_set_sizing(col1, GTK_TREE_VIEW_COLUMN_FIXED);
}


static void
window_on_allocate(GtkWidget *win, gpointer user_data)
{
	GdkColor colour;
	#define SCROLLBAR_WIDTH_HACK 32
	static gboolean done = FALSE;
	if(!app.view->requisition.width) return;

	if(!done){
		//printf("window_on_allocate(): requisition: wid=%i height=%i\n", app.view->requisition.width, app.view->requisition.height);
		//moved!
		//gtk_widget_set_size_request(win, app.view->requisition.width + SCROLLBAR_WIDTH_HACK, MIN(app.view->requisition.height, 900));
		done = TRUE;
	}else{
		//now reduce the request to allow the user to manually make the window smaller.
		gtk_widget_set_size_request(win, 100, 100);
	}

	colour_get_style_bg(&app.bg_colour, GTK_STATE_NORMAL);
	colour_get_style_fg(&app.fg_colour, GTK_STATE_NORMAL);
	colour_get_style_base(&app.base_colour, GTK_STATE_NORMAL);
	colour_get_style_text(&app.text_colour, GTK_STATE_NORMAL);
	//dbg(0, "app.text_colour=%02x%02x%02x", app.text_colour.red >> 8, app.text_colour.green >> 8, app.text_colour.blue >> 8);

	if(app.colourbox_dirty){
		//put the style colours into the palette:
		if(colour_darker (&colour, &app.fg_colour)) colour_box_add(&colour);
		colour_box_add(&app.fg_colour);
		if(colour_lighter(&colour, &app.fg_colour)) colour_box_add(&colour);
		colour_box_add(&app.bg_colour);
		colour_box_add(&app.base_colour);
		colour_box_add(&app.text_colour);

		if(colour_darker (&colour, &app.base_colour)) colour_box_add(&colour);
		if(colour_lighter(&colour, &app.base_colour)) colour_box_add(&colour);
		if(colour_lighter(&colour, &colour)         ) colour_box_add(&colour);

		colour_get_style_base(&colour, GTK_STATE_SELECTED);
		if(colour_lighter(&colour, &colour)) colour_box_add(&colour);
		if(colour_lighter(&colour, &colour)) colour_box_add(&colour);
		if(colour_lighter(&colour, &colour)) colour_box_add(&colour);

		//add greys:
		gdk_color_parse("#555555", &colour);
		colour_box_add(&colour);
		if(colour_lighter(&colour, &colour)) colour_box_add(&colour);
		if(colour_lighter(&colour, &colour)) colour_box_add(&colour);
		if(colour_lighter(&colour, &colour)) colour_box_add(&colour);

		gdk_color_parse("#5f5eff", &colour); //temp!

		//make modifier colours:
		colour_get_style_bg(&app.bg_colour_mod1, GTK_STATE_NORMAL);
		app.bg_colour_mod1.red   = MIN(app.bg_colour_mod1.red   + 0x1000, 0xffff);
		app.bg_colour_mod1.green = MIN(app.bg_colour_mod1.green + 0x1000, 0xffff);
		app.bg_colour_mod1.blue  = MIN(app.bg_colour_mod1.blue  + 0x1000, 0xffff);

		//set column colours:
		dbg(3, "fg_color: %x %x %x", app.fg_colour.red, app.fg_colour.green, app.fg_colour.blue);

		g_object_set(app.cell1, "cell-background-gdk", &app.bg_colour_mod1, "cell-background-set", TRUE, NULL);
		g_object_set(app.cell1, "foreground-gdk", &app.fg_colour, "foreground-set", TRUE, NULL);

		if(is_similar(&app.bg_colour_mod1, &app.fg_colour, 0xFF)) perr("colours not set properly!");
		dbg(2, "%s %s", gdkcolor_get_hexstring(&app.bg_colour_mod1), gdkcolor_get_hexstring(&app.fg_colour));
		if(app.fm_view) view_details_set_alt_colours(VIEW_DETAILS(app.fm_view), &app.bg_colour_mod1, &app.fg_colour);

		colour_box_update();
		app.colourbox_dirty = FALSE;
	}
}


static gboolean
window_on_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{
	static gboolean window_size_set = FALSE;
	if(!window_size_set){
		//take the window size from the config file, or failing that, from the treeview requisition.
		int width = atoi(app.config.window_width);
		if(!width) width = app.view->requisition.width + SCROLLBAR_WIDTH_HACK;
		int height = atoi(app.config.window_height);
		if(!height) height = MIN(app.view->requisition.height, 900);
		if(width && height){
			dbg(2, "width=%s height=%s", app.config.window_width, app.config.window_height);
			gtk_window_resize(GTK_WINDOW(app.window), width, height);
			window_size_set = TRUE;

			//set the position of the left pane elements.
			//As the allocation is somehow bigger than its container, we just do it v approximately.
			if(app.vpaned && GTK_WIDGET_REALIZED(app.vpaned)){
				//dbg(0, "height=%i %i %i", app.hpaned->allocation.height, app.statusbar->allocation.y, app.inspector->widget->allocation.height);
				guint inspector_y = height - app.hpaned->allocation.y - 210;
				gtk_paned_set_position(GTK_PANED(app.vpaned), inspector_y);

				gtk_paned_set_position(GTK_PANED(app.vpaned2), inspector_y / 2);
			}
		}
	}
	return FALSE;
}


static gboolean
window_on_destroy(GtkWidget* widget, gpointer user_data)
{
	return FALSE;
}


GtkWidget*
left_pane()
{
	app.vpaned = gtk_vpaned_new();
	gtk_widget_show(app.vpaned);

	//make another vpane sitting inside the 1st:
	GtkWidget* vpaned2 = app.vpaned2 = gtk_vpaned_new();
	gtk_widget_show(vpaned2);
	gtk_paned_add1(GTK_PANED(app.vpaned), vpaned2);

	if(mysql__is_connected()){
#ifndef NO_USE_DEVHELP_DIRTREE
		GtkWidget* tree = dir_tree_new();
		GtkWidget* scroll = scrolled_window_new();
		gtk_container_add((GtkContainer*)scroll, tree);
		gtk_paned_add1(GTK_PANED(vpaned2), scroll);
		gtk_widget_show(tree);
		g_signal_connect(tree, "link_selected", G_CALLBACK(dir_tree_on_link_selected), NULL);
#endif
	}

	gint expand = TRUE;
	ViewDirTree* dir_list = app.dir_treeview2 = vdtree_new(g_get_home_dir(), expand);
	vdtree_set_select_func(dir_list, dir_on_select, NULL); //callback
	GtkWidget* fs_tree = dir_list->widget;
	gtk_paned_add2(GTK_PANED(vpaned2), fs_tree);
	gtk_widget_show(fs_tree);

	//alternative dir tree:
#ifdef USE_NICE_GQVIEW_CLIST_TREE
	if(false){
		ViewDirList *dir_list = vdlist_new(app.home_dir);
		GtkWidget* tree = dir_list->widget;
	}
	gint expand = TRUE;
	ViewDirTree *dir_list = vdtree_new(app.home_dir, expand);
	GtkWidget* tree = dir_list->widget;
	gtk_paned_add1(GTK_PANED(app.vpaned), tree);
	gtk_widget_show(tree);
#endif

	inspector_pane();
	gtk_paned_add2(GTK_PANED(app.vpaned), app.inspector->widget);

	return app.vpaned;
}


static gboolean
filter_new()
{
	//search box
	PF;

	if(!app.window) return FALSE; //FIXME make this a macro with printf etc

	GtkWidget* hbox = app.toolbar = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(app.vbox), hbox, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label1 = gtk_label_new("Search");
	gtk_misc_set_padding(GTK_MISC(label1), 5,5);
	gtk_widget_show(label1);
	gtk_box_pack_start(GTK_BOX(hbox), label1, FALSE, FALSE, 0);
	
	GtkWidget *entry = app.search = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 64);
	gtk_entry_set_text(GTK_ENTRY(entry), app.search_phrase);
	gtk_widget_show(entry);	
	gtk_box_pack_start(GTK_BOX(hbox), entry, EXPAND_TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(entry), "focus-out-event", G_CALLBACK(new_search), NULL);
	//this is supposed to enable REUTRN to enter the text - does it work?
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	//g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_entry_activate), NULL);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(new_search), NULL);

	//----------------------------------------------------------------------

	//second row (metadata edit):
	GtkWidget* hbox_edit = app.toolbar2 = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox_edit);
	gtk_box_pack_start(GTK_BOX(app.vbox), hbox_edit, EXPAND_FALSE, FILL_FALSE, 0);

	//left align the label:
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align1);
	gtk_box_pack_start(GTK_BOX(hbox_edit), align1, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label2 = gtk_label_new("Tag");
	gtk_misc_set_padding(GTK_MISC(label2), 5,5);
	gtk_widget_show(label2);
	gtk_container_add(GTK_CONTAINER(align1), label2);	

	//make the two lhs labels the same width:
	GtkSizeGroup* size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget(size_group, label1);
	gtk_size_group_add_widget(size_group, align1);
	
	colour_box_new(hbox_edit);

	return TRUE;
}


static GtkWidget*
scrolled_window_new()
{
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL); //adjustments created automatically.
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scroll);
	return scroll;
}


GtkWidget*
message_panel__add_msg(const gchar* msg, const gchar* stock_id)
{
	//TODO expire old messages. Limit to 5 and add close button?

	GtkWidget* hbox = gtk_hbox_new(FALSE, 2);

	if(stock_id){
		//const gchar* stock_id = GTK_STOCK_DIALOG_WARNING;
		GtkWidget* icon = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_MENU);
		gtk_widget_set_size_request(icon, 16, -1);
		//gtk_image_set_from_stock(GTK_IMAGE(icon), stock_id, GTK_ICON_SIZE_MENU);
		gtk_box_pack_start((GtkBox*)hbox, icon, FALSE, FALSE, 2);
	}

	GtkWidget* label = gtk_label_new(msg);
	gtk_box_pack_start((GtkBox*)hbox, label, FALSE, FALSE, 2);

	gtk_box_pack_start((GtkBox*)app.msg_panel, hbox, FALSE, FALSE, 2);
	return hbox;
}


static GtkWidget*
message_panel__new()
{
	PF;
	GtkWidget* vbox = app.msg_panel = gtk_vbox_new(FALSE, 2);

#if 0
#ifndef USE_TRACKER
	char* msg = db__is_connected() ? "" : "no database available";
#else
	char* msg = "";
#endif
	GtkWidget* hbox = message_panel__add_msg(msg, GTK_STOCK_INFO);
	gtk_box_pack_start((GtkBox*)vbox, hbox, FALSE, FALSE, 2);
#endif

	if(mysql__is_connected()) gtk_widget_set_no_show_all(app.msg_panel, true); //initially hidden.
	return vbox;
}


GtkWidget*
dir_tree_new()
{
	//data:
	update_dir_node_list();

	//view:
	app.dir_treeview = dh_book_tree_new(&app.dir_tree);

	return app.dir_treeview;
}


gboolean
tag_selector_new()
{
	//the tag _edit_ selector

	/*
	//GtkWidget* combo = gtk_combo_box_new_text();
	GtkWidget* combo = app.category = gtk_combo_box_new_text();
	GtkComboBox* combo_ = GTK_COMBO_BOX(combo);
	gtk_combo_box_append_text(combo_, "no categories");
	gtk_combo_box_append_text(combo_, "drums");
	gtk_combo_box_append_text(combo_, "perc");
	gtk_combo_box_append_text(combo_, "keys");
	gtk_combo_box_append_text(combo_, "strings");
	gtk_combo_box_append_text(combo_, "fx");
	gtk_combo_box_append_text(combo_, "impulses");
	gtk_combo_box_set_active(combo_, 0);
	gtk_widget_show(combo);	
	gtk_box_pack_start(GTK_BOX(app.toolbar2), combo, EXPAND_FALSE, FALSE, 0);
	g_signal_connect(combo, "changed", G_CALLBACK(on_category_changed), NULL);
	//gtk_combo_box_get_active_text(combo);
	*/

	GtkWidget* combo2 = app.category = gtk_combo_box_entry_new_text();
	GtkComboBox* combo_ = GTK_COMBO_BOX(combo2);
	gtk_combo_box_append_text(combo_, "no categories");
	int i; for (i=0;i<A_SIZE(categories);i++) {
		gtk_combo_box_append_text(combo_, categories[i]);
	}
	gtk_widget_show(combo2);	
	gtk_box_pack_start(GTK_BOX(app.toolbar2), combo2, EXPAND_FALSE, FALSE, 0);

	//"set" button:
	GtkWidget* set = gtk_button_new_with_label("Set Tag");
	gtk_widget_show(set);	
	gtk_box_pack_start(GTK_BOX(app.toolbar2), set, EXPAND_FALSE, FALSE, 0);
	g_signal_connect(set, "clicked", G_CALLBACK(on_category_set_clicked), NULL);

	return TRUE;
}


gboolean
tagshow_selector_new()
{
	//the view-filter tag-select.

	GtkWidget* combo = app.view_category = gtk_combo_box_new_text();
	GtkComboBox* combo_ = GTK_COMBO_BOX(combo);
	gtk_combo_box_append_text(combo_, "all categories");
	//dbg(0, "  size=%i", sizeof(categories));
	int i; for(i=0;i<A_SIZE(categories);i++){
		gtk_combo_box_append_text(combo_, categories[i]);
	}
	gtk_combo_box_set_active(combo_, 0);
	gtk_widget_show(combo);	
	gtk_box_pack_start(GTK_BOX(app.toolbar), combo, EXPAND_FALSE, FALSE, 0);
	g_signal_connect(combo, "changed", G_CALLBACK(on_view_category_changed), NULL);
	//gtk_combo_box_get_active_text(combo);

	return TRUE;
}


static void
window_on_fileview_row_selected(GtkTreeView* treeview, gpointer user_data)
{
	//a filesystem file has been clicked on. Can we show info for it?
	PF;
	inspector_update_from_fileview(treeview);
}


#define COL_LEAF 0 //api leakage - does the filemanager really have no get_selected_files() function?
void
menu__add_to_db(GtkMenuItem* menuitem, gpointer user_data)
{
	PF;

	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(filer.view));
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(filer.view));
	GList* list = gtk_tree_selection_get_selected_rows(selection, NULL);
	for (; list; list = list->next) {
		GtkTreePath* path = list->data;
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(model, &iter, path)) {
			gchar* leaf;
			gtk_tree_model_get(model, &iter, COL_LEAF, &leaf, -1);
			gchar* filepath = g_build_filename(filer.real_path, leaf, NULL);
			dbg(2, "filepath=%s", filepath);

			add_file(filepath);
			g_free(filepath);
		}
	}
}


static void
make_fm_menu_actions()
{
	GtkActionGroup* group = gtk_action_group_new("File Manager");
	accel_group = gtk_accel_group_new();

	int count = A_SIZE(menu_keys);
	int k;
	for(k=0;k<count;k++){
		struct _accel* key = &menu_keys[k];

    	GtkAction* action = gtk_action_new(key->name, key->name, "Tooltip", key->stock_item? key->stock_item->stock_id : "gtk-file");
  		gtk_action_group_add_action(GTK_ACTION_GROUP(group), action);

    	GClosure* closure = g_cclosure_new(G_CALLBACK(key->callback), key->user_data, NULL);
		g_signal_connect_closure(G_OBJECT(action), "activate", closure, FALSE);
		//dbg(0, "callback=%p", closure->callback);
		gchar path[64]; sprintf(path, "<%s>/Categ/%s", gtk_action_group_get_name(GTK_ACTION_GROUP(group)), key->name);
		//gtk_accel_group_connect(accel_group, key->key[0].code, key->key[0].mask, GTK_ACCEL_MASK, closure);
		gtk_accel_group_connect_by_path(accel_group, path, closure);
		gtk_accel_map_add_entry(path, key->key[0].code, key->key[0].mask);
		gtk_action_set_accel_path(action, path);
		gtk_action_set_accel_group(action, accel_group);

		fm_add_menu_item(action);
	}
}


static gboolean
dir_tree_on_link_selected(GObject *ignored, DhLink *link, gpointer data)
{
	/*
	what does it mean if link->uri is empty?
	*/
	g_return_val_if_fail(link, false);

	dbg(1, "uri=%s", link->uri);
	//FIXME segfault if we use this if()
	//if(strlen(link->uri)){
		update_search_dir(link->uri);
	//}
	return FALSE;
}


