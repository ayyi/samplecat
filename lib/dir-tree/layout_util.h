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


#ifndef LAYOUT_UTIL_H
#define LAYOUT_UTIL_H


void layout_util_sync_thumb(LayoutWindow *lw);
void layout_util_sync(LayoutWindow *lw);


void layout_edit_update_all(void);

void layout_recent_update_all(void);
void layout_recent_add_path(const gchar *path);

void layout_actions_setup(LayoutWindow *lw);
void layout_actions_add_window(LayoutWindow *lw, GtkWidget *window);
GtkWidget *layout_actions_menu_bar(LayoutWindow *lw);

#ifdef GTK4_TODO
GtkWidget *layout_button(GtkWidget *box, gchar **pixmap_data, const gchar *stock_id, gint toggle,
			 GtkTooltips *tooltips, const gchar *tip_text,
			 GtkSignalFunc func, gpointer data);
#endif
GtkWidget *layout_button_bar(LayoutWindow *lw);

void layout_keyboard_init(LayoutWindow *lw, GtkWidget *window);


PixmapFolders *folder_icons_new(void);
void folder_icons_free(PixmapFolders *pf);


void layout_bars_new_image(LayoutWindow *lw);
void layout_bars_new_selection(LayoutWindow *lw, gint count);

GtkWidget *layout_bars_prepare(LayoutWindow *lw, GtkWidget *image);
void layout_bars_close(LayoutWindow *lw);

void layout_bars_maint_renamed(LayoutWindow *lw);


#endif
