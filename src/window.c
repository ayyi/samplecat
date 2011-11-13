#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "file_manager.h"
#include "gqview_view_dir_tree.h"
#include <gimp/gimpaction.h>
#include <gimp/gimpactiongroup.h>
#include "typedefs.h"
#include "sample.h"
#include "support.h"
#include "main.h"
#include "listview.h"
#include "file_manager/menu.h"
#include "dnd.h"
#include "file_view.h"
#include "inspector.h"
#ifndef NO_USE_DEVHELP_DIRTREE
#include "dh_link.h"
#include "dh_tree.h"
#else
#include "dir_tree.h"
#endif
#ifdef HAVE_FFTW3
#ifdef USE_OPENGL
#include "gl_spectrogram_view.h"
#else
#include "spectrogram_widget.h"
#endif
#endif
#ifdef USE_MYSQL
#include "db/mysql.h"
#endif
#include "colour_box.h"
#include "auditioner.h"
#include "window.h"

extern struct _app app;
extern Filer filer;
extern unsigned debug;

char* categories[] = {"drums", "perc", "bass", "keys", "synth", "strings", "brass", "fx", "impulse", "breaks"};
 
GtkWidget*        view_details_new                ();
void              view_details_dnd_get            (GtkWidget*, GdkDragContext*, GtkSelectionData*, guint info, guint time, gpointer data);

static GtkWidget* _dir_tree_new                   ();

static gboolean   window_on_destroy               (GtkWidget*, gpointer);
static void       window_on_realise               (GtkWidget*, gpointer);
static void       window_on_size_request          (GtkWidget*, GtkRequisition*, gpointer);
static void       window_on_allocate              (GtkWidget*, GtkAllocation*, gpointer);
static gboolean   window_on_configure             (GtkWidget*, GdkEventConfigure*, gpointer);
static gboolean   filter_new                      ();
static GtkWidget* scrolled_window_new             ();
static void       window_on_fileview_row_selected (GtkTreeView*, gpointer);
static void       on_category_set_clicked         (GtkComboBox*, gpointer);
static void       menu__add_to_db                 (GtkMenuItem*, gpointer);
static void       menu__add_dir_to_db             (GtkMenuItem*, gpointer);
static void       menu__play                      (GtkMenuItem*, gpointer);
static void       make_menu_actions               (struct _accel[], int, void (*add_to_menu)(GtkAction*));
static gboolean   on_dir_tree_link_selected       (GObject*, DhLink*, gpointer);
static GtkWidget* message_panel__new              ();
static GtkWidget* left_pane                       ();
static void       on_layout_changed               ();

static void       k_delete_row                    (GtkAccelGroup*, gpointer);


struct _accel menu_keys[] = {
	{"Add to database",NULL,        {{(char)'a',    0               },  {0, 0}}, menu__add_to_db, GINT_TO_POINTER(0)},
	{"Play"           ,NULL,        {{(char)'p',    0               },  {0, 0}}, menu__play,      NULL              },
};

struct _accel window_keys[] = {
	{"Quit",           NULL,        {{(char)'q',    GDK_CONTROL_MASK},  {0, 0}}, on_quit,         GINT_TO_POINTER(0)},
	{"Close",          NULL,        {{(char)'w',    GDK_CONTROL_MASK},  {0, 0}}, on_quit,         GINT_TO_POINTER(0)},
	{"Delete",         NULL,        {{GDK_Delete,   0               },  {0, 0}}, k_delete_row,    GINT_TO_POINTER(0)},
};

struct _accel fm_tree_keys[] = {
    {"Add to database",NULL,        {{(char)'y',    0               },  {0, 0}}, menu__add_dir_to_db,           NULL},
};

static GtkAccelGroup* accel_group = NULL;


gboolean
window_new()
{
/*
GtkWindow
+--GtkVbox                        app.vbox
   +--search box
   |  +--label
   |  +--text entry
   |
   +--edit metadata hbox
   |
   +--GtkAlignment                align1
   |  +--GtkVPaned
   |     +--GtkHPaned
   |     |  +--file manager tree
   |     |  +--scrollwin
   |     |     +--treeview file manager
   |     +--GtkHPaned
   |        +--vpaned (main left pane)
   |        |  +--directory tree
   |        |  +--inspector
   |        | 
   |        +--vpaned (main right pane)
   |           +--GtkVBox
   |              +--GtkLabel
   |              +--GtkVPaned
   |                 +--scrollwin
   |                 |  +--treeview file manager
   |                 |
   |                 +--scrollwin (right pane)
   |                    +--treeview
   |
   +--statusbar hbox
      +--statusbar
      +--statusbar2

*/
	GtkWidget* window = app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT(window), "delete_event", G_CALLBACK(on_quit), NULL);
	g_signal_connect(window, "destroy", G_CALLBACK(window_on_destroy), NULL);

	GtkWidget* vbox = app.vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	filter_new();
	tagshow_selector_new();
	tag_selector_new();

	//alignment to give top border to main hpane.
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align1), 2, 1, 0, 0); //top, bottom, left, right.
	gtk_box_pack_start(GTK_BOX(vbox), align1, EXPAND_TRUE, TRUE, 0);

	//---------
	GtkWidget* main_vpaned = gtk_vpaned_new();
	//gtk_paned_set_position(GTK_PANED(r_vpaned), 300);
	gtk_container_add(GTK_CONTAINER(align1), main_vpaned);

	GtkWidget* hpaned = app.hpaned = gtk_hpaned_new();
	gtk_paned_set_position(GTK_PANED(hpaned), 210);
	//gtk_container_add(GTK_CONTAINER(align1), hpaned);
	gtk_paned_add1(GTK_PANED(main_vpaned), hpaned);

	GtkWidget* left = left_pane();
	gtk_paned_add1(GTK_PANED(hpaned), left);

	GtkWidget* rhs_vbox = gtk_vbox_new(NON_HOMOGENOUS, 0);
	gtk_paned_add2(GTK_PANED(hpaned), rhs_vbox);

	gtk_box_pack_start(GTK_BOX(rhs_vbox), message_panel__new(), EXPAND_FALSE, FILL_FALSE, 0);

	//split the rhs in two:
	GtkWidget* r_vpaned = gtk_vpaned_new();
	gtk_paned_set_position(GTK_PANED(r_vpaned), 300);
	gtk_box_pack_start(GTK_BOX(rhs_vbox), r_vpaned, EXPAND_TRUE, TRUE, 0);

	app.scroll = scrolled_window_new();
	gtk_paned_add1(GTK_PANED(r_vpaned), app.scroll);

	listview__new();
	if(0 && BACKEND_IS_NULL) gtk_widget_set_no_show_all(app.view, true); //dont show main view if no database.
	gtk_container_add(GTK_CONTAINER(app.scroll), app.view);

	dbg(2, "making fileview pane...");
	void
	make_fileview_pane()
	{
		GtkWidget* fman_hpaned = gtk_hpaned_new();
		gtk_paned_set_position(GTK_PANED(fman_hpaned), 210);
		gtk_paned_add2(GTK_PANED(main_vpaned), fman_hpaned);

		void fman_left()
		{
			void dir_on_select(ViewDirTree *vdt, const gchar *path, gpointer data)
			{
				PF;
				filer_change_to(&filer, path, NULL);
			}

			gint expand = TRUE;
			ViewDirTree* dir_list = app.dir_treeview2 = vdtree_new(g_get_home_dir(), expand);
			vdtree_set_select_func(dir_list, dir_on_select, NULL); //callback
			GtkWidget* fs_tree = dir_list->widget;
			gtk_paned_add1(GTK_PANED(fman_hpaned), fs_tree);

			make_menu_actions(fm_tree_keys, G_N_ELEMENTS(fm_tree_keys), vdtree_add_menu_item);
		}

		void fman_right()
		{
			GtkWidget* scroll2 = scrolled_window_new();
			gtk_paned_add2(GTK_PANED(fman_hpaned), scroll2);

			const char* dir = (app.config.browse_dir && app.config.browse_dir[0] && g_file_test(app.config.browse_dir, G_FILE_TEST_IS_DIR)) ? app.config.browse_dir : g_get_home_dir();
			GtkWidget* file_view = app.fm_view = file_manager__new_window(dir);
			gtk_container_add(GTK_CONTAINER(scroll2), file_view);
			g_signal_connect(G_OBJECT(file_view), "cursor-changed", G_CALLBACK(window_on_fileview_row_selected), NULL);

			void window_on_dir_changed(GtkWidget* widget, gpointer data)
			{
				PF;
			}
			g_signal_connect(G_OBJECT(file_manager__get_signaller()), "dir_changed", G_CALLBACK(window_on_dir_changed), NULL);

			make_menu_actions(menu_keys, G_N_ELEMENTS(menu_keys), fm__add_menu_item);

			//set up fileview as dnd source:
			gtk_drag_source_set(file_view, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
					dnd_file_drag_types, dnd_file_drag_types_count,
					GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK);
			g_signal_connect(G_OBJECT(file_view), "drag_data_get", G_CALLBACK(view_details_dnd_get), NULL);
		}

		fman_left();
		fman_right();
	}
	make_fileview_pane();

#ifdef HAVE_FFTW3
	if(app.view_options[SHOW_SPECTROGRAM].value){
		show_spectrogram(true);

		//gtk_widget_set_no_show_all(app.spectrogram, true);
	}
#endif

	GtkWidget *hbox_statusbar = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), hbox_statusbar, EXPAND_FALSE, FALSE, 0);

	GtkWidget* statusbar = app.statusbar = gtk_statusbar_new();
	//gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), TRUE);	//why does give a warning??????
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(statusbar), 5);
	gtk_box_pack_start(GTK_BOX(hbox_statusbar), statusbar, EXPAND_TRUE, FILL_TRUE, 0);

	GtkWidget *statusbar2 = app.statusbar2 = gtk_statusbar_new();
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar2), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(statusbar2), 5);
	gtk_box_pack_start(GTK_BOX(hbox_statusbar), statusbar2, EXPAND_TRUE, FILL_TRUE, 0);

	g_signal_connect(G_OBJECT(window), "realize", G_CALLBACK(window_on_realise), NULL);
	g_signal_connect(G_OBJECT(window), "size-request", G_CALLBACK(window_on_size_request), NULL);
	g_signal_connect(G_OBJECT(window), "size-allocate", G_CALLBACK(window_on_allocate), NULL);
	g_signal_connect(G_OBJECT(window), "configure_event", G_CALLBACK(window_on_configure), NULL);

	GtkAccelGroup* accel_group = gtk_accel_group_new();
	gboolean mnemonics = FALSE;
	GimpActionGroupUpdateFunc update_func = NULL;
	GimpActionGroup* action_group = gimp_action_group_new("Samplecat-window", "Samplecat-window", "gtk-paste", mnemonics, NULL, update_func);
	make_accels(accel_group, action_group, window_keys, G_N_ELEMENTS(window_keys), NULL);
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	gtk_widget_show_all(window);

	dnd_setup();

	on_layout_changed();

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
window_on_size_request(GtkWidget* widget, GtkRequisition* req, gpointer user_data)
{
	req->height = atoi(app.config.window_height);
}

static void
colourise_boxes()
{
	GdkColor colour;
#if 0 // tim
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
#else // tom
	float hue (float m1, float m2, float h) {
		if (h<0)   { h = h+1.0; }
		if (h>1)   { h = h-1.0; }
		if (h*6<1) { return m1+(m2-m1)*h*6.0; }
		if (h*2<1) { return m2; }
		if (h*3<2) { return m1+(m2-m1)*((2/3.0)-h)*6.0; }
		return m1;
	}

	void hsl2rgb (float h, float s, float l, GdkColor *c) {
		float mr, mg, mb, m1, m2;
		if (s == 0.0) { mr = mg = mb = l; }
		else {
			if (l<=0.5) { m2 = l*(s+1.0); }
			else        { m2 = l+s-(l*s); }
			m1 = l*2.0 - m2;
			mr = hue(m1, m2, (h+(1/3.0)));
			mg = hue(m1, m2, h);
			mb = hue(m1, m2, (h-(1/3.0)));
		}
		c->red   = rintf(65536.0*mr);
		c->green = rintf(65536.0*mg);
		c->blue  = rintf(65536.0*mb);
	}

#define LUMSHIFT (0)
	colour_box_add(&app.bg_colour); // "transparent" - none
	int i;
  for (i=0;i<PALETTE_SIZE-1;i++) {
		float h, s, l; 
		l=1.0; h = (float)i / ((float)PALETTE_SIZE-1.0);
		s=(i%2)?.6:.9; 
		l=0.3 + ((i+LUMSHIFT)%4)/6.0; /* 0.3 .. 0.8 */
		hsl2rgb(h, s, l, &colour);
		if (!colour_box_add(&colour)) {
			fprintf(stderr, "WTF %d\n", i);
		}
	}
#endif
}

static void
window_on_allocate(GtkWidget *win, GtkAllocation *allocation, gpointer user_data)
{
	#define SCROLLBAR_WIDTH_HACK 32
	static gboolean done = FALSE;
	if(!app.view->requisition.width) return;

	if(!done){
		dbg(2, "app.view->requisition: wid=%i height=%i\n", app.view->requisition.width, app.view->requisition.height);
		//moved!
		//gtk_widget_set_size_request(win, app.view->requisition.width + SCROLLBAR_WIDTH_HACK, MIN(app.view->requisition.height, 900));
		done = TRUE;
	}else{
		//now reduce the request to allow the user to manually make the window smaller.
		gtk_widget_set_size_request(win, 100, 100);
	}

#if 0 
	/* XXX do those really need to be repeated? - style changes ? */
	colour_get_style_bg(&app.bg_colour, GTK_STATE_NORMAL);
	colour_get_style_fg(&app.fg_colour, GTK_STATE_NORMAL);
	colour_get_style_base(&app.base_colour, GTK_STATE_NORMAL);
	colour_get_style_text(&app.text_colour, GTK_STATE_NORMAL);
#endif

	static gboolean did_set_colours = FALSE;
	if (!did_set_colours) {
		did_set_colours=TRUE;

		colour_get_style_bg(&app.bg_colour, GTK_STATE_NORMAL);
		colour_get_style_fg(&app.fg_colour, GTK_STATE_NORMAL);
		colour_get_style_base(&app.base_colour, GTK_STATE_NORMAL);
		colour_get_style_text(&app.text_colour, GTK_STATE_NORMAL);

		hexstring_from_gdkcolor(app.config.colour[0], &app.bg_colour);

		if(app.colourbox_dirty){
			colourise_boxes();
			app.colourbox_dirty = FALSE;
		}

		//make modifier colours:
		colour_get_style_bg(&app.bg_colour_mod1, GTK_STATE_NORMAL);
		app.bg_colour_mod1.red   = MIN(app.bg_colour_mod1.red   + 0x1000, 0xffff);
		app.bg_colour_mod1.green = MIN(app.bg_colour_mod1.green + 0x1000, 0xffff);
		app.bg_colour_mod1.blue  = MIN(app.bg_colour_mod1.blue  + 0x1000, 0xffff);

		//set column colours:
		dbg(3, "fg_color: %x %x %x", app.fg_colour.red, app.fg_colour.green, app.fg_colour.blue);

		g_object_set(app.cell1, "cell-background-gdk", &app.bg_colour_mod1, "cell-background-set", TRUE, NULL);
		g_object_set(app.cell1, "foreground-gdk", &app.fg_colour, "foreground-set", TRUE, NULL);
#if 0
		if(is_similar(&app.bg_colour_mod1, &app.fg_colour, 0xFF)) perr("colours not set properly!");
#endif
		dbg(2, "%s %s", gdkcolor_get_hexstring(&app.bg_colour_mod1), gdkcolor_get_hexstring(&app.fg_colour));
		if(app.fm_view) view_details_set_alt_colours(VIEW_DETAILS(app.fm_view), &app.bg_colour_mod1, &app.fg_colour);

		colour_box_update();
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
		int window_height = atoi(app.config.window_height);
		if(!window_height) window_height = MIN(app.view->requisition.height, 900);
		if(width && window_height){
			dbg(2, "setting size: width=%i height=%i", width, window_height);
			gtk_window_resize(GTK_WINDOW(app.window), width, window_height);
			window_size_set = TRUE;

			//set the position of the left pane elements.
			//As the allocation is somehow bigger than its container, we just do it v approximately.
/*
			if(app.vpaned && GTK_WIDGET_REALIZED(app.vpaned)){
				//dbg(0, "height=%i %i %i", app.hpaned->allocation.height, app.statusbar->allocation.y, app.inspector->widget->allocation.height);
				guint inspector_y = height - app.hpaned->allocation.y - 210;
				gtk_paned_set_position(GTK_PANED(app.vpaned), inspector_y);
			}
*/
		}
	}
	return FALSE;
}


static gboolean
window_on_destroy(GtkWidget* widget, gpointer user_data)
{
	return FALSE;
}


static GtkWidget*
left_pane()
{
	app.vpaned = gtk_vpaned_new();

	if(!BACKEND_IS_NULL){
#ifndef NO_USE_DEVHELP_DIRTREE
		GtkWidget* tree = _dir_tree_new();
		GtkWidget* scroll = scrolled_window_new();
		gtk_container_add((GtkContainer*)scroll, tree);
		gtk_paned_add1(GTK_PANED(app.vpaned), scroll);
		g_signal_connect(tree, "link-selected", G_CALLBACK(on_dir_tree_link_selected), NULL);
#else
		GtkWidget* tree = _dir_tree_new();
		GtkWidget* scroll = scrolled_window_new();
		gtk_container_add((GtkContainer*)scroll, tree);
		gtk_paned_add1(GTK_PANED(app.vpaned), scroll);
#endif
	}

	//alternative dir tree:
#ifdef USE_NICE_GQVIEW_CLIST_TREE
	if(false){
		ViewDirList* dir_list = vdlist_new(app.home_dir);
		GtkWidget* tree = dir_list->widget;
	}
	gint expand = TRUE;
	ViewDirTree *dir_list = vdtree_new(app.home_dir, expand);
	GtkWidget* tree = dir_list->widget;
	gtk_paned_add1(GTK_PANED(app.vpaned), tree);
#endif

	/*
	void on_inspector_allocate(GtkWidget* widget, GtkAllocation* allocation, gpointer user_data)
	{
		dbg(0, "req=%i allocation=%i", widget->requisition.height, allocation->height);
		int tot_height = app.vpaned->allocation.height;
		if(allocation->height > widget->requisition.height){
			gtk_paned_set_position(GTK_PANED(app.vpaned), tot_height - widget->requisition.height);
		}
		//increase size:
		if(allocation->height < widget->requisition.height){
			if(allocation->height < tot_height / 2){
				gtk_paned_set_position(GTK_PANED(app.vpaned), MAX(tot_height / 2, tot_height - widget->requisition.height));
			}
		}
	}

	*/

	inspector_new();
	//g_signal_connect(app.inspector->widget, "size-allocate", (gpointer)on_inspector_allocate, NULL);
	gtk_paned_add2(GTK_PANED(app.vpaned), app.inspector->widget);

	void on_vpaned_allocate(GtkWidget* widget, GtkAllocation* vp_allocation, gpointer user_data)
	{
		static int previous_height = 0;
		if(!app.inspector || vp_allocation->height == previous_height) return;

		int inspector_requisition = 240;//app.inspector->vbox->requisition.height; //FIXME
		//dbg(0, "req=%i", inspector_requisition);
		if(!inspector_requisition) return;

		if(!app.inspector->user_height){
			//user has not specified a height so we have free reign

			int tot_height = vp_allocation->height;
			dbg(1, "req=%i tot_allocation=%i %i", inspector_requisition, tot_height, app.inspector->widget->allocation.height);

			//small window - dont allow the inspector to take up more than half the space.
			if(vp_allocation->height < inspector_requisition){
				gtk_paned_set_position(GTK_PANED(app.vpaned), MAX(tot_height / 2, tot_height - inspector_requisition));
			}

			//large window - dont allow the inspector to be too big
			int current_insp_height = tot_height - gtk_paned_get_position(GTK_PANED(app.vpaned));
			if(current_insp_height > inspector_requisition){
				gtk_paned_set_position(GTK_PANED(app.vpaned), tot_height - inspector_requisition);
			}
		}
		previous_height = vp_allocation->height;
	}

	g_signal_connect(app.vpaned, "size-allocate", (gpointer)on_vpaned_allocate, NULL);
	return app.vpaned;
}


static gboolean
filter_new()
{
	//search box
	PF;

	g_return_val_if_fail(app.window, FALSE);

	GtkWidget* hbox = app.toolbar = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(app.vbox), hbox, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label1 = gtk_label_new("Search");
	gtk_misc_set_padding(GTK_MISC(label1), 5,5);
	gtk_box_pack_start(GTK_BOX(hbox), label1, FALSE, FALSE, 0);

	gboolean on_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
	{
		PF;
		const gchar* text = gtk_entry_get_text(GTK_ENTRY(app.search));
		if(strcmp(text, app.search_phrase)){
			strncpy(app.search_phrase, text, 255);
			do_search((gchar*)text, app.search_dir);
		}
		return NOT_HANDLED;
	}

	GtkWidget *entry = app.search = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 64);
	gtk_entry_set_text(GTK_ENTRY(entry), app.search_phrase);
	gtk_box_pack_start(GTK_BOX(hbox), entry, EXPAND_TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(entry), "focus-out-event", G_CALLBACK(on_focus_out), NULL);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_focus_out), NULL);

	//----------------------------------------------------------------------

	//second row (metadata edit):
	GtkWidget* hbox_edit = app.toolbar2 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(app.vbox), hbox_edit, EXPAND_FALSE, FILL_FALSE, 0);

	//left align the label:
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(hbox_edit), align1, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label2 = gtk_label_new("Tag");
	gtk_misc_set_padding(GTK_MISC(label2), 5,5);
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

	if(!BACKEND_IS_NULL) gtk_widget_set_no_show_all(app.msg_panel, true); //initially hidden.
	return vbox;
}


static GtkWidget*
_dir_tree_new()
{
	//data:
	update_dir_node_list();

	//view:
#ifndef NO_USE_DEVHELP_DIRTREE
	app.dir_treeview = dh_book_tree_new(&app.dir_tree);
#else
	app.dir_treeview = dir_tree_new(&app.dir_tree);
#endif

	return app.dir_treeview;
}


gboolean
tag_selector_new()
{
	//the tag _edit_ selector

	GtkWidget* combo2 = app.category = gtk_combo_box_entry_new_text();
	GtkComboBox* combo_ = GTK_COMBO_BOX(combo2);
	gtk_combo_box_append_text(combo_, "no categories");
	int i; for (i=0;i<G_N_ELEMENTS(categories);i++) {
		gtk_combo_box_append_text(combo_, categories[i]);
	}
	gtk_box_pack_start(GTK_BOX(app.toolbar2), combo2, EXPAND_FALSE, FALSE, 0);

	//"set" button:
	GtkWidget* set = gtk_button_new_with_label("Set Tag");
	gtk_box_pack_start(GTK_BOX(app.toolbar2), set, EXPAND_FALSE, FALSE, 0);
	g_signal_connect(set, "clicked", G_CALLBACK(on_category_set_clicked), NULL);

	return TRUE;
}


gboolean
tagshow_selector_new()
{
	//the view-filter tag-select.

	#define ALL_CATEGORIES "all categories"

	GtkWidget* combo = app.view_category = gtk_combo_box_new_text();
	GtkComboBox* combo_ = GTK_COMBO_BOX(combo);
	gtk_combo_box_append_text(combo_, ALL_CATEGORIES);
	int i; for(i=0;i<G_N_ELEMENTS(categories);i++){
		gtk_combo_box_append_text(combo_, categories[i]);
	}
	gtk_combo_box_set_active(combo_, 0);
	gtk_box_pack_start(GTK_BOX(app.toolbar), combo, EXPAND_FALSE, FALSE, 0);

	void
	on_view_category_changed(GtkComboBox *widget, gpointer user_data)
	{
		//update the sample list with the new view-category.
		PF;

		if (app.search_category){ g_free(app.search_category); app.search_category = NULL; }
		char* category = gtk_combo_box_get_active_text(GTK_COMBO_BOX(app.view_category));
		if (strcmp(category, ALL_CATEGORIES)){
			app.search_category = category;
		}
		else g_free(category);

		do_search(app.search_phrase, app.search_dir);
	}
	g_signal_connect(combo, "changed", G_CALLBACK(on_view_category_changed), NULL);

	return TRUE;
}


static void
window_on_fileview_row_selected(GtkTreeView* treeview, gpointer user_data)
{
	//a filesystem file has been clicked on.
	PF;
	inspector_update_from_fileview(treeview);
}


#define FILE_VIEW_COL_FILENAME 11 // see file_manager/file_view.c
static void
menu__add_to_db(GtkMenuItem* menuitem, gpointer user_data)
{
	PF;

	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(filer.view));
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(filer.view));
	GList* l = gtk_tree_selection_get_selected_rows(selection, NULL);
	for (;l;l=l->next) {
		GtkTreePath* path = l->data;
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(model, &iter, path)) {
			gchar* leaf;
			gtk_tree_model_get(model, &iter, FILE_VIEW_COL_FILENAME, &leaf, -1);
			gchar* filepath = g_build_filename(filer.real_path, leaf, NULL);
			dbg(2, "filepath=%s", filepath);

			add_file(filepath);
			g_free(filepath);
		}
	}
	g_list_foreach(l, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(l);
}


static void
menu__add_dir_to_db(GtkMenuItem* menuitem, gpointer user_data)
{
	PF;
	const char* path = vdtree_get_selected(app.dir_treeview2);
	dbg(1, "path=%s", path);
	if(path){
		int n_added = 0;
		add_dir(path, &n_added);
	}
}


void
menu__play(GtkMenuItem* menuitem, gpointer user_data)
{
	PF;
	GList* selected = fm_selected_items(&filer);
	GList* l = selected;
	for(;l;l=l->next){
		char* item = l->data;
		dbg(1, "%s", item);
#if (defined USE_DBUS || defined USE_GAUDITION)
		auditioner_play_path(item);
#endif
		g_free(item);
	}
	g_list_free(selected);
}


static void
make_menu_actions(struct _accel keys[], int count, void (*add_to_menu)(GtkAction*))
{
	//takes the raw definitions and creates actions and optionally menu items for them.

	GtkActionGroup* group = gtk_action_group_new("File Manager");
	accel_group = gtk_accel_group_new();

	int k;
	for(k=0;k<count;k++){
		struct _accel* key = &keys[k];

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

		if(add_to_menu) add_to_menu(action);
	}
}


static gboolean
on_dir_tree_link_selected(GObject *ignored, DhLink *link, gpointer data)
{
	g_return_val_if_fail(link, false);

	dbg(2, "uri=%s", link->uri);
	update_search_dir(link->uri);
	return FALSE;
}


static void
on_category_set_clicked(GtkComboBox *widget, gpointer user_data)
{
	//add selected category to selected samples.
	PF;
	//selected category?
	gchar* category = gtk_combo_box_get_active_text(GTK_COMBO_BOX(app.category));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, NULL);
	if(!selectionlist){ dbg(0, "no files selected."); statusbar_print(1, "no files selected."); return; }

	int i;
	GtkTreeIter iter;
	for(i=0;i<g_list_length(selectionlist);i++){
		GtkTreePath *treepath_selection = g_list_nth_data(selectionlist, i);

		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, treepath_selection)){
			gchar *fname; gchar *tags;
			int id;
			gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_NAME, &fname, COL_KEYWORDS, &tags, COL_IDX, &id, -1);
			dbg(1, "id=%i name=%s", id, fname);

			if(!strcmp(category, "no categories"))
				listmodel__update_by_ref(&iter, COL_KEYWORDS, "");
			else{

				if(!keyword_is_dupe(category, tags)){
					char tags_new[1024];
					snprintf(tags_new, 1024, "%s %s", tags ? tags : "", category);
					g_strstrip(tags_new);//trim

					listmodel__update_by_ref(&iter, COL_KEYWORDS, (void*)tags_new);
				}else{
					dbg(1, "keyword is a dupe - not applying.");
					statusbar_print(1, "ignoring duplicate keyword.");
				}
			}

		} else perr("bad iter! i=%i (<%i)\n", i, g_list_length(selectionlist));
	}
	g_list_foreach(selectionlist, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(selectionlist);

	g_free(category);
}

#ifdef HAVE_FFTW3
void
show_spectrogram(gboolean enable)
{
	if(enable && !app.spectrogram){
#ifdef USE_OPENGL
		app.spectrogram = (GtkWidget*)gl_spectrogram_new();
		gtk_widget_set_size_request(app.spectrogram, 100, 100);
#else
		app.spectrogram = (GtkWidget*)spectrogram_widget_new();
#endif
		gtk_box_pack_start(GTK_BOX(app.vbox), app.spectrogram, EXPAND_TRUE, FILL_TRUE, 0);

		gchar* filename = listview__get_first_selected_filepath();
		dbg(1, "file=%s", filename);
#ifdef USE_OPENGL
		gl_spectrogram_set_file((GlSpectrogram*)app.spectrogram, filename);
#else
		spectrogram_widget_set_file((SpectrogramWidget*)app.spectrogram, filename);
#endif
		g_free(filename);
	}

	if(app.spectrogram){
		if(enable) gtk_widget_show(app.spectrogram);
		else gtk_widget_hide(app.spectrogram);
	}

	on_layout_changed();
}
#endif


static void
on_layout_changed()
{
	//what is the height of the inspector?

	if(app.inspector){
		GtkWidget* widget;
		if((widget = app.inspector->widget)){
			int tot_height = app.vpaned->allocation.height;
			int max_auto_height = tot_height / 2;
			dbg(1, "inspector_height=%i tot=%i", widget->allocation.height, tot_height);
			if(widget->allocation.height < app.inspector->min_height
					&& widget->allocation.height < max_auto_height){
				int inspector_height = MIN(max_auto_height, app.inspector->min_height);
				dbg(1, "setting height : %i/%i", tot_height - inspector_height, inspector_height);
				gtk_paned_set_position(GTK_PANED(app.vpaned), tot_height - inspector_height);
			}
		}
	}
}


static void
k_delete_row(GtkAccelGroup* _, gpointer user_data)
{
	delete_selected_rows();
}
