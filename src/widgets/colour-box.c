/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://samplecat.orford.org         |
 | copyright (C) 2007-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <math.h>
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "gtk/menu.h"
#include "types.h"
#include "application.h"
#include "colour-box.h"

typedef struct
{
	GtkWidget* menu;
} ColourBox;

#ifdef GTK4_TODO
static GtkWidget*  clicked_widget = NULL;
#endif

static void        colour_box_update               ();
static bool        colour_box_add                  (uint32_t);
#ifdef GTK4_TODO
static void        colour_box__set_colour          (int, uint32_t);
static gboolean    colour_box__on_event            (GtkWidget*, GdkEvent*, gpointer);
static GtkWidget*  colour_box__make_context_menu   ();
static int         colour_box__lookup_idx          (GtkWidget*);
#endif
#ifdef GTK4_TODO
static void        menu__open_selector             (GtkMenuItem*, gpointer);
#endif
#if 0
static gboolean    colour_box__exists              (GdkColor*);
#endif

ColourBox self = {NULL};

#ifdef GTK4_TODO
static MenuDef _menu_def[] = {
    {"Select Colour", G_CALLBACK(menu__open_selector), GTK_STOCK_SELECT_COLOR},
};
#endif

GtkWidget* colour_button[PALETTE_SIZE];
gboolean   colourbox_dirty;


void
colour_box_init ()
{
	for (int i=0;i<PALETTE_SIZE;i++) colour_button[i] = NULL;
	colourbox_dirty = true;
}


GtkWidget*
colour_box_new (GtkWidget* parent)
{
	GtkWidget* window = (GtkWidget*)gtk_application_get_active_window(GTK_APPLICATION(app));

	GtkWidget* e;
	for (int i=PALETTE_SIZE-1;i>=0;i--) {
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		e = colour_button[i] = gtk_color_button_new();
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

		GdkContentProvider* listview__on_drag_prepare (GtkDragSource *source, double x, double y, gpointer i)
		{
			GValue value = G_VALUE_INIT;
			g_value_init (&value, AYYI_TYPE_COLOUR);
			value.data[0].v_int = GPOINTER_TO_INT(i);

			return gdk_content_provider_new_for_value (&value);
		}

		void on_drag_begin (GtkDragSource* source, GdkDrag* drag, GtkWidget* self)
		{
			GdkPaintable* icon = GDK_PAINTABLE(gtk_icon_theme_lookup_icon (icon_theme, "text-x-generic-symbolic", NULL, 32, 1, 0, 0));
			gtk_drag_source_set_icon (source, icon, 0, 0);
			g_object_unref (icon);
		}

		GtkDragSource* drag_source = gtk_drag_source_new ();
		g_signal_connect (drag_source, "prepare", G_CALLBACK (listview__on_drag_prepare), GINT_TO_POINTER(i));
		g_signal_connect (drag_source, "drag-begin", G_CALLBACK (on_drag_begin), e);

		gtk_widget_add_controller (e, GTK_EVENT_CONTROLLER (drag_source));

#ifdef GTK4_TODO
#if 1 // do not allow to change 'neutral' colour.
		if (i > 0)
#endif
		g_signal_connect(G_OBJECT(e), "event", G_CALLBACK(colour_box__on_event), GUINT_TO_POINTER(i));
#endif

#ifdef GTK4_TODO
		gtk_widget_set_visible(e, false);
#endif

		gtk_flow_box_prepend(GTK_FLOW_BOX(parent), e);
	}
#ifdef GTK4_TODO
	if (!self.menu) self.menu = colour_box__make_context_menu();
#endif

	void _on_theme_change (Application* a, char* t, gpointer _){ colour_box_update(); }
	g_signal_connect((gpointer)app, "theme-changed", G_CALLBACK(_on_theme_change), NULL);

	void _config_loaded (Application* a, gpointer _)
	{
		for (int i=0;i<PALETTE_SIZE;i++) {
			if (strcmp(app->config.colour[i], "000000")) {
				colourbox_dirty = false;
				break;
			}
		}
	}
	g_signal_connect((gpointer)app, "config-loaded", G_CALLBACK(_config_loaded), NULL);

	if (gtk_widget_get_realized(window)) {
		colour_box_update();
	} else {
		void window_on_realise (GtkWidget* win, gpointer user_data)
		{
			colour_box_update();
		}
		g_signal_connect(G_OBJECT(window), "realize", G_CALLBACK(window_on_realise), NULL);
	}

#ifdef GTK4_TODO
	//TODO use theme-changed instead?
	void window_on_allocate (GtkWidget* win, GtkAllocation* allocation, gpointer user_data)
	{
		if (colourbox_dirty) {
			colour_box_colourise();
			colourbox_dirty = false;
		}
	}
	g_signal_connect(G_OBJECT(window), "size-allocate", G_CALLBACK(window_on_allocate), NULL);

#endif
	gtk_widget_set_size_request(e, -1, 24);

	return e;
}


/*
 *  Show the current palette colours in the colour_box
 */
static void
colour_box_update ()
{
#ifdef GTK4_TODO
	char colour_string[16];
	for(int i=PALETTE_SIZE-1;i>=0;i--){
		GtkWidget* widget = colour_button[i];
		if (colour_button[i] && strlen(app->config.colour[i])) {
			snprintf(colour_string, 16, "#%s", app->config.colour[i]);
			GdkColor colour;
			if (!gdk_color_parse(colour_string, &colour)) {
				warnprintf("%s(): %i: parsing of colour string failed. %s\n", __func__, i, colour_string);
				continue;
			}
			dbg(2, "%i colour: %x %x %x", i, colour.red, colour.green, colour.blue);

			//if(colour.red != colour_button[i]->style->bg[GTK_STATE_NORMAL].red) 
				gtk_widget_modify_bg(colour_button[i], GTK_STATE_NORMAL, &colour);

			gtk_widget_show(widget);
		}
		else gtk_widget_set_visible(widget, false);
	}
#endif
}


#if NEVER
static gboolean
colour_box__exists (GdkColor* colour)
{
	//returns true if a similar colour already exists in the colour_box.

	GdkColor existing_colour;
	char string[8];
	for (int i=0;i<PALETTE_SIZE;i++) {
		if (strlen(app->config.colour[i])) {
			snprintf(string, 8, "#%s", app->config.colour[i]);
			if (!gdk_color_parse(string, &existing_colour)) warnprintf("%s(): parsing of colour string failed (%s).\n", __func__, string);
			if (is_similar(colour, &existing_colour, 0x10)) return true;
		}
	}

	return false;
}
#endif


static bool
colour_box_add (uint32_t colour)
{
	static unsigned slot = 0;

	//char d[32]; hexstring_from_gdkcolor(d, colour); dbg(0, " %i: %s", slot, d);

	g_return_val_if_fail(slot < PALETTE_SIZE, false);

#if 0 /* don't try to be smarter than the user -- 
       * This can screw up the order or user-configured colours 
			 * if they're too similar.
			 */
	//only add a colour if a similar colour isnt already there.
	if(colour_box__exists(colour)){ dbg(2, "dup colour - not adding..."); return false; }
#endif
#ifdef GTK4_TODO
	app->config.colour[slot++] = colour;
#endif

	return true;
}


#ifdef GTK4_TODO
static void
colour_box__set_colour (int i, uint32_t colour)
{
	g_return_if_fail(i < PALETTE_SIZE);
	g_return_if_fail(colour_button[i]);
	hexstring_from_gdkcolor(app->config.colour[i], colour);
	gtk_widget_modify_bg(colour_button[i], GTK_STATE_NORMAL, colour);
}
#endif


#ifdef GTK4_TODO
static gboolean
colour_box__on_event (GtkWidget* widget, GdkEvent* event, gpointer user_data)
{
	switch (event->type){
		case GDK_BUTTON_PRESS:
			dbg (1, "button=%i", event->button.type);
			clicked_widget = widget;
			if (event->button.button == 3) {
				gtk_menu_popup(GTK_MENU(self.menu), NULL, NULL, 0, event, event->button.button, (guint32)(event->button.time));
				return HANDLED;
			} else {
				dbg (1, "normal button press...");
			}
			break;
		default:
			break;
	}
	return NOT_HANDLED;
}
#endif


#ifdef GTK4_TODO
static GtkWidget*
colour_box__make_context_menu ()
{
	GtkWidget* menu = gtk_menu_new();

	add_menu_items_from_defn(menu, G_N_ELEMENTS(_menu_def), _menu_def, NULL);

	return menu;
}
#endif


#ifdef GTK4_TODO
static int
colour_box__lookup_idx (GtkWidget* widget)
{
	for (int i=0;i<PALETTE_SIZE;i++) {
		if (colour_button[i] == widget) return i;
	}
	return -1;
}
#endif


#ifdef GTK4_TODO
static void
menu__open_selector (GtkMenuItem* menuitem, gpointer user_data)
{
	static bool colour_editing = false;
	dbg(2, "data=%p", user_data);

	if (colour_editing) return;
	colour_editing = true;

	void on_colour_change (GtkColorSelection* colorselection, gpointer user_data)
	{
		int box_idx = colour_box__lookup_idx(clicked_widget);
		if (box_idx > -1) {
			GdkColor colour;
			gtk_color_selection_get_current_color (colorselection, &colour);
			colour_box__set_colour(box_idx, &colour);
		}
	}

	void on_ok (GtkButton* button, gpointer user_data)
	{
		dbg(1, "...");
		gtk_widget_destroy(gtk_widget_get_toplevel((GtkWidget*)button));
	}

	void on_destroy (GtkButton* button, gpointer user_data)
	{
		dbg(1, "...");
		colour_editing = false;
	}
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget* v = gtk_vbox_new(NON_HOMOGENOUS, 2);
	gtk_container_add((GtkContainer*)window, v);
	GtkWidget* sel = gtk_color_selection_new();
#if 1
	int box_idx = colour_box__lookup_idx(clicked_widget);
	GtkStyle *curstyle = gtk_widget_get_style(colour_button[box_idx]);
	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(sel), &curstyle->bg[GTK_STATE_NORMAL]);
#endif
	GtkWidget* b = gtk_button_new_with_label("Ok");
	gtk_box_pack_start((GtkBox*)v, sel, EXPAND_FALSE, FILL_FALSE, 0);
	gtk_box_pack_start((GtkBox*)v, b, EXPAND_FALSE, FILL_FALSE, 0);
	gtk_widget_show_all(window);

	g_signal_connect (G_OBJECT(b), "clicked", G_CALLBACK(on_ok), user_data);
	g_signal_connect (G_OBJECT(b), "destroy", G_CALLBACK(on_destroy), user_data);
	g_signal_connect (G_OBJECT(sel), "color-changed", G_CALLBACK(on_colour_change), user_data);
}
#endif


typedef struct 
{
  guint32 pixel;
  guint16 red;
  guint16 green;
  guint16 blue;
} GdkColor;

static uint32_t
colour_from_gdk (GdkColor* colour)
{
	return colour->red >> 8;
}

void
colour_box_colourise ()
{
	GdkColor colour;
#if 0 // tim
	//put the style colours into the palette:
	if(colour_darker (&colour, &app->fg_colour)) colour_box_add(&colour);
	colour_box_add(&app->fg_colour);
	if(colour_lighter(&colour, &app->fg_colour)) colour_box_add(&colour);
	colour_box_add(&app->bg_colour);
	colour_box_add(&app->base_colour);
	colour_box_add(&app->text_colour);

	if(colour_darker (&colour, &app->base_colour)) colour_box_add(&colour);
	if(colour_lighter(&colour, &app->base_colour)) colour_box_add(&colour);
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
#ifdef GTK4_TODO
	colour_box_add(&((Application*)app)->bg_colour); // "transparent" - none
#endif

	for (int i=0;i<PALETTE_SIZE-1;i++) {
		float h, s, l; 
		l = 1.0; h = (float)i / ((float)PALETTE_SIZE - 1.0);
		s = (i % 2) ? .6 : .9;
		l = 0.3 + ((i + LUMSHIFT)%4) / 6.0; /* 0.3 .. 0.8 */
		hsl2rgb(h, s, l, &colour);
		if (!colour_box_add(colour_from_gdk(&colour))) {
			fprintf(stderr, "WTF %d\n", i);
		}
	}
#endif
}

