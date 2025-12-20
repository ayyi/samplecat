/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"

#ifdef USE_OPENGL

#include "gdl/gdl-dock-item.h"
#include "gdl/gdl-dock.h"
#include "debug/debug.h"
#include "actors/spinner.h"
#include "ui/actors/hover.h"
#include "waveform/view_plus.h"
#include "application.h"
#include "support.h"

static void update_waveform_view        (Sample*);
static void on_waveform_view_realise    (GtkWidget*, gpointer);
static void on_waveform_view_show       (GtkWidget*, gpointer);
static void on_waveform_view_set_parent (GObject*, GParamSpec*, gpointer);
static void show_waveform               (bool);

static void on_waveform_view_finalize   (gpointer, GObject*);

typedef struct {
   gulong selection_handler;
} C;

static struct _window {
   GtkWidget*     waveform;
   struct {
      AGlActor*   text;
      AGlActor*   spp;
      AGlActor*   spinner;
      AGlActor*   hover;
   }              layers;
} window;


GtkWidget*
waveform_panel_new ()
{
	PF;

	waveform_view_plus_set_gl(agl_get_gl_context());
	WaveformViewPlus* view = waveform_view_plus_new(NULL);
	window.waveform = (GtkWidget*)view;

	window.layers.text = waveform_view_plus_add_layer(view, text_actor(NULL), 3);
	text_actor_set_colour((TextActor*)window.layers.text, 0x000000bb, 0xffffffbb);

	window.layers.spp = waveform_view_plus_add_layer(view, wf_spp_actor(waveform_view_plus_get_actor(view)), 0);

	window.layers.spinner = waveform_view_plus_add_layer(view, agl_spinner(NULL), 0);
	window.layers.hover = waveform_view_plus_add_layer(view, hover_actor(waveform_view_plus_get_actor(view)), 4);

	//waveform_view_plus_add_layer(view, grid_actor(waveform_view_plus_get_actor(view)), 0);
#if 0
	waveform_view_plus_set_show_grid(view, true);
#endif

	void _waveform_on_selection_change (SamplecatModel* m, GParamSpec* pspec, gpointer _c)
	{
		PF;
		g_return_if_fail(window.waveform);
		update_waveform_view(m->selection);
	}

	C* c = g_new0(C, 1);

	c->selection_handler = g_signal_connect((gpointer)samplecat.model, "notify::selection", G_CALLBACK(_waveform_on_selection_change), c);

	g_signal_connect((gpointer)window.waveform, "realize", G_CALLBACK(on_waveform_view_realise), NULL);
	g_signal_connect((gpointer)window.waveform, "show", G_CALLBACK(on_waveform_view_show), NULL);
	g_signal_connect((gpointer)window.waveform, "notify::parent", G_CALLBACK(on_waveform_view_set_parent), NULL);

	void waveform_on_audio_ready (gpointer _, gpointer __)
	{
		bool waveform_is_playing ()
		{
			if (!play->sample) return false;
			if (!((WaveformViewPlus*)window.waveform)->waveform) return false;

			char* path = ((WaveformViewPlus*)window.waveform)->waveform->filename;
			return !strcmp(path, play->sample->full_path);
		}

		void waveform_on_position (GObject* _player, gpointer _)
		{
			if (!play->sample) return;

			if (!((WaveformViewPlus*)window.waveform)->waveform) return;

			if (window.layers.spp) {
				wf_spp_actor_set_time((SppActor*)window.layers.spp, waveform_is_playing() ? play->position : UINT32_MAX);
			}
		}

		void waveform_on_play (GObject* player, gpointer _)
		{
			waveform_on_position(player, _);
		}

		void waveform_on_stop (GObject* _player, gpointer _)
		{
			AGlActor* spp = waveform_view_plus_get_layer((WaveformViewPlus*)window.waveform, 5);
			if (spp) {
				wf_spp_actor_set_time((SppActor*)spp, UINT32_MAX);
			}
		}

		g_signal_connect(play, "play", (GCallback)waveform_on_play, NULL);
		g_signal_connect(play, "stop", (GCallback)waveform_on_stop, NULL);
		g_signal_connect(play, "position", (GCallback)waveform_on_position, NULL);
	}
	am_promise_add_callback(play->ready, waveform_on_audio_ready, NULL);

	g_object_weak_ref((GObject*)window.waveform, on_waveform_view_finalize, c);

	void waveform_on_show ()
	{
	}
	g_signal_connect((gpointer)view, "show", G_CALLBACK(waveform_on_show), NULL);

	void waveform_on_map (GtkWidget* widget, GtkWidget* _)
	{
		show_waveform(true);
	}
	g_signal_connect(G_OBJECT(view), "map", G_CALLBACK(waveform_on_map), NULL);

	void waveform_on_unmap (GtkWidget* widget, GtkWidget* _)
	{
		show_waveform(false);
	}
	g_signal_connect(G_OBJECT(view), "unmap", G_CALLBACK(waveform_on_unmap), NULL);

	return (GtkWidget*)view;
}


static void
on_waveform_view_finalize (gpointer _c, GObject* was)
{
	// The panel will finalized when a new layout is loaded.
	// if DEBUG is enabled, it will also occur at program exit.
	// At exit, is freed in window_on_quit.

	PF;
	C* c = _c;

	if (window.waveform) {
#if 0
		if (window.layers.spinner ){
			//agl_actor__remove_child(window.waveform, window.layers.spinner);
			waveform_view_plus_remove_layer((WaveformViewPlus*)window.waveform, window.layers.spinner);
			window.layers.spinner = NULL;
		}
		if (window.layers.spp) {
			waveform_view_plus_remove_layer((WaveformViewPlus*)window.waveform, window.layers.spp);
			window.layers.spp = NULL;
		}
		AGlActor** layers[] = {&window.layers.text};
		for (int i=0;i<G_N_ELEMENTS(layers);i++) {
			if (*layers[i]) {
				waveform_view_plus_remove_layer((WaveformViewPlus*)window.waveform, *layers[i]);
				*layers[i] = NULL;
			}
		}
#endif
		window.waveform = NULL;
	}

	g_signal_handler_disconnect((gpointer)samplecat.model, c->selection_handler);
	g_free(c);
}


static void
on_loaded (Waveform* w, GError* error, gpointer _view)
{
	WaveformViewPlus* view = _view;

	agl_spinner_stop((AGlSpinner*)window.layers.spinner);

	if (error) {
		AGlActor* text_layer = waveform_view_plus_get_layer(view, 3);
		if (text_layer) {
			text_actor_set_text((TextActor*)text_layer, NULL, g_strdup(error->message));
		}
	}
}


static void
update_waveform_view (Sample* sample)
{
	g_return_if_fail(window.waveform);
	if (!gtk_widget_get_realized(window.waveform)) return; // may be hidden by the layout manager.

	WaveformViewPlus* view = (WaveformViewPlus*)window.waveform;

	agl_spinner_start((AGlSpinner*)window.layers.spinner);

	waveform_view_plus_load_file(view, sample->online ? sample->full_path : NULL, on_loaded, view);

#if 0
	void on_waveform_finalize(gpointer _c, GObject* was)
	{
		dbg(0, "!");
	}
	g_object_weak_ref((GObject*)((WaveformViewPlus*)window.waveform)->waveform, on_waveform_finalize, NULL);
#endif

	dbg(1, "name=%s", sample->name);

	AGlActor* text_layer = waveform_view_plus_get_layer(view, 3);
	if (text_layer) {
		char* text = NULL;
		if (sample->channels) {
			char* ch_str = format_channels(sample->channels);
			char* level  = gain2dbstring(sample->peaklevel);
			char length[64]; format_smpte(length, sample->length);
			char fs_str[32]; samplerate_format(fs_str, sample->sample_rate); strcpy(fs_str + strlen(fs_str), " kHz");

			text = g_strdup_printf("%s  %s  %s  %s", length, ch_str, fs_str, level);

			g_free(ch_str);
			g_free(level);
		}

		text_actor_set_text(((TextActor*)text_layer), g_strdup(sample->name), text);
		//text_actor_set_colour((TextActor*)text_layer, 0x33aaffff, 0xffff00ff);
	}
}


/*
 *   Registered as the show/hide function for the panel
 */
static void
show_waveform (bool enable)
{
	if (window.waveform) {
		gtk_widget_set_visible(gtk_widget_get_parent(window.waveform), enable);
		if (enable) {
			gboolean show_wave ()
			{
				Sample* s;
				if ((s = samplecat.model->selection)) {
					WaveformViewPlus* view = (WaveformViewPlus*)window.waveform;
					if (view) {
						Waveform* w = view->waveform;
						if (!w || strcmp(w->filename, s->full_path)) {
							update_waveform_view(s);
						}
					}
				}
				return G_SOURCE_REMOVE;
			}
			g_idle_add(show_wave, NULL);
		}
	}
}


static void
on_waveform_view_realise (GtkWidget* widget, gpointer user_data)
{
}


static void
on_waveform_view_show (GtkWidget* widget, gpointer user_data)
{
}


static void
on_waveform_view_set_parent (GObject *master, GParamSpec *pspec, gpointer data)
{
}

#endif
