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
#include <gtk/gtk.h>
#include <debug/debug.h>
#include "player/player.h"
#include "application.h"
#include "model.h"

#if (defined HAVE_JACK)
  #include "player/jack_player.h"
#endif

typedef struct
{
	GtkWidget* widget;

	struct {
	   GtkWidget* position;
	   GtkWidget* pitch;
	}          sliders;
	GtkWidget* slider3; // player speed
	GtkWidget* cbfx;    // player enable FX
	GtkWidget* cblnk;   // link speed/pitch
	struct {
	   GtkWidget* next;
	}          pb;
} PlayCtrl;

typedef struct {
	GtkWidget* stop;
	GtkWidget* play;
	GtkWidget* pause;
} Btns;
static Btns btns;
static GtkWidget** btn = (GtkWidget**)& btns;

static PlayCtrl* playercontrol;

void player_control_on_show_hide (bool);

static void  pc_add_widgets       ();

static guint slider1sigid;
static void  slider_value_changed (GtkRange*, gpointer user_data);

#if (defined ENABLE_LADSPA)
static guint slider2sigid, slider3sigid;

static void  speed_value_changed  (GtkRange*, gpointer user_data);
static void  pitch_value_changed  (GtkRange*, gpointer user_data);
static void  cb_pitch_toggled     (GtkToggleButton*, gpointer user_data);
static void  cb_link_toggled      (GtkToggleButton*, gpointer user_data);
#endif


GtkWidget*
player_control_new ()
{
	g_return_val_if_fail(!playercontrol, NULL);

	PlayCtrl* pc = playercontrol = g_new0(PlayCtrl, 1);

	pc->widget = gtk_scrolled_window_new();
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pc->widget), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	gtk_widget_set_size_request(pc->widget, -1, 40);

	void pc_on_audio_ready (gpointer user_data, gpointer _)
	{
		pc_add_widgets();

		if (playercontrol->sliders.position)
			gtk_widget_set_sensitive(playercontrol->sliders.position, false);

		player_control_on_show_hide(true);
	}

	am_promise_add_callback(play->ready, pc_on_audio_ready, NULL);

	return pc->widget;
}


static void
pc_add_widgets ()
{
	PlayCtrl* pc = playercontrol;

	GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(pc->widget), vbox);

	#define BORDERMARGIN (2)
	#define MarginLeft (5)

	if (play->auditioner->position && play->auditioner->seek) { // JACK seek-slider
		GtkWidget* slider = pc->sliders.position = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 1.0 / (float)OVERVIEW_WIDTH);
		gtk_widget_set_size_request(slider, OVERVIEW_WIDTH + BORDERMARGIN + BORDERMARGIN, -1);
		gtk_widget_set_tooltip_text(slider, "playhead position / seek");
		gtk_scale_set_draw_value(GTK_SCALE(slider), false);
		gtk_box_append(GTK_BOX(vbox), slider);

		slider1sigid = g_signal_connect((gpointer)slider, "value-changed", G_CALLBACK(slider_value_changed), NULL);
	}

	if (!(play->auditioner->position && play->auditioner->seek)) {
		GtkWidget* label = gtk_label_new("<span size=\"200%\">00:00</span> 000");
		gtk_label_set_use_markup((GtkLabel*)label, true);
		gtk_widget_set_name(label, "spp");
		gtk_label_set_xalign((GtkLabel*)label, 0.);
		gtk_box_append(GTK_BOX(vbox), label);

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

				char str[40];
				snprintf(str, 39, "<span size=\"200%%\">%02i:%02i</span> %03i", minutes % 1000, secs % 100, ms % 1000);
				gtk_label_set_markup((GtkLabel*)label, str);

				return G_SOURCE_CONTINUE;
			}

			gboolean keep_alive (void* label)
			{
				inactivity = 0;
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

	//
	// Play-control toggle buttons
	//
	GtkWidget* btnbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	{
		gtk_box_append(GTK_BOX(vbox), btnbox);

		struct {
			char* icon;
			char* action;
		} b[] = {
			{"media-playback-stop-symbolic", "app.player-stop"},
			{"media-playback-start-symbolic", "app.player-play"},
			{"media-playback-pause-symbolic", "app.player-pause"},
		};
		int n_buttons = play->auditioner->pause ? G_N_ELEMENTS(b) : 2;
		for (int i=0;i<n_buttons;i++) {
			btn[i] = gtk_toggle_button_new ();
			gtk_button_set_icon_name ((GtkButton*)btn[i], b[i].icon);
			gtk_box_append(GTK_BOX(btnbox), btn[i]);
			gtk_widget_set_hexpand(btn[i], true);
			gtk_actionable_set_action_name (GTK_ACTIONABLE(btn[i]), b[i].action);
		}
	}

	if (play->auditioner->position && play->auditioner->seek) {
#if (defined ENABLE_LADSPA)
		/* note: we could do speed-changes w/o LADSPA, but it'd be EVEN MORE ifdefs */

		/* pitch LADSPA/rubberband */
#ifdef GTK4_TODO
		GtkWidget* align10 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
		gtk_alignment_set_padding(GTK_ALIGNMENT(align10), 0, 0, MarginLeft-BORDERMARGIN, 0);
		gtk_box_pack_start(GTK_BOX(vbox), align10, EXPAND_FALSE, FILL_FALSE, 0);
#endif

		GtkWidget* slider2 = pc->sliders.pitch = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -1200, 1200, 1.0);
		gtk_widget_set_size_request(slider2, OVERVIEW_WIDTH + BORDERMARGIN + BORDERMARGIN, -1);
		gtk_range_set_value(GTK_RANGE(slider2), play->effect_param[0]);
		gtk_widget_set_tooltip_text(slider2, "Pitch in Cents (1/100 semitone)");
		//gtk_scale_set_draw_value(GTK_SCALE(slider2), false);
#ifdef GTK4_TODO
		gtk_container_add(GTK_CONTAINER(align10), slider2);	
#else
		gtk_box_append(GTK_BOX(vbox), slider2);
#endif

		/* effect ctrl buttons */
		GtkWidget* cb0 = pc->cbfx = gtk_check_button_new_with_label("enable pitch-shift");
		gtk_widget_set_tooltip_text(cb0, "Pitch-shifting (rubberband) will decrease playback-quality and increase CPU load.");
		gtk_check_button_set_active (GTK_CHECK_BUTTON(cb0), play->enable_effect);
		gtk_box_append(GTK_BOX(vbox), cb0);
		g_signal_connect((gpointer)cb0, "toggled", G_CALLBACK(cb_pitch_toggled), NULL);
#ifdef VARISPEED
		GtkWidget* cb1 = pc->cblnk = gtk_check_button_new_with_label("preserve pitch.");
		gtk_widget_set_tooltip_text(cb1, "link the two sliders to preserve the original pitch.");
		gtk_check_button_set_active (GTK_CHECK_BUTTON(cb1), true);
		gtk_widget_set_sensitive(cb1, play->enable_effect);
		gtk_box_append(GTK_BOX(vbox), cb1);
		g_signal_connect((gpointer)cb1, "toggled", G_CALLBACK(cb_link_toggled), NULL);
#endif

		/* speed - samplerate */
#ifdef GTK4_TODO
		GtkWidget* align11 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
		gtk_alignment_set_padding(GTK_ALIGNMENT(align11), 0, 0, MarginLeft-BORDERMARGIN, 0);
		gtk_box_pack_start(GTK_BOX(vbox), align11, EXPAND_FALSE, FILL_FALSE, 0);
#endif

		GtkWidget* slider3 = pc->slider3 = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.5, 2.0, .01);
		gtk_widget_set_size_request(slider3, OVERVIEW_WIDTH+BORDERMARGIN+BORDERMARGIN, -1);
		gtk_range_set_inverted(GTK_RANGE(slider3),true);
		gtk_widget_set_tooltip_text(slider3, "Playback speed factor.");
		//gtk_scale_set_draw_value(GTK_SCALE(slider3), false);
		gtk_range_set_value(GTK_RANGE(slider3), play->playback_speed);
#ifdef GTK4_TODO
		gtk_container_add(GTK_CONTAINER(align11), slider3);
#else
		gtk_box_append(GTK_BOX(vbox), slider3);
#endif

		slider2sigid = g_signal_connect((gpointer)slider2, "value-changed", G_CALLBACK(pitch_value_changed), slider3);
		slider3sigid = g_signal_connect((gpointer)slider3, "value-changed", G_CALLBACK(speed_value_changed), slider2);
#endif /* LADSPA-rubberband/VARISPEED */
	}

	if (play->auditioner->set_level) {
		GtkWidget* slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0., 100., 1.);
		gtk_widget_set_tooltip_text(slider, "Volume");
		gtk_range_set_value(GTK_RANGE(slider), 100.);
		gtk_box_append(GTK_BOX(vbox), slider);

		void
		level_slider_changed (GtkRange* range, gpointer user_data)
		{
			player_set_level(gtk_range_get_value(range));
		}
		g_signal_connect((gpointer)slider, "value-changed", G_CALLBACK(level_slider_changed), NULL);
	}

	pc->pb.next = ({
		GtkWidget* widget = gtk_button_new_from_icon_name("go-next-symbolic");
		gtk_widget_set_size_request(widget, 100, -1);
		gtk_widget_set_hexpand(widget, true);
		gtk_box_append(GTK_BOX(btnbox), widget);

		gtk_actionable_set_action_name (GTK_ACTIONABLE(widget), "app.player-next");
		widget;
	});
}


/* JACK auditioner - UI */

#if (defined ENABLE_LADSPA)
static void
pitch_value_changed (GtkRange *range, gpointer user_data)
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
speed_value_changed (GtkRange *range, gpointer user_data)
{
	float v =  gtk_range_get_value(range);
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
cb_pitch_toggled (GtkToggleButton *btn, gpointer user_data)
{
	play->enable_effect = gtk_toggle_button_get_active (btn);
	gtk_widget_set_sensitive(playercontrol->sliders.pitch, play->enable_effect);
	gtk_widget_set_sensitive(playercontrol->cblnk, play->enable_effect);
	if (!play->enable_effect) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(playercontrol->cblnk), false);
}


static void
cb_link_toggled (GtkToggleButton *btn, gpointer user_data)
{
	play->link_speed_pitch = gtk_toggle_button_get_active (btn);
	if (play->link_speed_pitch) {
		speed_value_changed(GTK_RANGE(playercontrol->slider3), playercontrol->sliders.pitch);
	}
}
#endif


static void
slider_value_changed (GtkRange *range, gpointer user_data)
{
	if (play->auditioner->seek) {
		double v =  gtk_range_get_value(range);
		play->auditioner->seek(v);
	}
}

#ifdef HAVE_JACK
static void
update_slider (gpointer _)
{
	PlayCtrl* pc = playercontrol;

#ifdef GTK4_TODO
	if (pc->sliders.position && gtk_widget_has_grab(GTK_WIDGET(pc->sliders.position))) return; // user is manipulating the slider
#endif
	if (!play->auditioner->position) return;

#if (defined ENABLE_LADSPA)
	if ( gtk_widget_get_sensitive(pc->sliders.pitch)
			&& (!play->enable_effect || !play->effect_enabled)) {
		/* Note: play->effect_enabled is set by player thread
		 * and not yet up-to-date in player_control_on_show_hide() */
		gtk_widget_set_sensitive(pc->sliders.pitch, false);
	}
#endif

	double v = play->auditioner->position();
	if (v >= 0) {
		g_signal_handler_block(pc->sliders.position, slider1sigid);
		gtk_range_set_value(GTK_RANGE(pc->sliders.position), v);
		g_signal_handler_unblock(pc->sliders.position, slider1sigid);
	}
	else if (v == -2.0)
		return; // seek in progress
	else { 
		/* playback is done */
		if (pc->sliders.position)
			gtk_widget_set_sensitive(pc->sliders.position, false);
#if (defined ENABLE_LADSPA)
		gtk_widget_set_sensitive(pc->cbfx, true);
#ifndef VARISPEED
		gtk_widget_set_sensitive(pc->slider3, true);
#endif
		gtk_widget_set_sensitive(pc->sliders.pitch, true);
#endif
	}
}
#endif


void
player_control_on_show_hide (bool enable)
{
	PlayCtrl* pc = playercontrol;

	#define WIDGETS_CREATED (pc->sliders.position) // not created until audio is ready.

	static guint play_start_handler;
#ifdef HAVE_JACK
	static guint play_stop_handler;
	static guint play_pos_handler;
	static guint pause_handler;
#endif

	gtk_widget_set_visible(playercontrol->widget, true);

	if (!enable) {
#ifdef HAVE_JACK
		if (play_start_handler) {
			g_signal_handler_disconnect((gpointer)play, play_start_handler);
			play_start_handler = 0;
			g_signal_handler_disconnect((gpointer)play, play_stop_handler);
			play_stop_handler = 0;
			g_signal_handler_disconnect((gpointer)play, play_pos_handler);
			play_pos_handler = 0;
			g_signal_handler_disconnect((gpointer)play, pause_handler);
			pause_handler = 0;
		}
#endif
		return;
	}

	bool visible = gtk_widget_get_visible(pc->widget);

	if (samplecat.model->selection && play->sample && (play->sample->id == samplecat.model->selection->id)) {
		if (!visible) {
			gtk_widget_set_visible(pc->widget, true);
		}
#if (defined ENABLE_LADSPA)
		gtk_widget_set_sensitive(playercontrol->cbfx, false);
#ifndef VARISPEED
		gtk_widget_set_sensitive(playercontrol->slider3, false);
#endif
		if (!play->enable_effect) {
			gtk_widget_set_sensitive(playercontrol->sliders.pitch, false);
		}
#endif
	} else {
		/* hide player */
		if (WIDGETS_CREATED) gtk_widget_set_sensitive(playercontrol->sliders.position, false);
	}

#ifdef HAVE_JACK
	void pc_on_play (GObject* _player, gpointer _pc)
	{
		PlayCtrl* pc = playercontrol;

		if (pc->sliders.position) {
			Sample* playing = play->sample;
			if (playing->sample_rate) {
				GtkAdjustment* a = gtk_range_get_adjustment ((GtkRange*)pc->sliders.position);
				gtk_adjustment_set_upper(a, (playing->frames * 1000) / playing->sample_rate);
			}
			gtk_widget_set_sensitive(pc->sliders.position, true);
		}
	}

	void pc_on_stop (GObject* _player, gpointer _pc)
	{
		if (playercontrol->sliders.position)
			gtk_widget_set_sensitive(playercontrol->sliders.position, false);
	}

	if (player_is_stopped()) pc_on_stop(NULL, NULL);
#endif

#ifdef HAVE_JACK
	if (!play_start_handler) play_start_handler = g_signal_connect(play, "play", (GCallback)pc_on_play, pc);
	if (!play_stop_handler) play_stop_handler = g_signal_connect(play, "stop", (GCallback)pc_on_stop, pc);
	if (!play_pos_handler) play_pos_handler = g_signal_connect(play, "position", (GCallback)update_slider, pc);

	void on_player_state_change (GObject* o, GParamSpec*, gpointer _)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btns.stop), play->state == PLAYER_STOPPED);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btns.play), play->state > PLAYER_PAUSED);
		if (btns.pause) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btns.pause), play->state == PLAYER_PAUSED);
		}
	}
	if (!pause_handler) pause_handler = g_signal_connect(play, "notify::state", G_CALLBACK(on_player_state_change), NULL);

	on_player_state_change(NULL, NULL, NULL);
#endif
}
