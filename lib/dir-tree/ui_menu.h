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


#ifndef UI_MENU_H
#define UI_MENU_H


#ifdef GTK4_TODO
GtkWidget *menu_item_add(GtkWidget *menu, const gchar *label,
			 GCallback func, gpointer data);
GtkWidget *menu_item_add_stock(GtkWidget *menu, const gchar *label, const gchar *stock_id,
			       GCallback func, gpointer data);
GtkWidget *menu_item_add_sensitive(GtkWidget *menu, const gchar *label, gint sensitive,
				   GCallback func, gpointer data);
GtkWidget *menu_item_add_stock_sensitive(GtkWidget *menu, const gchar *label, const gchar *stock_id, gint sensitive,
					 GCallback func, gpointer data);
GtkWidget *menu_item_add_check(GtkWidget *menu, const gchar *label, gint active,
			       GCallback func, gpointer data);
void menu_item_add_divider(GtkWidget *menu);

/* use to avoid mnemonics, for example filenames */
GtkWidget *menu_item_add_simple(GtkWidget *menu, const gchar *label,
                                GCallback func, gpointer data);

GtkWidget *popup_menu_short_lived(void);

/* clamp a menu's position to within the screen
 * if menu will attempt to stay out of region y to y+height
 */
gint popup_menu_position_clamp(GtkMenu *menu, gint *x, gint *y, gint height);
#endif


#endif

