#include "config.h"
#include <stdio.h>
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "application.h"
#include "support.h"
#include "progress_dialog.h"

#ifdef GTK4_TODO

#define NOPROGBARWIN 1

struct _progres {
	GtkWidget *bar;
	GtkWidget *win;
	GtkWidget *label;
	GtkWidget *btn_ok, *btn_yes, *btn_no;
	gboolean aborted;
	int btn;
	int tics;
};

static struct _progres pw = {NULL, NULL, NULL, NULL, NULL, NULL, false, 0, 0};


static gboolean
on_destroy (GtkWidget* w, gpointer p) {
	gtk_widget_destroyed(w,p);
	pw.win=NULL;
	return FALSE;
}

static gboolean
on_abort (GtkWidget* w, gpointer p) {
	pw.aborted=true;
	return FALSE;
}

static gboolean
on_btn (GtkWidget* w, gpointer p) {
	pw.btn=GPOINTER_TO_INT(p);
	return FALSE;
}

GtkWidget*
progress_win_new (GtkWidget *parent, gchar *title)
{
	GtkWidget* window = pw.win= gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), title);
	gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(parent));
	gtk_window_set_destroy_with_parent(GTK_WINDOW(window), true);
	gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER_ON_PARENT);

	GtkWidget* v = gtk_vbox_new(FALSE, 2);
	gtk_container_add((GtkContainer*)window, v);

	GtkWidget* lb = pw.label = gtk_label_new(title);
	gtk_box_pack_start((GtkBox*)v, lb, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* pb = pw.bar = gtk_progress_bar_new();
	gtk_box_pack_start((GtkBox*)v, pb, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* h = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start((GtkBox*)v, h, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* ab = gtk_button_new_with_label("Abort");
	gtk_box_pack_start((GtkBox*)h, ab, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* ok = pw.btn_ok = gtk_button_new_with_label("OK");
	gtk_box_pack_start((GtkBox*)h, ok, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* no = pw.btn_no = gtk_button_new_with_label("No");
	gtk_box_pack_start((GtkBox*)h, no, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* yes = pw.btn_yes = gtk_button_new_with_label("Yes");
	gtk_box_pack_start((GtkBox*)h, yes, EXPAND_FALSE, FILL_FALSE, 0);

	gtk_widget_show_all(window);
	gtk_widget_hide(ok);
	gtk_widget_hide(no);
	gtk_widget_hide(yes);

	g_signal_connect (G_OBJECT(window), "delete_event", G_CALLBACK(on_destroy), NULL);
	g_signal_connect (G_OBJECT(ab), "clicked", G_CALLBACK(on_abort), NULL);
	g_signal_connect (G_OBJECT(ok), "clicked", G_CALLBACK(on_btn), GINT_TO_POINTER(1));
	g_signal_connect (G_OBJECT(yes),"clicked", G_CALLBACK(on_btn), GINT_TO_POINTER(1));
	g_signal_connect (G_OBJECT(no), "clicked", G_CALLBACK(on_btn), GINT_TO_POINTER(2));
	g_signal_connect (G_OBJECT(ab), "destroy", G_CALLBACK(on_destroy), NULL);

	pw.tics=0;
	return window;
}


void
set_progress (int cur, int all)
{
	GtkProgressBar *p = GTK_PROGRESS_BAR(pw.bar);
	gchar txt[64];
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

#define DEFAULT_TEXT "Referencing files.."

int 
do_progress (int cur, int all)
{
	if (pw.aborted) return 1;
	pw.tics++;
#ifndef NOPROGBARWIN
	if (!pw.win) {
		pw.win = progress_win_new(app->window, DEFAULT_TEXT);
		pw.tics=0;
	}
	set_progress(cur, all);
#endif
	statusbar_print(2, "referencing files.%s%s", (pw.tics&2)?"..":"", (pw.tics&1)?".":"");
	int needlock = !pthread_equal(pthread_self(), app->gui_thread);
	if(needlock) gdk_threads_enter();
	while (gtk_events_pending()) gtk_main_iteration();
	if(needlock) gdk_threads_leave();
	return 0;
}

void
hide_progress ()
{
	if (pw.win) {
		gtk_widget_destroy(pw.win);
	}
	statusbar_print(2, PACKAGE_NAME". Version "PACKAGE_VERSION);
	pw.win = NULL;
	pw.aborted = false;
}

int
do_progress_question (gchar *msg /* TODO: question-ID, config, options */ )
{
	if (!pw.win) {
		pw.win = progress_win_new(app->window, DEFAULT_TEXT);
	}

	/* TODO 
	 * currently: it is basic working skeleton 
	 */
	int btnoption = 2|4;

	// check config: "do this to all" -> early return

	// prepare window
	gtk_widget_hide(pw.bar);

	// set question/info text.
	gtk_label_set_text(GTK_LABEL(pw.label), msg);

	// add radio or checkboxes (choose file, action, options)
	// add [YES|NO|OK]  buttons

	pw.btn = 0;
	if (btnoption & 1) gtk_widget_show(pw.btn_ok);
	if (btnoption & 2) gtk_widget_show(pw.btn_no);
	if (btnoption & 3) gtk_widget_show(pw.btn_yes);

	/* Wait for decision */
	gtk_window_set_modal(GTK_WINDOW(pw.win), true);
	while (pw.win && pw.btn == 0 && !pw.aborted) {
		int needlock = !pthread_equal(pthread_self(), app->gui_thread);
		if(needlock) gdk_threads_enter();
		while (gtk_events_pending ()) gtk_main_iteration ();
		if(needlock) gdk_threads_leave();
		usleep(10000);
	}
	if (!pw.win) return 0; ///< window was closed: abort.
	///NOTE: IF pw.aborted THEN pw.btn=0

	// pre-parse decision - fi. "repeat this decision for all"

	/* clean up window */
	gtk_window_set_modal(GTK_WINDOW(pw.win), false);
	gtk_widget_hide(pw.btn_ok);
	gtk_widget_hide(pw.btn_no);
	gtk_widget_hide(pw.btn_yes);
	gtk_widget_show(pw.bar);
	gtk_label_set_text(GTK_LABEL(pw.label), DEFAULT_TEXT);
#ifdef NOPROGBARWIN
	gtk_widget_destroy(pw.win);
	pw.win = NULL;
#endif
	
	// return decision 
	return pw.btn;
}
#endif
