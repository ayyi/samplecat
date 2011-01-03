#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <gtk/gtk.h>
#include "file_manager.h"
#include "gqview_view_dir_tree.h"
#include <gimp/gimpaction.h>
#include <gimp/gimpactiongroup.h>
#include "typedefs.h"
#include "support.h"
#include "main.h"
#include "listview.h"
#include "dnd.h"
#include "colour_box.h"

extern struct _app app;
extern Filer       filer;
extern unsigned    debug;
static GtkWidget*  clicked_widget = NULL;

static gboolean    colour_box__exists              (GdkColor*);
static void        colour_box__set_colour          (int, GdkColor*);
static gboolean    colour_box__on_event            (GtkWidget*, GdkEvent*, gpointer);
static int         colour_box__drag_dataget        (GtkWidget*, GdkDragContext*, GtkSelectionData*, guint info, guint time, gpointer);
static GtkWidget*  colour_box__make_context_menu   ();
static int         colour_box__lookup_idx          (GtkWidget*);
static void        menu__open_selector             (GtkMenuItem*, gpointer);

struct _colour_box self = {NULL};

static MenuDef _menu_def[] = {
    {"Select Colour", G_CALLBACK(menu__open_selector), GTK_STOCK_SELECT_COLOR},
};


GtkWidget*
colour_box_new(GtkWidget* parent)
{
	GtkWidget* e;
	int i;
	for(i=PALETTE_SIZE-1;i>=0;i--){
		e = app.colour_button[i] = gtk_event_box_new();

		gtk_drag_source_set(e, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK, dnd_file_drag_types, dnd_file_drag_types_count, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
		g_signal_connect(G_OBJECT(e), "drag-data-get", G_CALLBACK(colour_box__drag_dataget), GUINT_TO_POINTER(i));
		g_signal_connect(G_OBJECT(e), "event", G_CALLBACK(colour_box__on_event), GUINT_TO_POINTER(i));

		gtk_widget_set_no_show_all(e, true);

		gtk_box_pack_end(GTK_BOX(parent), e, true, true, 0);
	}

	if(!self.menu) self.menu = colour_box__make_context_menu();

	return e;
}


void
colour_box_update()
{
	//show the current palette colours in the colour_box
	int i;
	GdkColor colour;
	char colour_string[16];
	for(i=PALETTE_SIZE-1;i>=0;i--){
		GtkWidget* widget = app.colour_button[i];
		if(app.colour_button[i] && strlen(app.config.colour[i])){
			snprintf(colour_string, 16, "#%s", app.config.colour[i]);
			if(!gdk_color_parse(colour_string, &colour)){ warnprintf("%s(): %i: parsing of colour string failed. %s\n", __func__, i, colour_string); continue; }
			dbg(2, "%i colour: %x %x %x", i, colour.red, colour.green, colour.blue);

			if(colour.red != app.colour_button[i]->style->bg[GTK_STATE_NORMAL].red) 
				gtk_widget_modify_bg(app.colour_button[i], GTK_STATE_NORMAL, &colour);

			gtk_widget_show(widget);
		}
		else gtk_widget_hide(widget);
	}
}


static gboolean
colour_box__exists(GdkColor* colour)
{
	//returns true if a similar colour already exists in the colour_box.

	GdkColor existing_colour;
	char string[8];
	int i;
	for(i=0;i<PALETTE_SIZE;i++){
		if(strlen(app.config.colour[i])){
			snprintf(string, 8, "#%s", app.config.colour[i]);
			if(!gdk_color_parse(string, &existing_colour)) warnprintf("%s(): parsing of colour string failed (%s).\n", __func__, string);
			if(is_similar(colour, &existing_colour, 0x10)) return true;
		}
	}

	return false;
}


gboolean
colour_box_add(GdkColor* colour)
{
	static unsigned slot = 0;

	//char d[32]; hexstring_from_gdkcolor(d, colour); dbg(0, " %i: %s", slot, d);

	if(slot >= PALETTE_SIZE){ if(debug) warnprintf("%s(): colour_box full.\n", __func__); return false; }

	//only add a colour if a similar colour isnt already there.
	if(colour_box__exists(colour)){ dbg(2, "dup colour - not adding..."); return false; }

	hexstring_from_gdkcolor(app.config.colour[slot++], colour);
	return true;
}


static void
colour_box__set_colour(int i, GdkColor* colour)
{
	g_return_if_fail(i < PALETTE_SIZE);
	g_return_if_fail(app.colour_button[i]);
	hexstring_from_gdkcolor(app.config.colour[i], colour);
	gtk_widget_modify_bg(app.colour_button[i], GTK_STATE_NORMAL, colour);
}


static gboolean
colour_box__on_event(GtkWidget* widget, GdkEvent* event, gpointer user_data)
{
	switch (event->type){
		case GDK_BUTTON_PRESS:
			dbg (1, "button=%i", event->button.type);
			clicked_widget = widget;
			if(event->button.button == 3){
				gtk_menu_popup(GTK_MENU(self.menu), NULL, NULL, 0, event, event->button.button, (guint32)(event->button.time));
				return HANDLED;
			}else{
				dbg (1, "normal button press...");
			}
			break;
		default:
			break;
	}
	return NOT_HANDLED;
}


static int
colour_box__drag_dataget(GtkWidget *widget, GdkDragContext *drag_context, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
	char text[16];
	gboolean data_sent = false;
	PF;

	int box_num = GPOINTER_TO_UINT(user_data); //box_num corresponds to the colour index.

	//convert to a pseudo uri string:
	sprintf(text, "colour:%i%c%c", box_num + 1, 13, 10); //1 based to avoid atoi problems.

	gtk_selection_data_set(data, GDK_SELECTION_TYPE_STRING, BITS_PER_CHAR_8, (guchar*)text, strlen(text));
	data_sent = true;

	return false;
}


static GtkWidget*
colour_box__make_context_menu()
{
	GtkWidget *menu = gtk_menu_new();

	int i; for(i=0;i<G_N_ELEMENTS(_menu_def);i++){
		MenuDef* item = &_menu_def[i];
		GtkWidget* menu_item = gtk_image_menu_item_new_with_label (item->label);
		gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
		if(item->stock_id){
			GtkIconSet* set = gtk_style_lookup_icon_set(gtk_widget_get_style(menu), item->stock_id);
			GdkPixbuf* pixbuf = gtk_icon_set_render_icon(set, gtk_widget_get_style(menu), GTK_TEXT_DIR_LTR, GTK_STATE_NORMAL, GTK_ICON_SIZE_MENU, menu, NULL);

			GtkWidget* ico = gtk_image_new_from_pixbuf(pixbuf);
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), ico);
		}
		if(item->callback) g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(item->callback), GINT_TO_POINTER(i));
	}

	gtk_widget_show_all(menu);
	return menu;
}


static int
colour_box__lookup_idx(GtkWidget* widget)
{
	int i; for(i=0;i<PALETTE_SIZE;i++){
		if(app.colour_button[i] == widget) return i;
	}
	return -1;
}


static void
menu__open_selector(GtkMenuItem* menuitem, gpointer user_data)
{
	dbg(0, "data=%p", user_data);
	void on_colour_change(GtkColorSelection* colorselection, gpointer user_data)
	{
		int box_idx = colour_box__lookup_idx(clicked_widget);
		if(box_idx > -1){
			GdkColor colour;
			gtk_color_selection_get_current_color (colorselection, &colour);
			colour_box__set_colour(box_idx, &colour);
		}
	}

	void on_ok(GtkButton* button, gpointer user_data)
	{
		dbg(0, "...");
		gtk_widget_destroy(gtk_widget_get_toplevel((GtkWidget*)button));
	}

	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget* v = gtk_vbox_new(NON_HOMOGENOUS, 2);
	gtk_container_add((GtkContainer*)window, v);
	GtkWidget* sel = gtk_color_selection_new();
	GtkWidget* b = gtk_button_new_with_label("Ok");
	gtk_box_pack_start((GtkBox*)v, sel, EXPAND_FALSE, FILL_FALSE, 0);
	gtk_box_pack_start((GtkBox*)v, b, EXPAND_FALSE, FILL_FALSE, 0);
	gtk_widget_show_all(window);
	g_signal_connect (G_OBJECT(b), "clicked", G_CALLBACK(on_ok), user_data);
	g_signal_connect (G_OBJECT(sel), "color-changed", G_CALLBACK(on_colour_change), user_data);
}


