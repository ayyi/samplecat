/*
  This file is part of Samplecat. http://samplecat.orford.org
  copyright (C) 2007-2012 Tim Orford and others.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 3
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

//#define USE_MYSQL
#include "config.h"
#include <string.h>
#include <mx/mx.h>
#include "src/typedefs.h"
#include "db/db.h"
#include "sample.h"
#include "list_view.h"
#include "utils/ayyi_utils.h"

struct _backend backend;

struct _samplecat_model //TODO is a dupe
{
	struct {
		char       phrase[256]; // XXX TODO increase to PATH_MAX
		char*      dir;
		gchar*     category;

	} filters;
};
SamplecatModel model;

unsigned debug = 1;

static void rotate_clicked_cb (ClutterActor* button, MxWindow*);


int
main (int argc, char **argv)
{
	memset(&backend, 0, sizeof(struct _backend));

	MxApplication* app = mx_application_new (&argc, &argv, "Test PathBar", 0);

	#define MAX_DISPLAY_ROWS 20
	mysql__init(&model, &(SamplecatMysqlConfig){
		"localhost",
		"samplecat",
		"samplecat",
		"samplecat",
	});
	if(samplecat_set_backend(BACKEND_MYSQL)){
		//g_strlcpy(model.filters.phrase, "909", 256);
	}

	MxWindow* window = mx_application_create_window (app);
	ClutterActor* stage = (ClutterActor*)mx_window_get_clutter_stage (window);
	mx_window_set_icon_name (window, "window-new");
	clutter_actor_set_size (stage, 480, 320);

	ClutterActor* table = mx_table_new ();
	mx_table_set_column_spacing (MX_TABLE (table), 8);
	mx_table_set_row_spacing (MX_TABLE (table), 12);
	mx_window_set_child (window, table);

	ClutterActor* entry = mx_entry_new();

	ClutterActor* scroll_view = mx_scroll_view_new ();
	mx_scroll_view_set_scroll_policy(MX_SCROLL_VIEW(scroll_view), MX_SCROLL_POLICY_VERTICAL);

	ClutterModel* list_model = sample_list_model_new(&model);

	int n_results = 0;
	if(!backend.search_iter_new("", "", "", &n_results)){
	}
	unsigned long* lengths;
	Sample* result;
	int row_count = 0;
	dbg(1, "adding items to model...");
	while((result = backend.search_iter_next(&lengths)) && row_count < MAX_DISPLAY_ROWS){
		dbg(2, "  %s", result->sample_name);
		sample_list_model_add_item((SampleListModel*)list_model, result);
		row_count++;
	}
	dbg(0, "n_results=%i", n_results);

	SamplecatMxListView* list = (SamplecatMxListView*)list_view_new((SampleListModel*)list_model);
	clutter_container_add_actor (CLUTTER_CONTAINER (scroll_view), CLUTTER_ACTOR(list));

	mx_table_add_actor_with_properties (MX_TABLE (table),
                                      entry,
                                      0, 0,
                                      "x-expand", TRUE,
                                      //"x-align", MX_ALIGN_END,
                                      "x-fill", TRUE,
                                      "y-fill", FALSE,
                                      "y-expand", FALSE,
                                      NULL);

	mx_table_add_actor_with_properties (MX_TABLE (table),
	                                  scroll_view,
                                      1, 0,
                                      "x-expand", TRUE,
                                      //"x-align", MX_ALIGN_END,
                                      "x-fill", TRUE,
                                      NULL);

	/*
	void small_screen_cb (MxToggle* toggle, GParamSpec* pspec, MxWindow* window)
	{
		mx_window_set_small_screen (window, mx_toggle_get_active (toggle));
	}

	ClutterActor* toggle = mx_toggle_new ();
	ClutterActor* label = mx_label_new_with_text ("Toggle small-screen mode");
	g_signal_connect (toggle, "notify::active", G_CALLBACK (small_screen_cb), window);
	mx_table_add_actor_with_properties (MX_TABLE (table),
                                      //toggle,
	                                  listview,
                                      0, 0,
                                      "x-expand", TRUE,
                                      "x-align", MX_ALIGN_END,
                                      "x-fill", TRUE,
                                      NULL);

	void fullscreen_cb (MxToggle *toggle, GParamSpec *pspec, ClutterStage *stage)
	{
		clutter_stage_set_fullscreen (stage, mx_toggle_get_active (toggle));
	}

	toggle = mx_toggle_new ();
	label = mx_label_new_with_text ("Toggle full-screen mode");
	g_signal_connect (toggle, "notify::active", G_CALLBACK (fullscreen_cb), stage);
	mx_table_add_actor_with_properties (MX_TABLE (table),
                                      toggle,
                                      1, 0,
                                      "x-expand", TRUE,
                                      "x-align", MX_ALIGN_END,
                                      "x-fill", FALSE,
                                      NULL);

	void icon_cb (MxToggle* toggle, GParamSpec* pspec, MxWindow* window)
	{
		gboolean use_custom = mx_toggle_get_active (toggle);

		if (use_custom) {
			CoglHandle texture = cogl_texture_new_from_file ("redhand.png", COGL_TEXTURE_NONE, COGL_PIXEL_FORMAT_ANY, NULL);
			if (texture)
				mx_window_set_icon_from_cogl_texture (window, texture);
			cogl_handle_unref (texture);
		}
		else
			mx_window_set_icon_name (window, "window-new");
	}

	toggle = mx_toggle_new ();
	label = mx_label_new_with_text ("Toggle custom window icon");
	g_signal_connect (toggle, "notify::active", G_CALLBACK (icon_cb), window);
	mx_table_add_actor_with_properties (MX_TABLE (table),
                                      toggle,
                                      2, 0,
                                      "x-expand", TRUE,
                                      "x-align", MX_ALIGN_END,
                                      "x-fill", FALSE,
                                      NULL);

	void resizable_cb (MxToggle* toggle, GParamSpec* pspec, ClutterStage* stage)
	{
		clutter_stage_set_user_resizable (stage, mx_toggle_get_active (toggle));
	}

	toggle = mx_toggle_new ();
	mx_toggle_set_active (MX_TOGGLE (toggle), TRUE);
	label = mx_label_new_with_text ("Toggle user-resizable");
	g_signal_connect (toggle, "notify::active", G_CALLBACK (resizable_cb), stage);
	mx_table_add_actor_with_properties (MX_TABLE (table),
                                      toggle,
                                      3, 0,
                                      "x-expand", TRUE,
                                      "x-align", MX_ALIGN_END,
                                      "x-fill", FALSE,
                                      NULL);

	void toolbar_cb (MxToggle *toggle, GParamSpec *pspec, MxWindow *window)
	{
		mx_window_set_has_toolbar (window, mx_toggle_get_active (toggle));
	}

	toggle = mx_toggle_new ();
	mx_toggle_set_active (MX_TOGGLE (toggle), TRUE);
	label = mx_label_new_with_text ("Toggle toolbar");
	g_signal_connect (toggle, "notify::active", G_CALLBACK (toolbar_cb), window);
	mx_table_add_actor_with_properties (MX_TABLE (table),
                                      toggle,
                                      4, 0,
                                      "x-expand", TRUE,
                                      "x-align", MX_ALIGN_END,
                                      "x-fill", FALSE,
                                      NULL);
	*/

	ClutterActor* icon = mx_icon_new();
	mx_icon_set_icon_name (MX_ICON (icon), "object-rotate-right");
	mx_icon_set_icon_size (MX_ICON (icon), 16);
	ClutterActor* button = mx_button_new ();
	mx_bin_set_child (MX_BIN (button), icon);
	g_signal_connect (button, "clicked", G_CALLBACK (rotate_clicked_cb), window);
	clutter_container_add_actor ( CLUTTER_CONTAINER (mx_window_get_toolbar (window)), button);
	mx_bin_set_alignment (MX_BIN (mx_window_get_toolbar (window)), MX_ALIGN_END, MX_ALIGN_MIDDLE);

	clutter_actor_show_all (stage);

	mx_application_run (app);

	return 0;
}


static void
rotate_clicked_cb (ClutterActor* button, MxWindow* window)
{
	dbg(0, "");

	MxWindowRotation rotation = mx_window_get_window_rotation (window);

	switch (rotation) {
		case MX_WINDOW_ROTATION_0:
			rotation = MX_WINDOW_ROTATION_90;
			break;
		case MX_WINDOW_ROTATION_90:
			rotation = MX_WINDOW_ROTATION_180;
			break;
		case MX_WINDOW_ROTATION_180:
			rotation = MX_WINDOW_ROTATION_270;
			break;
		case MX_WINDOW_ROTATION_270:
			rotation = MX_WINDOW_ROTATION_0;
			break;
	}

	mx_window_set_window_rotation (window, rotation);
}


