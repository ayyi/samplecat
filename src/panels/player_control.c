/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2024 Tim Orford <tim@orford.org>                  |
 | copyright (C) 2011 Robin Gareus <robin@gareus.org>                   |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <math.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include <debug/debug.h>
#include "player/player.h"
#include "support.h"
#include "application.h"

#if (defined HAVE_JACK)
  #include "player/jack_player.h"
#endif

#include "player_control.h"

struct _PlayCtrl
{
	GtkWidget* widget;

	GtkWidget* slider1; // player position
	GtkWidget* slider2; // player pitch
	GtkWidget* slider3; // player speed
	GtkWidget* cbfx;    // player enable FX
#ifdef VARISPEED
	GtkWidget* cblnk;   // link speed/pitch
#endif
	struct {
	   GtkWidget* box;   // playback control box (pause/stop/next)
	   GtkWidget* pause; // playback pause button
	   GtkWidget* next;
	}          pb;
};

static void  pc_add_widgets       ();

static void  playpause_toggled    (GtkToggleButton*, gpointer);

static guint slider1sigid;
static void  slider_value_changed (GtkRange*, gpointer);

#if (defined ENABLE_LADSPA)
static guint slider2sigid, slider3sigid;

static void  speed_value_changed  (GtkRange*, gpointer);
static void  pitch_value_changed  (GtkRange*, gpointer);
static void  pitch_toggled        (GtkToggleButton*, gpointer);
#ifdef VARISPEED
static void  cb_link_toggled      (GtkToggleButton*, gpointer);
#endif
#endif


GtkWidget*
player_control_new ()
{
	g_return_val_if_fail(!app->playercontrol, NULL);

	PlayCtrl* pc = app->playercontrol = SC_NEW(PlayCtrl,
		.widget = ({
			GtkWidget* widget = gtk_scrolled_window_new(NULL, NULL);
			gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
			gtk_widget_set_size_request(widget, -1, 40);
			widget;
		}),
	);

	void pc_on_audio_ready (gpointer user_data, gpointer _)
	{
		pc_add_widgets();
#ifdef USE_GDL // always visible
		PlayCtrl* pc = app->playercontrol;
		if (pc->slider1) gtk_widget_set_sensitive(pc->slider1, false);

		gtk_widget_show_all(app->playercontrol->widget);
		player_control_on_show_hide(true);
#endif
	}

	am_promise_add_callback(play->ready, pc_on_audio_ready, NULL);

	return pc->widget;
}


static void
pc_add_widgets ()
{
	PlayCtrl* pc = app->playercontrol;

	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pc->widget), vbox);

#ifndef USE_GDL
	GtkWidget* label1 = gtk_label_new("Player Control");
	gtk_box_pack_start(GTK_BOX(vbox), label1, EXPAND_FALSE, FILL_TRUE, 0);
#endif

	#define BORDERMARGIN (2)
	#define MarginLeft (5)

	GtkWidget* align (GtkWidget* widget)
	{
		GtkWidget* align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
		gtk_alignment_set_padding(GTK_ALIGNMENT(align), 0, 0, MarginLeft - BORDERMARGIN, 0);
		gtk_container_add(GTK_CONTAINER(align), widget);
		return align;
	}

	if (play->auditioner->position && play->auditioner->seek) { // JACK seek-slider
		GtkWidget* slider = pc->slider1 = gtk_hscale_new_with_range(0.0, 1.0, 1.0 / (float)OVERVIEW_WIDTH);
		gtk_widget_set_size_request(slider, OVERVIEW_WIDTH + BORDERMARGIN + BORDERMARGIN, -1);
		gtk_widget_set_tooltip_text(slider, "playhead position / seek");
		gtk_box_pack_start(GTK_BOX(vbox), align(slider), EXPAND_FALSE, FILL_FALSE, 0);

		slider1sigid = g_signal_connect((gpointer)slider, "value-changed", G_CALLBACK(slider_value_changed), NULL);

		gchar* format_value (GtkScale* scale, gdouble value)
		{
			int mins = (int)(value / 1000.) / 60;
			int secs = (int)value / 1000 - 60 * mins;
			int sub = ((int)value % 1000) / 10;
			return g_strdup_printf("%i:%02i:%02i", mins, secs, sub);
		}
		g_signal_connect((gpointer)slider, "format-value", G_CALLBACK(format_value), NULL);
	}

	if (!(play->auditioner->position && play->auditioner->seek)) {
		static char* zero_string = "<span color=\"white\" size=\"200%\">00:00</span> 000";
		GtkWidget* label = gtk_label_new(zero_string);
		gtk_label_set_use_markup((GtkLabel*)label, true);
		gtk_widget_set_name(label, "spp");
		GtkWidget* wrapper = gtk_event_box_new();
		gtk_widget_modify_bg(wrapper, GTK_STATE_NORMAL, &(GdkColor){0,});
		gtk_container_add(GTK_CONTAINER(wrapper), align(label));
		gtk_box_pack_start(GTK_BOX(vbox), wrapper, EXPAND_FALSE, FILL_FALSE, 0);

		void on_position (GObject* player, void* label)
		{
			static guint timer = 0;
			static guint inactivity = 0;

			static int prev_position = 0;
			static int64_t offset = 0;

			gboolean on_timeout (void* label)
			{
				int t = 0;
				static int dt = 0;

				if (play->status != PLAY_PLAYING) {
					g_source_remove(inactivity);
					timer = 0;
					inactivity = 0;
					gtk_label_set_markup((GtkLabel*)label, zero_string);
					return G_SOURCE_REMOVE;
				}

				int64_t wall = g_get_monotonic_time() / 1000;
				int64_t tnow = wall - offset;
				if (play->position == prev_position) {
					t = tnow + dt / 2;
					dt /= 2;
				} else {
					dt = tnow - play->position;
					if (ABS(dt) > 4000) {
						t = play->position;
						dt = 0;
					} else {
						t = play->position + dt / 2;
					}
					offset = wall - t;
					prev_position = play->position;
				}

				int minutes = t / 1000 / 60;
				int secs = t / 1000 - minutes * 60;
				int ms = t - 1000 * (secs + minutes * 60);

				char str[72];
				snprintf(str, 71, "<span color=\"#eee\"><span size=\"200%%\">%02i:%02i</span> %03i</span>", minutes % 1000, secs % 100, ms % 1000);
				gtk_label_set_markup((GtkLabel*)label, str);

				return G_SOURCE_CONTINUE;
			}

			gboolean keep_alive (void* label)
			{
				int64_t wall = g_get_monotonic_time() / 1000;
				int64_t tnow = wall - offset;

				if (tnow - prev_position > 3000) {
					g_source_remove(timer);
					timer = 0;
					inactivity = 0;

					return G_SOURCE_REMOVE;
				}

				return G_SOURCE_CONTINUE;
			}

			if (!timer) timer = g_timeout_add(100, on_timeout, label);
			if (!inactivity) inactivity = g_timeout_add_seconds(4, keep_alive, label);
		}
		g_signal_connect(play, "position", (GCallback)on_position, label);
	}
	/* play ctrl buttons */
	pc->pb.box = gtk_hbox_new(FALSE, 10);
	gtk_widget_set_sensitive(pc->pb.box, false);
	gtk_box_pack_start(GTK_BOX(vbox), pc->pb.box, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* pb1 = gtk_button_new_from_stock (GTK_STOCK_MEDIA_STOP);
	gtk_widget_set_size_request(pb1, 100, -1);
	gtk_box_pack_start(GTK_BOX(pc->pb.box), pb1, EXPAND_FALSE, FILL_TRUE, 0);
	void _stop(GtkButton* button, gpointer _) { player_stop(); }
	g_signal_connect((gpointer)pb1, "clicked", G_CALLBACK(_stop), NULL);

	if (play->auditioner->playpause) {
		pc->pb.pause = gtk_toggle_button_new_with_label(GTK_STOCK_MEDIA_PAUSE);
		gtk_button_set_use_stock ((GtkButton*)pc->pb.pause, true);
		gtk_widget_set_size_request(pc->pb.pause, 100, -1);
		gtk_box_pack_start(GTK_BOX(pc->pb.box), pc->pb.pause, EXPAND_FALSE, FILL_TRUE, 0);
		g_signal_connect((gpointer)pc->pb.pause, "toggled", G_CALLBACK(playpause_toggled), NULL);
	}

	if (play->auditioner->position && play->auditioner->seek) {

#if (defined ENABLE_LADSPA)
	/* note: we could do speed-changes w/o LADSPA, but it'd be EVEN MORE ifdefs */

	/* pitch LADSPA/rubberband */
	GtkWidget* align10 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align10), 0, 0, MarginLeft-BORDERMARGIN, 0);
	gtk_box_pack_start(GTK_BOX(vbox), align10, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* slider2 = pc->slider2 = gtk_hscale_new_with_range(-1200, 1200, 1.0);
	gtk_widget_set_size_request(slider2, OVERVIEW_WIDTH + BORDERMARGIN + BORDERMARGIN, -1);
	gtk_range_set_value(GTK_RANGE(slider2), play->effect_param[0]);
	gtk_widget_set_tooltip_text(slider2, "Pitch in Cents (1/100 semitone)");
	//gtk_scale_set_draw_value(GTK_SCALE(slider2), false);
	gtk_container_add(GTK_CONTAINER(align10), slider2);

	/* effect ctrl buttons */
	GtkWidget* cb0 = pc->cbfx = gtk_check_button_new_with_label("enable pitch-shift");
	gtk_widget_set_tooltip_text(cb0, "Pitch-shifting (rubberband) will decrease playback-quality and increase CPU load.");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(cb0), play->enable_effect);
	gtk_box_pack_start(GTK_BOX(vbox), cb0, EXPAND_FALSE, FILL_FALSE, 0);
	g_signal_connect((gpointer)cb0, "toggled", G_CALLBACK(pitch_toggled), NULL);
#ifdef VARISPEED
	GtkWidget* cb1 = pc->cblnk = gtk_check_button_new_with_label("preserve pitch.");
	gtk_widget_set_tooltip_text(cb1, "link the two sliders to preserve the original pitch.");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(cb1), true);
	gtk_widget_set_sensitive(cb1, play->enable_effect);
	gtk_box_pack_start(GTK_BOX(vbox), cb1, EXPAND_FALSE, FILL_FALSE, 0);
	g_signal_connect((gpointer)cb1, "toggled", G_CALLBACK(cb_link_toggled), NULL);
#endif

	/* speed - samplerate */
	GtkWidget* align11 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align11), 0, 0, MarginLeft - BORDERMARGIN, 0);
	gtk_box_pack_start(GTK_BOX(vbox), align11, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* slider3 = pc->slider3 = gtk_hscale_new_with_range(0.5,2.0,.01);
	gtk_widget_set_size_request(slider3, OVERVIEW_WIDTH + BORDERMARGIN + BORDERMARGIN, -1);
	gtk_range_set_inverted(GTK_RANGE(slider3),true);
	gtk_widget_set_tooltip_text(slider3, "Playback speed factor.");
	//gtk_scale_set_draw_value(GTK_SCALE(slider3), false);
	gtk_range_set_value(GTK_RANGE(slider3), play->playback_speed);
	gtk_container_add(GTK_CONTAINER(align11), slider3);

	slider2sigid = g_signal_connect((gpointer)slider2, "value-changed", G_CALLBACK(pitch_value_changed), slider3);
	slider3sigid = g_signal_connect((gpointer)slider3, "value-changed", G_CALLBACK(speed_value_changed), slider2);
#endif /* LADSPA-rubberband/VARISPEED */
	}

	if (play->auditioner->set_level) {
		GtkWidget* slider = gtk_hscale_new_with_range(0., 100., 1.);
		gtk_widget_set_size_request(slider, OVERVIEW_WIDTH + BORDERMARGIN + BORDERMARGIN, -1);
		gtk_widget_set_tooltip_text(slider, "Volume");
		gtk_range_set_value(GTK_RANGE(slider), 100.);
		gtk_box_pack_start(GTK_BOX(vbox), align(slider), EXPAND_FALSE, FILL_FALSE, 0);

		void level_slider_changed (GtkRange* range, gpointer user_data)
		{
			player_set_level(gtk_range_get_value(range));
		}
		/*slider2sigid = */g_signal_connect((gpointer)slider, "value-changed", G_CALLBACK(level_slider_changed), NULL);
	}

	pc->pb.next = ({
		GtkWidget* widget = gtk_button_new_with_label(GTK_STOCK_MEDIA_NEXT);
		gtk_button_set_use_stock ((GtkButton*)widget, true);
		gtk_widget_set_size_request(widget, 100, -1);
		gtk_box_pack_start(GTK_BOX(pc->pb.box), widget, EXPAND_FALSE, FILL_TRUE, 0);

		g_signal_connect((gpointer)widget, "clicked", G_CALLBACK(play->next), NULL);
		widget;
	});
}


/* JACK auditioner - UI */

#if (defined ENABLE_LADSPA)
static void
pitch_value_changed (GtkRange* range, gpointer user_data)
{
	float v =  gtk_range_get_value(range);
	play->effect_param[0] = (((int)v+1250)%100)-50;
	play->effect_param[1] = floorf((v+50.0)/100.0);
	//dbg(0," %f -> st:%.1f cent:%.1f", v, app->effect_param[1], app->effect_param[0]);
#ifdef VARISPEED
	if (play->link_speed_pitch) {
		double spd = pow(exp2(1.0/12.0), v/100.0);
		play->playback_speed = spd;
		g_signal_handler_block(GTK_RANGE(user_data), slider3sigid);
		gtk_range_set_value(GTK_RANGE(user_data), spd);
		g_signal_handler_unblock(GTK_RANGE(user_data), slider3sigid);
	}
#endif
}


static void
speed_value_changed (GtkRange* range, gpointer user_data)
{
	float v = gtk_range_get_value(range);
	play->playback_speed = v;
#ifdef VARISPEED
	if (play->link_speed_pitch) {
		double semitones = log(v) / log(exp2(1.0/12.0));
		g_signal_handler_block(GTK_RANGE(user_data), slider2sigid);
		gtk_range_set_value(GTK_RANGE(user_data), semitones*100.0);
		g_signal_handler_unblock(GTK_RANGE(user_data), slider2sigid);
		play->effect_param[1] = rint(semitones);
		play->effect_param[0] = rint(semitones) - semitones;
	}
#endif
}


static void
pitch_toggled (GtkToggleButton* btn, gpointer user_data)
{
	play->enable_effect = gtk_toggle_button_get_active (btn);
	gtk_widget_set_sensitive(app->playercontrol->slider2, play->enable_effect);
#ifdef VARISPEED
	gtk_widget_set_sensitive(app->playercontrol->cblnk, play->enable_effect);
	if (!play->enable_effect) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->playercontrol->cblnk), false);
#endif
}


#ifdef VARISPEED
static void
cb_link_toggled (GtkToggleButton *btn, gpointer user_data)
{
	play->link_speed_pitch = gtk_toggle_button_get_active (btn);
	if (play->link_speed_pitch) {
		speed_value_changed(GTK_RANGE(app->playercontrol->slider3), app->playercontrol->slider2);
	}
}
#endif
#endif


static void
playpause_toggled (GtkToggleButton* btn, gpointer user_data)
{
	gtk_toggle_button_get_active(btn)
		? application_pause()
		: application_play(NULL);
}


static void
slider_value_changed (GtkRange *range, gpointer user_data)
{
	if (play->auditioner->seek) {
		double v =  gtk_range_get_value(range);
		play->auditioner->seek(v);
	}
}

#ifdef HAVE_JACK
void
update_slider (gpointer _)
{
	PlayCtrl* pc = app->playercontrol;

	if (pc->slider1 && gtk_widget_has_grab(GTK_WIDGET(pc->slider1))) return; // user is manipulating the slider
	if (!play->auditioner->position) return;

#if (defined ENABLE_LADSPA)
	if ( gtk_widget_get_sensitive(pc->slider2)
			&& (!play->enable_effect || !play->effect_enabled)) {
		/* Note: play->effect_enabled is set by player thread
		 * and not yet up-to-date in player_control_on_show_hide() */
		gtk_widget_set_sensitive(pc->slider2, false);
	}
#endif

	double v = play->auditioner->position();
	if (v >= 0) {
		g_signal_handler_block(pc->slider1, slider1sigid);
		gtk_range_set_value(GTK_RANGE(pc->slider1), v);
		g_signal_handler_unblock(pc->slider1, slider1sigid);
	}
	else if (v == -2.0)
		return; // seek in progress
	else {
		/* playback is done */
		if (pc->slider1)
			gtk_widget_set_sensitive(pc->slider1, false);
#if (defined ENABLE_LADSPA)
		gtk_widget_set_sensitive(pc->cbfx, true);
#ifndef VARISPEED
		gtk_widget_set_sensitive(pc->slider3, true);
#endif
		gtk_widget_set_sensitive(pc->slider2, true);
#endif
	}
}
#endif


void
player_control_on_show_hide (bool enable)
{
	PlayCtrl* pc = app->playercontrol;

	#define WIDGETS_CREATED (pc->slider1) // not created until audio is ready.

	static guint play_start_handler = 0;
	static guint play_stop_handler = 0;
#ifdef HAVE_JACK
	static guint play_pos_handler = 0;
#endif

#ifdef USE_GDL
	gtk_widget_show_all(app->playercontrol->widget);
#endif

	if (!enable) {
		if (play_start_handler) {
			g_signal_handler_disconnect((gpointer)app, play_start_handler);
			play_start_handler = 0;
			g_signal_handler_disconnect((gpointer)app, play_stop_handler);
			play_stop_handler = 0;
#ifdef HAVE_JACK
			g_signal_handler_disconnect((gpointer)app, play_pos_handler);
			play_pos_handler = 0;
#endif
		}
		return;
	}

	bool visible = gtk_widget_get_visible(pc->widget);

	if (WIDGETS_CREATED) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pc->pb.pause), play->status == PLAY_PAUSED);
	}

	if (samplecat.model->selection && play->sample && (play->sample->id == samplecat.model->selection->id)) {
		if (!visible) {
			gtk_widget_set_no_show_all(pc->widget, false);
			gtk_widget_show_all(pc->widget);
		}
#if (defined ENABLE_LADSPA)
		gtk_widget_set_sensitive(pc->cbfx, false);
#ifndef VARISPEED
		gtk_widget_set_sensitive(pc->slider3, false);
#endif
		if (!play->enable_effect) {
			gtk_widget_set_sensitive(pc->slider2, false);
		}
#endif
	} else {
		/* hide player */
		if (WIDGETS_CREATED) gtk_widget_set_sensitive(pc->slider1, false);
	}

	void pc_on_play (GObject* _player, PlayCtrl* pc)
	{
		if (pc->slider1) {
			Sample* playing = play->sample;
			if (playing->sample_rate) {
				GtkAdjustment* a = gtk_range_get_adjustment ((GtkRange*)pc->slider1);
				gtk_adjustment_set_upper(a, (playing->frames * 1000) / playing->sample_rate);
			}
			gtk_widget_set_sensitive(pc->slider1, true);
		}
		if (pc->pb.box) {
			gtk_widget_set_sensitive(pc->pb.box, true);
		}
		if (pc->pb.next) {
			gtk_widget_set_sensitive(pc->pb.next, play->queue != NULL);
		}
	}

	void pc_on_stop (GObject* _player, PlayCtrl* pc)
	{
		if (pc->slider1)
			gtk_widget_set_sensitive(pc->slider1, false);
		gtk_widget_set_sensitive(pc->pb.box, false);

		if (pc->pb.next) {
			gtk_widget_set_sensitive(pc->pb.next, false);
		}
	}

	if (!play_start_handler) play_start_handler = g_signal_connect(play, "play", (GCallback)pc_on_play, pc);
	if (!play_stop_handler) play_stop_handler = g_signal_connect(play, "stop", (GCallback)pc_on_stop, pc);
#ifdef HAVE_JACK
	if (!play_pos_handler) play_pos_handler = g_signal_connect(play, "position", (GCallback)update_slider, pc);
#endif
}

