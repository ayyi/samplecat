/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2014 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "file_manager/rox_support.h" // to_utf8()
#include "mimetype.h"
#include "support.h"
#include "main.h"
#include "mimetype.h"
#include "sample.h"
#include "listview.h"
#include "window.h"
#ifdef USE_TRACKER
  #include "src/db/db.h"
  #include "src/db/tracker.h"
#endif
#if (defined HAVE_JACK)
  #include "jack_player.h"
#endif

#define USE_SIMPLE_TABLE
#ifdef USE_SIMPLE_TABLE
#include "table.h"
#define gtk_table_new simple_table_new
#define gtk_table_attach simple_table_attach
#define gtk_table_get_size simple_table_get_size
#define gtk_table_resize simple_table_resize
#undef GTK_TABLE
#define GTK_TABLE GTK_SIMPLE_TABLE
#endif

#include "inspector.h"

#define N_EBUR_ROWS 8
#define MARGIN_LEFT 5

extern SamplecatModel*  model;
extern int              debug;

typedef struct
{
    GtkWidget*     first_child;
    int            start;
    int            n;
} RowRange;

struct _inspector_priv
{
	unsigned       row_id;

	GtkWidget*     name;
	GtkWidget*     image;
	GtkWidget*     table;
	GtkWidget*     filename;
	GtkWidget*     tags;
	GtkWidget*     tags_ev;    // event box for mouse clicks
	GtkWidget*     length;
	GtkWidget*     frames;
	GtkWidget*     samplerate;
	GtkWidget*     channels;
	GtkWidget*     mimetype;
	GtkWidget*     level;
	GtkWidget*     bitrate;
	GtkWidget*     bitdepth;
	GtkWidget*     ebur0;     // first ebur item
	RowRange       meta;
	GtkWidget*     vbox;
	GtkWidget*     text;
	GtkTextBuffer* notes;
	GtkWidget*     edit;
	GtkTreeRowReference* row_ref; // TODO remove
};

static void inspector_clear              ();
static void inspector_update             (SamplecatModel*, Sample*, gpointer);
static void inspector_set_labels         (Sample*);
static void hide_fields                  ();
static void show_fields                  ();
static bool inspector_on_tags_clicked    (GtkWidget*, GdkEventButton*, gpointer);
static bool on_notes_focus_out           (GtkWidget*, gpointer);
static void tag_edit_start               (int);
static void tag_edit_stop                (GtkWidget*, GdkEventCrossing*, gpointer);
static void inspector_remove_rows        (int, int);


GtkWidget*
inspector_new()
{
	// detailed information on a single sample. LHS of main window.

	g_return_val_if_fail(!app->inspector, NULL);

	Inspector* inspector = app->inspector = g_new0(Inspector, 1);
	InspectorPriv* i = inspector->priv = g_new0(InspectorPriv, 1);
	inspector->preferred_height = 200;
	inspector->show_waveform = true;

	GtkWidget* scroll = inspector->widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	i->vbox = gtk_vbox_new(false, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), i->vbox);

	GtkWidget* label_new(const char* text)
	{
		GtkWidget* label = gtk_label_new(text);
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
		gtk_widget_set_sensitive(label, false);
		return label;
	}

	//left align the label:
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(i->vbox), align1, EXPAND_FALSE, FILL_FALSE, 0);

	// sample name:
	GtkWidget* label1 = i->name = label_new("");
	gtk_label_set_ellipsize((GtkLabel*)label1, PANGO_ELLIPSIZE_MIDDLE);
	gtk_misc_set_padding(GTK_MISC(i->name), MARGIN_LEFT, 5);
	gtk_container_add(GTK_CONTAINER(align1), i->name);	

	PangoFontDescription* pangofont = pango_font_description_from_string("Sans-Serif 18");
	gtk_widget_modify_font(label1, pangofont);
	pango_font_description_free(pangofont);

	//-----------

	i->image = gtk_image_new_from_pixbuf(NULL);
	gtk_misc_set_alignment(GTK_MISC(i->image), 0.0, 0.5);
	gtk_misc_set_padding(GTK_MISC(i->image), MARGIN_LEFT, 2);
	gtk_box_pack_start(GTK_BOX(i->vbox), i->image, EXPAND_FALSE, FILL_FALSE, 0);

	struct _rows {
		char*       description;
		GtkWidget** widget;
	} rows[] = {
		{"filename",   &i->filename},
		{"tags",       &i->tags},
		{"length",     &i->length},
		{"frames",     &i->frames},
		{"samplerate", &i->samplerate},
		{"channels",   &i->channels},
		{"mimetype",   &i->mimetype},
		{"level",      &i->level},
		{"bitrate",    &i->bitrate},
		{"bitdepth",   &i->bitdepth},
	};
	int n_rows = G_N_ELEMENTS(rows);

	GtkWidget* table = i->table = gtk_table_new(n_rows/* + N_EBUR_ROWS*/ + 1, 3, HOMOGENOUS);

	int s = 0;
	//first two rows are special case - colspan=2

	GtkWidget* label = i->filename = label_new("Filename");
	gtk_misc_set_padding(GTK_MISC(label), MARGIN_LEFT, 0);

	gtk_label_set_ellipsize((GtkLabel*)i->filename, PANGO_ELLIPSIZE_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), label, 0, 3, s, s+1, GTK_FILL, GTK_FILL, 0, 0);
	s++;

	{
		//the tags label has an event box to catch mouse clicks.
		i->tags_ev = gtk_event_box_new ();
		g_signal_connect((gpointer)i->tags_ev, "button-release-event", G_CALLBACK(inspector_on_tags_clicked), NULL);

		GtkWidget* label = i->tags = label_new("Tags");
		gtk_misc_set_padding(GTK_MISC(label), MARGIN_LEFT, 0);
		gtk_container_add(GTK_CONTAINER(i->tags_ev), label);
		gtk_table_attach(GTK_TABLE(table), i->tags_ev, 0, 2, s, s+1, GTK_FILL, GTK_FILL, 0, 0);
		s++;
	}

	for(;s<n_rows;s++){
		GtkWidget* label = label_new(rows[s].description);
		gtk_misc_set_padding(GTK_MISC(label), MARGIN_LEFT, 0);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, s+1, s+2, GTK_FILL, GTK_FILL, 0, 0);

		label = *rows[s].widget = label_new("");
		gtk_table_attach(GTK_TABLE(table), label, 1, 2, s+1, s+2, GTK_FILL, GTK_FILL, 0, 0);
	}

	gtk_box_pack_start(GTK_BOX(i->vbox), table, EXPAND_FALSE, FILL_FALSE, 0);

	{
		// notes box:

		i->text = gtk_text_view_new();
		i->notes = gtk_text_view_get_buffer(GTK_TEXT_VIEW(i->text));
		gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(i->text), false);
		gtk_widget_set_sensitive(i->text, false);

#ifdef USE_TRACKER
		if(BACKEND_IS_TRACKER){
			gtk_widget_set_no_show_all(i->text, true);
		}
#endif
		g_signal_connect(G_OBJECT(i->text), "focus-out-event", G_CALLBACK(on_notes_focus_out), NULL);
		gtk_box_pack_start(GTK_BOX(i->vbox), i->text, false, true, 2);

		GValue gval = {0,};
		g_value_init(&gval, G_TYPE_CHAR);
		g_value_set_char(&gval, MARGIN_LEFT);
		g_object_set_property(G_OBJECT(i->text), "border-width", &gval);

		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(i->text), GTK_WRAP_WORD_CHAR);
		gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(i->text), 5);
		gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(i->text), 5);
	}

	//invisible edit widget:
	GtkWidget* edit = i->edit = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(edit), 64);
	gtk_entry_set_text(GTK_ENTRY(edit), "");
	g_object_ref(edit); // stops gtk deleting the unparented widget.

	g_signal_connect((gpointer)app->model, "selection-changed", G_CALLBACK(inspector_update), NULL);

	void sample_changed(SamplecatModel* m, Sample* sample, int what, void* data, gpointer user_data)
	{
		if (sample->id == app->inspector->priv->row_id)
			inspector_update(m, sample, NULL);
	}
	g_signal_connect((gpointer)app->model, "sample-changed", G_CALLBACK(sample_changed), NULL);

	gtk_widget_set_size_request(inspector->widget, 20, 20);

#ifdef USE_GDL
	void _inspector_on_layout_changed(GObject* object, gpointer user_data)
	{
	}

	Idle* idle = idle_new(_inspector_on_layout_changed, NULL);
	g_signal_connect(app, "layout-changed", (GCallback)idle->run, idle);
#endif

	void inspector_on_allocate(GtkWidget* win, GtkAllocation* allocation, gpointer user_data)
	{
		InspectorPriv* i = app->inspector->priv;
		gtk_widget_set_size_request(i->filename, allocation->width -2, -1);
		gtk_widget_set_size_request(i->name, allocation->width -2, -1);
	}
	g_signal_connect(G_OBJECT(scroll), "size-allocate", G_CALLBACK(inspector_on_allocate), NULL);

	return scroll;
}


void
inspector_free(Inspector* inspector)
{
	g_free(inspector->priv);
	g_free(inspector);
}


static void
inspector_add_ebu_cells()
{
	Inspector* inspector = app->inspector;
	InspectorPriv* i = inspector->priv;

	#define SPACER_ROW 1
	guint s = GTK_TABLE(i->table)->nrows;
	gtk_table_resize(GTK_TABLE(i->table), s + SPACER_ROW + N_EBUR_ROWS, 3);

	GtkWidget* ebur_rows[N_EBUR_ROWS];
	int r; for(r=s+SPACER_ROW;r<N_EBUR_ROWS+s+SPACER_ROW;r++){
		GtkWidget* row00 = ebur_rows[r - s - SPACER_ROW] = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(row00), 0.0, 0.5);
		gtk_misc_set_padding(GTK_MISC(row00), MARGIN_LEFT, 0);
		gtk_table_attach(GTK_TABLE(i->table), row00, 0, 1, r, r+1, GTK_FILL, GTK_FILL, 0, 0);

		GtkWidget* row01 = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(row01), 1.0, 0.5);
		gtk_table_attach(GTK_TABLE(i->table), row01, 1, 2, r, r+1, GTK_FILL, GTK_FILL, 0, 0);
	}
	i->ebur0 = ebur_rows[0];
	i->meta.start = r + SPACER_ROW;
	gtk_widget_show_all(i->table);
}


static void
inspector_add_meta_cells(GPtrArray* meta_data)
{
	Inspector* inspector = app->inspector;
	InspectorPriv* i = inspector->priv;

	if(!meta_data) return;

	// currently we are not able to add ebu cells after metadata cells so they must be added first.
	if(!i->meta.start) inspector_add_ebu_cells();

	int rows_needed = meta_data->len / 2;
	int n_to_add = rows_needed - i->meta.n;

	if(n_to_add > 0){
		GtkWidget* meta_rows[n_to_add];
		int r; for(r=i->meta.start+i->meta.n;r<i->meta.start+meta_data->len/2;r++){
			GtkWidget* row00 = meta_rows[r - i->meta.start - i->meta.n] = gtk_label_new("");
			gtk_misc_set_alignment(GTK_MISC(row00), 0.0, 0.5);
			gtk_misc_set_padding(GTK_MISC(row00), MARGIN_LEFT, 0);
			gtk_table_attach(GTK_TABLE(i->table), row00, 0, 1, r, r+1, GTK_FILL, GTK_FILL, 0, 0);

			GtkWidget* row01 = gtk_label_new("");
			gtk_misc_set_alignment(GTK_MISC(row01), 0.0, 0.5);
			gtk_label_set_ellipsize((GtkLabel*)row01, PANGO_ELLIPSIZE_END);
			gtk_table_attach(GTK_TABLE(i->table), row01, 1, 2, r, r+1, GTK_FILL, GTK_FILL, 0, 0);
		}
		if(!i->meta.first_child) i->meta.first_child = meta_rows[0];
	}else if(n_to_add < 0){
		inspector_remove_rows(i->meta.start + meta_data->len / 2, -n_to_add);
	}

	i->meta.n = meta_data->len / 2;
	gtk_widget_show_all(i->table);
}


static void
inspector_remove_rows(int n_rows, int n_to_remove)
{
	// remove rows from the end of the table

	Inspector* inspector = app->inspector;
	InspectorPriv* i = inspector->priv;

	GList* rows = gtk_container_get_children(GTK_CONTAINER(i->table));
	GList* l = rows; int r = 0;
	for(;l && r < n_to_remove;l=l->next, r++){
		GtkWidget* child = l->data;
		gtk_widget_destroy(child);
		l = l->next, child = l->data;
		gtk_widget_destroy(child);
	}

	gtk_table_resize(GTK_TABLE(i->table), n_rows, 3);

	g_list_free(rows);
}


static void
inspector_remove_ebu_cells()
{
	PF0;
	Inspector* inspector = app->inspector;
	InspectorPriv* i = inspector->priv;

	guint s = 0;
	GList* rows = g_list_reverse(gtk_container_get_children(GTK_CONTAINER(i->table)));
	GList* l = rows; int r = 0;
	for(;l;l=l->next, r++){
		if((GtkWidget*)l->data == i->ebur0) break;
		s++;
	}
	s = s / 2;
	for(r=0;l && r<N_EBUR_ROWS;l=l->next, r++){
		GtkWidget* child = l->data;
		gtk_widget_destroy(child);
		l = l->next, child = l->data;
		gtk_widget_destroy(child);
	}

	i->ebur0 = NULL;

	gtk_table_resize(GTK_TABLE(i->table), s, 3);

	g_list_free(rows);
}


static void
inspector_set_labels(Sample* sample)
{
	Inspector* inspector = app->inspector;
	if(!inspector) return;
	g_return_if_fail(sample);
	InspectorPriv* i = inspector->priv;

	{
		gtk_widget_set_sensitive(i->name, true);
		gtk_widget_set_sensitive(i->tags, true);
		gtk_widget_set_sensitive(i->filename, true);
		gtk_widget_set_sensitive(i->text, true);

		GList* children = gtk_container_get_children(GTK_CONTAINER(i->table));
		GList* l = children;
		for(;l;l=l->next){
			GtkWidget* child = l->data;
			gtk_widget_set_sensitive(child, true);
		}
		g_list_free(children);
	}

	char* ch_str = channels_format(sample->channels);
	char* level  = gain2dbstring(sample->peaklevel);

	char fs_str[32]; samplerate_format(fs_str, sample->sample_rate); strcpy(fs_str + strlen(fs_str), " kHz");
	char length[64]; smpte_format(length, sample->length);
	char frames[32]; sprintf(frames, "%"PRIi64"", sample->frames);
	char bitrate[32]; bitrate_format(bitrate, sample->bit_rate);
	char bitdepth[32]; bitdepth_format(bitdepth, sample->bit_depth);

	char* keywords = (sample->keywords && strlen(sample->keywords)) ? sample->keywords : "<no tags>";

	gtk_label_set_text(GTK_LABEL(i->name),       sample->sample_name);
	gtk_label_set_text(GTK_LABEL(i->filename),   to_utf8(sample->full_path));
	gtk_label_set_text(GTK_LABEL(i->tags),       keywords);
	gtk_label_set_text(GTK_LABEL(i->length),     length);
	gtk_label_set_text(GTK_LABEL(i->frames),     frames);
	gtk_label_set_text(GTK_LABEL(i->samplerate), fs_str);
	gtk_label_set_text(GTK_LABEL(i->channels),   ch_str);
	gtk_label_set_text(GTK_LABEL(i->mimetype),   sample->mimetype);
	gtk_label_set_text(GTK_LABEL(i->level),      level);
	gtk_label_set_text(GTK_LABEL(i->bitrate),    bitrate);
	gtk_label_set_text(GTK_LABEL(i->bitdepth),   bitdepth);
	gtk_text_buffer_set_text(i->notes,           sample->notes ? sample->notes : "", -1);

	char* ebu = sample->ebur ? sample->ebur : "";
	if(ebu && strlen(ebu) > 1){
		if(!i->ebur0) inspector_add_ebu_cells();
	} else{
		if(i->ebur0) inspector_remove_ebu_cells();
	}

	inspector_add_meta_cells(sample->meta_data);

	GList* rows = g_list_reverse(gtk_container_get_children(GTK_CONTAINER(i->table)));
	GList* l = rows; int r = 0;

	if(ebu && strlen(ebu) > 1){

		gchar** lines = g_strsplit(ebu, "\n", N_EBUR_ROWS + 1);

		for(;l;l=l->next, r++){
			if((GtkWidget*)l->data == i->ebur0) break;
		}
		for(r=0;l && r<N_EBUR_ROWS;l=l->next, r++){
			GtkWidget* child = l->data;
			char* line = lines[r];
			if(!line) break;
			gchar** c = g_strsplit(line, ":", 2);
			gtk_label_set_text(GTK_LABEL(child), c[0]);
			l = l->next, child = l->data;
			gtk_label_set_text(GTK_LABEL(child), c[1]);

			g_free(c); // free the array but not the array contents.
		}
		g_strfreev(lines);
	}

	if(sample->meta_data){
		char** meta_data = (char**)sample->meta_data->pdata;

		for(;l;l=l->next, r++){
			if((GtkWidget*)l->data == i->meta.first_child) break;
		}

		for(r=0;l && r<sample->meta_data->len/2;l=l->next, r++){
			GtkWidget* child = l->data;
			gtk_label_set_markup(GTK_LABEL(child), meta_data[2*r]); // markup used for backwards compatibility. previous format contained italic tags.
			l = l->next, child = l->data;
			gtk_label_set_text(GTK_LABEL(child), meta_data[2*r+1]);
		}
	}

	g_list_free(rows);

	if (inspector->show_waveform) {
		if (sample->overview) {
			gtk_image_set_from_pixbuf(GTK_IMAGE(i->image), sample->overview);
		} else {
			gtk_image_clear(GTK_IMAGE(i->image));
		}
		gtk_widget_show(i->image);
	} else {
		gtk_widget_hide(i->image);
	}

	show_fields();
	g_free(level);
	g_free(ch_str);

	//store a reference to the row id in the inspector widget:
	// needed to later updated "notes" for that sample
	// as well to check for updates
	// *** row_ref is deprecated. use database id instead.
	i->row_id = sample->id;
	if(i->row_ref) gtk_tree_row_reference_free(i->row_ref);
	if(sample->row_ref){
		i->row_ref = gtk_tree_row_reference_copy(sample->row_ref);
	} else {
		dbg(2, "setting row_ref failed");
		i->row_ref = NULL;
		/* can not edit tags or notes w/o reference */
		gtk_widget_hide(GTK_WIDGET(i->text));
		gtk_widget_hide(GTK_WIDGET(i->tags));
	}

#ifndef USE_GDL
	if (app->auditioner->status && app->auditioner->status() != -1.0 && app->view_options[SHOW_PLAYER].value){
		show_player(true); // show/hide player
	}
#endif
}


static void
inspector_update(SamplecatModel* m, Sample* sample, gpointer user_data)
{
	PF;
	Inspector* i = app->inspector;
	if(!i) return;
	InspectorPriv* i_ = i->priv;

	if(!sample){
		inspector_clear();
		gtk_label_set_text(GTK_LABEL(i->priv->name), "");
		return;
	}

	// forget previous inspector item
	if(i_->row_ref) gtk_tree_row_reference_free(i_->row_ref);
	i_->row_ref = NULL;
	i_->row_id = 0;

	#ifdef USE_TRACKER
	if(BACKEND_IS_TRACKER){
		if(!sample->length){
			//this sample hasnt been previously selected, and non-db info isnt available.
			//-get the info directly from the file, and set it into the main treeview.
			MIME_type* mime_type = type_from_path(sample->sample_name);
			char mime_string[64];
			snprintf(mime_string, 64, "%s/%s", mime_type->media_type, mime_type->subtype);
			if(mimestring_is_unsupported(mime_string)){
				inspector_clear();
				gtk_label_set_text(GTK_LABEL(i->filename), basename(sample->sample_name));
				return;
			}
			if(!sample_get_file_info(sample)){
				perr("cannot open file?\n");
				return;
			}
		}
	}
	#endif

	// - check if file is already in DB -> load sample-info
	// - check if file is an audio-file -> read basic info directly from file
	// - else just display the base-name in the inspector..
	if(sample->id || sample_get_file_info(sample)){
		inspector_set_labels(sample);
	} else {
		inspector_clear();
		gtk_label_set_text(GTK_LABEL(i->priv->name), sample->sample_name);
	}
}


static void
inspector_clear()
{
	Inspector* i = app->inspector;
	if(!i) return;

	hide_fields();
	gtk_image_clear(GTK_IMAGE(i->priv->image));
}


static void
hide_fields()
{
	InspectorPriv* i = app->inspector->priv;
	GtkWidget* fields[] = {i->filename, i->tags, i->length, i->samplerate, i->channels, i->mimetype, i->table, i->level, i->text, i->bitdepth, i->bitrate};
	int f = 0; for(;f<G_N_ELEMENTS(fields);f++){
		gtk_widget_hide(GTK_WIDGET(fields[f]));
	}
}


static void
show_fields()
{
	InspectorPriv* i = app->inspector->priv;
	GtkWidget* fields[] = {i->filename, i->tags, i->length, i->samplerate, i->channels, i->mimetype, i->table, i->level, i->text, i->bitdepth, i->bitrate};
	int f = 0; for(;f<G_N_ELEMENTS(fields);f++){
		gtk_widget_show(GTK_WIDGET(fields[f]));
	}
}


/** a single click on the Tags label puts us into edit mode.*/
static gboolean
inspector_on_tags_clicked(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
	if(event->button == 3) return false;
	tag_edit_start(0);
	return true;
}


static gboolean
on_notes_focus_out(GtkWidget* widget, gpointer userdata)
{
	InspectorPriv* i = app->inspector->priv;

	GtkTextBuffer* textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
	g_return_val_if_fail(textbuf, false);

	GtkTextIter start_iter, end_iter;
	gtk_text_buffer_get_start_iter(textbuf,  &start_iter);
	gtk_text_buffer_get_end_iter  (textbuf,  &end_iter);
	gchar* notes = gtk_text_buffer_get_text(textbuf, &start_iter, &end_iter, true);

	Sample* sample = app->model->selection;
	if(sample && (sample->id == i->row_id) && (!sample->notes || strcmp(notes, sample->notes))){
		statusbar_print(1,
			samplecat_model_update_sample (app->model, sample, COL_X_NOTES, notes)
				? "notes updated"
				: "failed to update notes");
	}
	g_free(notes);
	return false;
}


static void
tag_edit_start(int _)
{
	/*
	initiate the inplace renaming of a tag label in the inspector following a double-click.

	-rename widget has had an extra ref added at creation time, so we dont have to do it here.

	-current strategy is too swap the non-editable gtklabel with the editable gtkentry widget.
	-fn is fragile in that it makes too many assumptions about the widget layout (widget parent).

	FIXME global keyboard shortcuts are still active during editing?
	FIXME add ESC key to stop editing.
	*/

	PF;
	InspectorPriv* i = app->inspector->priv;

	static gulong handler1 = 0; // the edit box "focus_out" handler.
	//static gulong handler2 = 0; // the edit box RETURN key trap.

	GtkWidget* parent = i->tags_ev;
	GtkWidget* label  = i->tags;
	GtkWidget* edit   = i->edit;

	if(!GTK_IS_WIDGET(edit)) { perr("edit widget missing.\n"); return;}
	if(!GTK_IS_WIDGET(label)) { perr("label widget is missing.\n"); return;}

	Sample* s = sample_get_by_row_ref(i->row_ref);
	if (!s) {
		dbg(0,"sample not found");
	} else {
		//show the rename widget:
		gtk_entry_set_text(GTK_ENTRY(i->edit), s->keywords?s->keywords:"");
		sample_unref(s);
	}
	g_object_ref(label); //stops gtk deleting the widget.
	gtk_container_remove(GTK_CONTAINER(parent), label);
	gtk_container_add   (GTK_CONTAINER(parent), edit);
	gtk_widget_show(edit);

	//our focus-out could be improved by properly focussing other widgets when clicked on (widgets that dont catch events?).

	if(handler1) g_signal_handler_disconnect((gpointer)edit, handler1);

#warning handle ENTER
	handler1 = g_signal_connect((gpointer)edit, "focus-out-event", G_CALLBACK(tag_edit_stop), GINT_TO_POINTER(0));

	/*
	if(handler2) g_signal_handler_disconnect((gpointer)arrange->wrename, handler2);
	handler2 =   g_signal_connect(G_OBJECT(arrange->wrename), "key-press-event", 
									G_CALLBACK(track_entry_key_press), GINT_TO_POINTER(tnum));//traps the RET key.
	*/

	//grab_focus allows us to start typing imediately. It also selects the whole string.
	gtk_widget_grab_focus(GTK_WIDGET(edit));
}


static void
tag_edit_stop(GtkWidget* widget, GdkEventCrossing* event, gpointer user_data)
{
	/*
	tidy up widgets and notify core of label change.
	called following a focus-out-event for the gtkEntry renaming widget.
	-track number should correspond to the one being edited...???

	warning: if called incorrectly, this fn can cause mysterious segfaults.
	Is it something to do with multiple focus-out events? This function causes the
	rename widget to lose focus, it can run a 2nd time before completing the first: segfault!
	Currently it works ok if focus is removed before calling.
	*/

	InspectorPriv* i = app->inspector->priv;
	GtkWidget* edit = i->edit;
	GtkWidget* parent = i->tags_ev;

	bool row_set_tags_from_id(int id, GtkTreeRowReference* row_ref, const char* tags_new)
	{
		g_return_val_if_fail(id, false);
		g_return_val_if_fail(row_ref, false);
		bool ok = false;

		dbg(1, "id=%i", id);

		Sample* sample = sample_get_by_row_ref(row_ref);
		g_return_val_if_fail(sample, false);
		if((ok = samplecat_model_update_sample (app->model, sample, COL_KEYWORDS, (void*)tags_new))){
			statusbar_print(1, "keywords updated");
		}else{
			statusbar_print(1, "failed to update keywords");
		}
		sample_unref(sample);
		return ok;
	}

	//update the data:
	row_set_tags_from_id(i->row_id, i->row_ref, gtk_entry_get_text(GTK_ENTRY(edit)));

	//swap back to the normal label:
	gtk_container_remove(GTK_CONTAINER(parent), edit);

	gtk_container_add(GTK_CONTAINER(parent), i->tags);
	g_object_unref(i->tags); //remove 'artificial' ref added in edit_start.
	dbg(0, "finished.");
}

