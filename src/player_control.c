/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://samplecat.orford.org          |
* | copyright (C) 2007-2013 Tim Orford <tim@orford.org>                  |
* | copyright (C) 2011 Robin Gareus <robin@gareus.org>                   |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <math.h>

#include "support.h"
#include "main.h"

#if (defined HAVE_JACK)
  #include "jack_player.h"
#endif


void         menu_play_stop       (GtkWidget*, gpointer user_data); //defined in main.c
static void  cb_playpause         (GtkToggleButton*, gpointer user_data);

static guint slider1sigid;
static void  slider_value_changed (GtkRange*, gpointer  user_data);

#if (defined ENABLE_LADSPA)
static guint slider2sigid, slider3sigid;

static void  speed_value_changed  (GtkRange*, gpointer user_data);
static void  pitch_value_changed  (GtkRange*, gpointer user_data);
static void  cb_pitch_toggled     (GtkToggleButton*, gpointer user_data);
static void  cb_link_toggled      (GtkToggleButton*, gpointer user_data);
#endif


GtkWidget*
player_control_new()
{
	g_return_val_if_fail(!app->playercontrol, NULL);
	PlayCtrl* pc = app->playercontrol = g_new0(PlayCtrl, 1);

	GtkWidget* top = pc->widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(top), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	gtk_widget_set_size_request(pc->widget, -1, 40);

	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(top), vbox);

#ifndef USE_GDL
	GtkWidget* label1 = gtk_label_new("Player Control");
	gtk_box_pack_start(GTK_BOX(vbox), label1, EXPAND_FALSE, FILL_TRUE, 0);
#endif

#define BORDERMARGIN (2)
#define MarginLeft (5)

	if (app->auditioner->status && app->auditioner->seek) {  //JACK seek-slider

		GtkWidget* align9 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
		gtk_alignment_set_padding(GTK_ALIGNMENT(align9), 0, 0, MarginLeft-BORDERMARGIN, 0);
		gtk_box_pack_start(GTK_BOX(vbox), align9, EXPAND_FALSE, FILL_FALSE, 0);

		GtkWidget* slider = pc->slider1 = gtk_hscale_new_with_range(0.0,1.0,1.0/(float)OVERVIEW_WIDTH);
		gtk_widget_set_size_request(slider, OVERVIEW_WIDTH+BORDERMARGIN+BORDERMARGIN, -1);
		gtk_widget_set_tooltip_text(slider, "playhead position / seek");
		gtk_scale_set_draw_value(GTK_SCALE(slider), false);
		gtk_container_add(GTK_CONTAINER(align9), slider);

		slider1sigid = g_signal_connect((gpointer)slider, "value-changed", G_CALLBACK(slider_value_changed), NULL);
	} /* end JACK auditioner */

	/* play ctrl buttons */
	GtkWidget* hbox2 = pc->pbctrl = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* pb1 = gtk_button_new_with_label("stop");
	gtk_box_pack_start(GTK_BOX(hbox2), pb1, EXPAND_TRUE, FILL_TRUE, 0);
	g_signal_connect((gpointer)pb1, "clicked", G_CALLBACK(menu_play_stop), NULL);

	if (app->auditioner->playpause) {
		GtkWidget* pb0 = pc->pbpause = gtk_toggle_button_new_with_label("pause");
		gtk_box_pack_start(GTK_BOX(hbox2), pb0, EXPAND_TRUE, FILL_TRUE, 0);
		g_signal_connect((gpointer)pb0, "toggled", G_CALLBACK(cb_playpause), NULL);
	}

	if (app->auditioner->status && app->auditioner->seek) {

#if (defined ENABLE_LADSPA) // experimental
	/* note: we could do speed-changes w/o LADSPA, but it'd be EVEN MORE ifdefs */

	/* pitch LADSPA/rubberband */
	GtkWidget* align10 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align10), 0, 0, MarginLeft-BORDERMARGIN, 0);
	gtk_box_pack_start(GTK_BOX(vbox), align10, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget *slider2 = pc->slider2 =gtk_hscale_new_with_range(-1200,1200,1.0);
	gtk_widget_set_size_request(slider2, OVERVIEW_WIDTH+BORDERMARGIN+BORDERMARGIN, -1);
	gtk_range_set_value(GTK_RANGE(slider2), app->effect_param[0]);
	gtk_widget_set_tooltip_text(slider2, "Pitch in Cents (1/100 semitone)");
	//gtk_scale_set_draw_value(GTK_SCALE(slider2), false);
	gtk_container_add(GTK_CONTAINER(align10), slider2);	

	/* effect ctrl buttons */
	GtkWidget* hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* cb0 = pc->cbfx = gtk_check_button_new_with_label("enable pitch-shift");
	gtk_widget_set_tooltip_text(cb0, "Pitch-shifting (rubberband) will decrease playback-quality and increase CPU load.");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(cb0), app->enable_effect);
	gtk_box_pack_start(GTK_BOX(hbox), cb0, EXPAND_FALSE, FILL_FALSE, 0);
	g_signal_connect((gpointer)cb0, "toggled", G_CALLBACK(cb_pitch_toggled), NULL);
#ifdef VARISPEED
	GtkWidget* cb1 = pc->cblnk = gtk_check_button_new_with_label("preserve pitch.");
	gtk_widget_set_tooltip_text(cb1, "link the two sliders to preserve the original pitch.");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(cb1), true);
	gtk_widget_set_sensitive(cb1, app->enable_effect);
	gtk_box_pack_start(GTK_BOX(hbox), cb1, EXPAND_FALSE, FILL_FALSE, 0);
	g_signal_connect((gpointer)cb1, "toggled", G_CALLBACK(cb_link_toggled), NULL);
#endif

	/* speed - samplerate */
	GtkWidget* align11 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align11), 0, 0, MarginLeft-BORDERMARGIN, 0);
	gtk_box_pack_start(GTK_BOX(vbox), align11, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* slider3 = pc->slider3 = gtk_hscale_new_with_range(0.5,2.0,.01);
	gtk_widget_set_size_request(slider3, OVERVIEW_WIDTH+BORDERMARGIN+BORDERMARGIN, -1);
	gtk_range_set_inverted(GTK_RANGE(slider3),true);
	gtk_widget_set_tooltip_text(slider3, "Playback speed factor.");
	//gtk_scale_set_draw_value(GTK_SCALE(slider3), false);
	gtk_range_set_value(GTK_RANGE(slider3), app->playback_speed);
	gtk_container_add(GTK_CONTAINER(align11), slider3);


	slider2sigid = g_signal_connect((gpointer)slider2, "value-changed", G_CALLBACK(pitch_value_changed), slider3);
	slider3sigid = g_signal_connect((gpointer)slider3, "value-changed", G_CALLBACK(speed_value_changed), slider2);
#endif /* LADSPA-rubberband/VARISPEED */
	}

	return top;
}


/* JACK auditioner - UI */

#if (defined ENABLE_LADSPA)
static void pitch_value_changed (GtkRange *range, gpointer  user_data) {
	float v =  gtk_range_get_value(range);
	app->effect_param[0] = (((int)v+1250)%100)-50;
	app->effect_param[1] = floorf((v+50.0)/100.0);
	//dbg(0," %f -> st:%.1f cent:%.1f", v, app->effect_param[1], app->effect_param[0]);
#ifdef VARISPEED
	if (app->link_speed_pitch) {
		double spd = pow(exp2(1.0/12.0), v/100.0);
		app->playback_speed = spd;
		g_signal_handler_block(GTK_RANGE(user_data), slider3sigid);
		gtk_range_set_value(GTK_RANGE(user_data), spd);
		g_signal_handler_unblock(GTK_RANGE(user_data), slider3sigid);
	}
#endif
}

static void speed_value_changed (GtkRange *range, gpointer  user_data) {
	float v =  gtk_range_get_value(range);
	app->playback_speed = v;
#ifdef VARISPEED
	if (app->link_speed_pitch) {
		double semitones = log(v)/log(exp2(1.0/12.0));
		g_signal_handler_block(GTK_RANGE(user_data), slider2sigid);
		gtk_range_set_value(GTK_RANGE(user_data), semitones*100.0);
		g_signal_handler_unblock(GTK_RANGE(user_data), slider2sigid);
		app->effect_param[1] = rint(semitones);
		app->effect_param[0] = rint(semitones)-semitones;
	}
#endif
}

static void cb_pitch_toggled(GtkToggleButton *btn, gpointer user_data) {
	app->enable_effect = gtk_toggle_button_get_active (btn);
	gtk_widget_set_sensitive(app->playercontrol->slider2, app->enable_effect);
	gtk_widget_set_sensitive(app->playercontrol->cblnk, app->enable_effect);
	if (!app->enable_effect) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->playercontrol->cblnk), false);
}

static void cb_link_toggled(GtkToggleButton *btn, gpointer user_data) {
	app->link_speed_pitch = gtk_toggle_button_get_active (btn);
	if (app->link_speed_pitch) {
		speed_value_changed(GTK_RANGE(app->playercontrol->slider3), app->playercontrol->slider2);
	}
}
#endif

static void cb_playpause (GtkToggleButton *btn, gpointer user_data) {
	if (app->auditioner->playpause) {
		app->auditioner->playpause(gtk_toggle_button_get_active (btn)?1:0);
	}
}

static void slider_value_changed (GtkRange *range, gpointer  user_data) {
	if (app->auditioner->seek) {
		double v =  gtk_range_get_value(range);
		app->auditioner->seek(v);
	}
}

#ifdef HAVE_JACK
gboolean update_slider (gpointer data) {
	if (gtk_widget_has_grab(GTK_WIDGET(data))) return TRUE; // user is manipulating the slider
	if (!app->auditioner->status) return FALSE;

	if ( gtk_widget_get_sensitive(app->playercontrol->slider2)
			&& (!app->enable_effect || !app->effect_enabled)) {
		/* Note: app->effect_enabled is set by player thread 
		 * and not yet up-to-date in player_control_on_show_hide() */
		gtk_widget_set_sensitive(app->playercontrol->slider2, false);
	}

  double v = app->auditioner->status();
	if (v>=0 && v<=1) {
		g_signal_handler_block(data, slider1sigid);
		gtk_range_set_value(GTK_RANGE(data), v);
		g_signal_handler_unblock(data, slider1sigid);
	}
	else if (v == -2.0) return TRUE; // seek in progress
	else { 
		/* playback is done */
		gtk_widget_hide(app->playercontrol->slider1);
		gtk_widget_hide(app->playercontrol->pbctrl);
#if (defined ENABLE_LADSPA)
		gtk_widget_set_sensitive(app->playercontrol->cbfx, true);
#ifndef VARISPEED
		gtk_widget_set_sensitive(app->playercontrol->slider3, true);
#endif
		gtk_widget_set_sensitive(app->playercontrol->slider2, true);
#endif
		return FALSE;
	}
	return TRUE;
}
#endif


void
player_control_on_show_hide(gboolean enable)
{
	//TODO should be split into 2 fns, one to hide show whole panel, other to hide show the slider.

	if(!enable){
		return;
	}

	gboolean visible = gtk_widget_get_visible(app->playercontrol->widget);

	static guint id = 0;
	static GSource* source = NULL;
	int updateinterval = 50; /* ms */
	if (!app->auditioner->status) return;
	if (!app->auditioner->playpause) return;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->playercontrol->pbpause), (app->auditioner->playpause(-2)==1)?true:false);

	if (app->playing_id == app->inspector->row_id) {
		if(!visible){
			gtk_widget_set_no_show_all(app->playercontrol->widget, false);
			gtk_widget_show_all(app->playercontrol->widget);
		}else{
			/* show player */
			gtk_widget_show(app->playercontrol->slider1);
			gtk_widget_show(app->playercontrol->pbctrl);
		}
#if (defined ENABLE_LADSPA)
		gtk_widget_set_sensitive(app->playercontrol->cbfx, false);
#ifndef VARISPEED
		gtk_widget_set_sensitive(app->playercontrol->slider3, false);
#endif
		if (!app->enable_effect) {
			gtk_widget_set_sensitive(app->playercontrol->slider2, false);
		}
#endif
	} else {
		/* hide player */
		gtk_widget_hide(app->playercontrol->slider1);
		updateinterval = 250; // we still need to catch EOF.
	}

#ifdef HAVE_JACK
	/* [re] launch background thread to update play slider play position */
	if (id) g_source_destroy(source);
	source = g_timeout_source_new (updateinterval);
	g_source_set_callback (source, update_slider, app->playercontrol->slider1, NULL);
	id = g_source_attach (source, NULL);
#endif
}

