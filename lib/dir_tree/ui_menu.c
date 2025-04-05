/*
 * (SLIK) SimpLIstic sKin functions
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "ui_menu.h"


/*
 *-----------------------------------------------------------------------------
 * menu items
 *-----------------------------------------------------------------------------
 */ 

#ifdef GTK4_TODO
static void menu_item_finish(GtkWidget *menu, GtkWidget *item, GCallback func, gpointer data)
{
	if (func) g_signal_connect(G_OBJECT(item), "activate", func, data);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);
}

GtkWidget *menu_item_add(GtkWidget *menu, const gchar *label,
			 GCallback func, gpointer data)
{
	GtkWidget *item;

	item = gtk_menu_item_new_with_mnemonic(label);
	menu_item_finish(menu, item, func, data);

	return item;
}

GtkWidget *menu_item_add_stock(GtkWidget *menu, const gchar *label, const gchar *stock_id,
			       GCallback func, gpointer data)
{
	GtkWidget *item;
	GtkWidget *image;

	item = gtk_image_menu_item_new_with_mnemonic(label);
	image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
	gtk_widget_show(image);
	menu_item_finish(menu, item, func, data);

	return item;
}

GtkWidget *menu_item_add_sensitive(GtkWidget *menu, const gchar *label, gint sensitive,
				   GCallback func, gpointer data)
{
	GtkWidget *item;

	item = menu_item_add(menu, label, func, data);
	gtk_widget_set_sensitive(item, sensitive);

	return item;
}

GtkWidget *menu_item_add_stock_sensitive(GtkWidget *menu, const gchar *label, const gchar *stock_id, gint sensitive,
					 GCallback func, gpointer data)
{
	GtkWidget *item;

	item = menu_item_add_stock(menu, label, stock_id, func, data);
	gtk_widget_set_sensitive(item, sensitive);
	
	return item;
}

GtkWidget *menu_item_add_check(GtkWidget *menu, const gchar *label, gint active,
			       GCallback func, gpointer data)
{
	GtkWidget *item;

	item = gtk_check_menu_item_new_with_mnemonic(label);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), active);
	menu_item_finish(menu, item, func, data);

	return item;
}

void menu_item_add_divider(GtkWidget *menu)
{
	GtkWidget *item = gtk_menu_item_new();
	gtk_widget_set_sensitive(item, FALSE);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);
        gtk_widget_show(item);
}

GtkWidget *menu_item_add_simple(GtkWidget *menu, const gchar *label,
				GCallback func, gpointer data)
{
	GtkWidget *item = gtk_menu_item_new_with_label(label);
	menu_item_finish(menu, item, func, data);

	return item;
}

/*
 *-----------------------------------------------------------------------------
 * popup menus
 *-----------------------------------------------------------------------------
 */ 

static void popup_menu_short_lived_cb(GtkWidget *widget, gpointer data)
{
	/* destroy the menu */
	g_object_unref(G_OBJECT(data));
}

GtkWidget *popup_menu_short_lived(void)
{
	GtkWidget *menu;

	menu = gtk_menu_new();
	g_object_ref(G_OBJECT(menu));
	gtk_object_sink(GTK_OBJECT(menu));
	g_signal_connect(G_OBJECT(menu), "selection_done",
			 G_CALLBACK(popup_menu_short_lived_cb), menu);
	return menu;
}

gint popup_menu_position_clamp(GtkMenu *menu, gint *x, gint *y, gint height)
{
	gint adjusted = FALSE;
	gint w, h;
	gint xw, xh;
	
	w = GTK_WIDGET(menu)->requisition.width;
	h = GTK_WIDGET(menu)->requisition.height;
	xw = gdk_screen_width();
	xh = gdk_screen_height();

	if (*x + w > xw)
		{
		*x = xw - w;
		adjusted = TRUE;
		}
	if (*y + h > xh)
		{
		if (height)
			{
			*y = MAX(0, *y - h - height);
			}
		else
			{
			*y = xh - h;
			}
		adjusted = TRUE;
		};

	if (*x < 0)
		{
		*x = 0;
		adjusted = TRUE;
		}
	if (*y < 0)
		{
		*y = 0;
		adjusted = TRUE;
		}

	return adjusted;
}
#endif
