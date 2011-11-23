#include "config.h"
#include <stdio.h>
#include <gtk/gtk.h>
#include "main.h"
#include "support.h"
#include "progress_dialog.h"

struct _progres {
	GtkWidget *bar;
	GtkWidget *win;
	gboolean aborted;
	int tics;
};

static struct _progres pw = {NULL, NULL, false};


static gboolean on_destroy (GtkWidget* w, gpointer p) {
	gtk_widget_destroyed(w,p);
	pw.win=NULL;
	pw.aborted=false;
	return FALSE;
}

static gboolean on_abort (GtkWidget* w, gpointer p) {
	pw.aborted=true;
	return FALSE;
}

GtkWidget*
progress_win_new(GtkWidget *parent, gchar *title)
{
	GtkWidget* window = pw.win= gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), title);
	gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(parent));
	gtk_window_set_destroy_with_parent(GTK_WINDOW(window), true);
	gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER_ON_PARENT);

	GtkWidget* v = gtk_vbox_new(NON_HOMOGENOUS, 2);
	gtk_container_add((GtkContainer*)window, v);

	GtkWidget* lb = gtk_label_new(title);
	gtk_box_pack_start((GtkBox*)v, lb, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* pb = pw.bar = gtk_progress_bar_new();
	gtk_box_pack_start((GtkBox*)v, pb, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* ab = gtk_button_new_with_label("Abort");
	gtk_box_pack_start((GtkBox*)v, ab, EXPAND_FALSE, FILL_FALSE, 0);
	pw.aborted=false;

	gtk_widget_show_all(window);
	g_signal_connect (G_OBJECT(window), "delete_event", G_CALLBACK(on_destroy), NULL);
	g_signal_connect (G_OBJECT(ab), "clicked", G_CALLBACK(on_abort), NULL);
	g_signal_connect (G_OBJECT(ab), "destroy", G_CALLBACK(on_destroy), NULL);
	return window;
}

void
set_progress(int cur, int all)
{
	GtkProgressBar *p = GTK_PROGRESS_BAR(pw.bar);
	gchar txt[64];
	pw.tics++;
	if (all < 1 || cur > all) {
		gtk_progress_bar_set_pulse_step(p, .1);
		sprintf(txt, "%d", pw.tics);
		gtk_progress_bar_set_text(p,txt);
		gtk_progress_bar_pulse(p);
	} else {
		sprintf(txt, "%d/%d", cur, all);
		gtk_progress_bar_set_text(p,txt);
		gtk_progress_bar_set_fraction(p, (double)cur / (double) all);
	}
}

/*****************************************************************************/

int 
do_progress(int cur, int all) 
{
	dbg(0, "..");
	if (pw.aborted) return 1;
	if (!pw.win) {
		pw.win=progress_win_new(app.window, "Referencing files..");
		pw.tics=0;
	}
  set_progress(cur, all);
	while (gtk_events_pending ()) gtk_main_iteration ();
	return 0;
}

void
hide_progress()
{
	dbg(0, "..");
	if (pw.win) {
		gtk_widget_destroy(pw.win);
	}
	pw.win=NULL;
	pw.aborted=false;
}
