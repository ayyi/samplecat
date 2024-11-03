#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "gdl/gdl.h"
#include "debug.h"
#include "gtk/utils.h"

#include <glib.h>

/* ---- end of debugging code */

static void
on_style_button_toggled (GtkCheckButton *button, GdlDock *dock)
{
	GdlDockMaster *master = GDL_DOCK_MASTER (gdl_dock_object_get_master (GDL_DOCK_OBJECT (dock)));
	GdlSwitcherStyle style = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "__style_id"));
	if (gtk_check_button_get_active (button)) {
		g_object_set (master, "switcher-style", style, NULL);
	}
}

static GtkWidget *
create_style_button (GtkWidget *dock, GtkWidget *box, GtkWidget *group, GdlSwitcherStyle style, const gchar *style_text)
{
	GdlDockMaster *master = GDL_DOCK_MASTER (gdl_dock_object_get_master (GDL_DOCK_OBJECT (dock)));

	GdlSwitcherStyle current_style;
	g_object_get (master, "switcher-style", &current_style, NULL);
	GtkWidget* button1 = gtk_check_button_new_with_label (style_text);
	gtk_check_button_set_group (GTK_CHECK_BUTTON (button1), GTK_CHECK_BUTTON (group));
	g_object_set_data (G_OBJECT (button1), "__style_id", GINT_TO_POINTER (style));
	if (current_style == style) {
		gtk_check_button_set_active (GTK_CHECK_BUTTON (button1), TRUE);
	}
	g_signal_connect (button1, "toggled", G_CALLBACK (on_style_button_toggled), dock);
	gtk_box_append (GTK_BOX (box), button1);

	return button1;
}


static GtkWidget *
create_styles_item (GtkWidget *dock)
{

	GtkWidget* vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

	GtkWidget* group = NULL;
	group = create_style_button (dock, vbox1, group, GDL_SWITCHER_STYLE_ICON, "Only icon");
	group = create_style_button (dock, vbox1, group, GDL_SWITCHER_STYLE_TEXT, "Only text");
	group = create_style_button (dock, vbox1, group, GDL_SWITCHER_STYLE_BOTH, "Both icons and texts");
	group = create_style_button (dock, vbox1, group, GDL_SWITCHER_STYLE_TOOLBAR, "Desktop toolbar style");
	group = create_style_button (dock, vbox1, group, GDL_SWITCHER_STYLE_TABS, "Notebook tabs");
	group = create_style_button (dock, vbox1, group, GDL_SWITCHER_STYLE_NONE, "None of the above");

	return vbox1;
}


static GtkWidget *
create_item (const gchar *button_title)
{
	GtkWidget* button1 = gtk_button_new_with_label (button_title);

	return button1;
}


/* creates a simple widget with a textbox inside */
static GtkWidget *
create_text_item ()
{
	GtkWidget* vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

	GtkWidget* scrolledwindow1 = gtk_scrolled_window_new ();
	gtk_box_append (GTK_BOX (vbox1), scrolledwindow1);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#ifdef GTK4_TODO
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_SHADOW_ETCHED_IN);
#endif
	GtkWidget* text = gtk_text_view_new ();
	g_object_set (text, "wrap-mode", GTK_WRAP_WORD, NULL);
	GtkTextBuffer* buffer = gtk_text_view_get_buffer ((GtkTextView*)text);
	gtk_text_buffer_set_text (buffer, "Item", -1);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolledwindow1), text);

	return vbox1;
}

static void
button_dump_cb (GtkWidget *button, gpointer data)
{
	/* Dump XML tree. */
	gdl_dock_layout_save_to_file (GDL_DOCK_LAYOUT (data), "layout.xml");
	g_spawn_command_line_async ("cat layout.xml", NULL);
}

static void
save_layout_cb (GtkWidget *w, gpointer data)
{
#ifdef GTK4_TODO
	GdlDockLayout *layout = GDL_DOCK_LAYOUT (data);

	GtkWidget* dialog = gtk_dialog_new_with_buttons ("New Layout", NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	GtkWidget* hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_action_area (GTK_DIALOG (dialog))), hbox, FALSE, FALSE, 0);

	GtkWidget* label = gtk_label_new ("Name:");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	GtkWidget* entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);

	int response = gtk_dialog_run (GTK_DIALOG (dialog));

	if (response == GTK_RESPONSE_OK) {
		const gchar *name = gtk_entry_get_text (GTK_ENTRY (entry));
		gdl_dock_layout_save_layout (layout, name);
	}

	gtk_widget_destroy (dialog);
#endif
}


static void
on_change_name (GtkWidget* widget, gpointer data)
{
	static int index = 10;
	static gboolean toggle = TRUE;

	gchar* name = g_strdup_printf ("Item %d", index);
	GdlDockItem* item3 = data;
	g_object_set (G_OBJECT (item3), "long_name", name, NULL);
	g_free (name);

	if (toggle) {
		gdl_dock_item_hide_grip (item3);
		g_object_set (G_OBJECT (widget), "label", "hidden", NULL);
		toggle = FALSE;
	} else {
		gdl_dock_item_show_grip (item3);
		g_object_set (G_OBJECT (widget), "label", "shown", NULL);
		toggle = TRUE;
	}
	index++;
}


static GdlDockLayout* layout;

static GtkWidget*
activate (GtkApplication* app, gpointer user_data)
{
	GtkWidget* win = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (win), "Docking widget test");
	gtk_window_set_default_size (GTK_WINDOW (win), 400, 400);

	/* table */
	GtkWidget* table = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
	gtk_window_set_child (GTK_WINDOW (win), table);
	// gtk_container_set_border_width (GTK_CONTAINER (table), 10);

	/* create the dock */
	GtkWidget* dock = gdl_dock_new ();
	GdlDockMaster *master = GDL_DOCK_MASTER (gdl_dock_object_get_master (GDL_DOCK_OBJECT (dock)));
	g_object_set (master, "tab-pos", GTK_POS_TOP, NULL);
	g_object_set (master, "tab-reorderable", TRUE, NULL);

	/* ... and the layout manager */
	layout = gdl_dock_layout_new (G_OBJECT (dock));

	/* create the dockbar */
	GtkWidget* dockbar = gdl_dock_bar_new (G_OBJECT (dock));
	gdl_dock_bar_set_style(GDL_DOCK_BAR(dockbar), GDL_DOCK_BAR_TEXT);

	GtkWidget* box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_valign(box, GTK_ALIGN_FILL);
	gtk_widget_set_halign(box, GTK_ALIGN_FILL);
	gtk_box_append (GTK_BOX (table), box);

	gtk_box_append (GTK_BOX (box), dockbar);
	gtk_box_append (GTK_BOX (box), dock);
	gtk_widget_set_valign(dock, GTK_ALIGN_FILL);
	gtk_widget_set_halign(dock, GTK_ALIGN_FILL);

	/* create the dock items */
	GtkWidget* item1 = gdl_dock_item_new ("item1", "Item #1", GDL_DOCK_ITEM_BEH_LOCKED);
	gdl_dock_item_set_child (GDL_DOCK_ITEM (item1), create_text_item ());
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item1), GDL_DOCK_TOP);

	GtkWidget* item2 = gdl_dock_item_new_with_icon ("item2", "Item #2: Select the switcher style for notebooks", "folder", GDL_DOCK_ITEM_BEH_NORMAL);
	gdl_dock_item_set_child (GDL_DOCK_ITEM (item2), create_styles_item (dock));
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item2), GDL_DOCK_RIGHT);
	g_object_set (item2, "expand", FALSE, NULL);

	GtkWidget* item3 = gdl_dock_item_new_with_icon ("item3", "Item #3 has accented characters (áéíóúñ)", "document-save-symbolic", GDL_DOCK_ITEM_BEH_NORMAL | GDL_DOCK_ITEM_BEH_CANT_CLOSE);
	GtkWidget* name_button = create_item ("Change name");
	gdl_dock_item_set_child (GDL_DOCK_ITEM (item3), name_button);
	g_signal_connect (name_button, "clicked", G_CALLBACK(on_change_name), item3);
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item3), GDL_DOCK_BOTTOM);

	GtkWidget *items [4];
	items [0] = gdl_dock_item_new_with_icon ("Item #4", "Item #4", "view-refresh-symbolic", GDL_DOCK_ITEM_BEH_NORMAL | GDL_DOCK_ITEM_BEH_CANT_ICONIFY);
	gdl_dock_item_set_child (GDL_DOCK_ITEM (items [0]), create_text_item ());
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (items [0]), GDL_DOCK_BOTTOM);

	for (int i = 1; i < 3; i++) {
		gchar name[10];
		snprintf (name, sizeof (name), "Item #%d", i + 4);
		items [i] = gdl_dock_item_new_with_icon (name, name, "go-next-symbolic", GDL_DOCK_ITEM_BEH_NORMAL);
		gdl_dock_item_set_child (GDL_DOCK_ITEM (items [i]), create_text_item ());

		gdl_dock_object_dock (GDL_DOCK_OBJECT (items [0]), GDL_DOCK_OBJECT (items [i]), GDL_DOCK_CENTER, NULL);
	};

	/* tests: manually dock and move around some of the items */
#ifdef GTK4_TODO
	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item3), GDL_DOCK_ITEM (item1), GDL_DOCK_TOP, -1);

	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item2), GDL_DOCK_ITEM (item3), GDL_DOCK_RIGHT, -1);

	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item2), GDL_DOCK_ITEM (item3), GDL_DOCK_LEFT, -1);

	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item2), NULL, GDL_DOCK_FLOATING, -1);
#endif

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_set_homogeneous (GTK_BOX (box), TRUE);
	gtk_box_append (GTK_BOX (table), box);

	GtkWidget* button = gtk_button_new_from_icon_name ("document-save-symbolic");
	g_signal_connect (button, "clicked", G_CALLBACK (save_layout_cb), layout);
	gtk_box_append (GTK_BOX (box), button);

	button = gtk_button_new_with_label ("Dump XML");
	g_signal_connect (button, "clicked", G_CALLBACK (button_dump_cb), layout);
	gtk_box_append (GTK_BOX (box), button);

	gtk_widget_set_visible (win, true);

#ifndef GDL_DISABLE_DEPRECATED
	gdl_dock_placeholder_new ("ph1", GDL_DOCK_OBJECT (dock), GDL_DOCK_TOP, FALSE);
	gdl_dock_placeholder_new ("ph2", GDL_DOCK_OBJECT (dock), GDL_DOCK_BOTTOM, FALSE);
	gdl_dock_placeholder_new ("ph3", GDL_DOCK_OBJECT (dock), GDL_DOCK_LEFT, FALSE);
	gdl_dock_placeholder_new ("ph4", GDL_DOCK_OBJECT (dock), GDL_DOCK_RIGHT, FALSE);
#endif

#ifdef DEBUG
	gboolean on_idle (gpointer master)
	{
		gdl_dock_print (master);
		return G_SOURCE_REMOVE;
	}
	g_idle_add(on_idle, master);
#endif

	return win;
}


int
main (int argc, char **argv)
{
	GtkApplication* app = gtk_application_new ("org.ayyi.test", G_APPLICATION_NON_UNIQUE);
	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

	int status = g_application_run (G_APPLICATION (app), argc, argv);

	g_object_unref (layout);
	g_object_unref (app);

	return status;
}
