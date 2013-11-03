/*
  This file is part of Samplecat. http://samplecat.orford.org
  copyright (C) 2007-2013 Tim Orford and others.

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
#include "inspector.h"
#ifdef USE_TRACKER
  #include "src/db/db.h"
  #include "src/db/tracker.h"
#endif
#if (defined HAVE_JACK)
  #include "jack_player.h"
#endif

#define N_EBUR_ROWS 8

extern SamplecatModel*  model;
//extern SamplecatBackend backend;
extern int              debug;

struct _inspector_priv
{
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
	GtkWidget*     metadata;
};

static void inspector_clear              ();
static void inspector_update             (Application*, Sample*, gpointer);
static void hide_fields                  ();
static void show_fields                  ();
static bool inspector_on_tags_clicked    (GtkWidget*, GdkEventButton*, gpointer);
static bool on_notes_focus_out           (GtkWidget*, gpointer);
static void inspector_on_notes_insert    (GtkTextView*, gchar* arg1, gpointer);
static void tag_edit_start               (int);
static void tag_edit_stop                (GtkWidget*, GdkEventCrossing*, gpointer);


GtkWidget*
inspector_new()
{
	// detailed information on a single sample. LHS of main window.

	g_return_val_if_fail(!app->inspector, NULL);

	Inspector* inspector = app->inspector = g_new0(Inspector, 1);
	InspectorPriv* i = inspector->priv = g_new0(InspectorPriv, 1);
	inspector->min_height = 200; // more like preferred height, as the box can be smaller if there is no room for it.
	inspector->show_waveform = true;

	int margin_left = 5;

	GtkWidget* scroll = inspector->widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_NEVER);

	GtkWidget* vbox = inspector->vbox = gtk_vbox_new(FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), vbox);

	//left align the label:
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), align1, EXPAND_FALSE, FILL_FALSE, 0);

	// sample name:
	GtkWidget* label1 = inspector->priv->name = gtk_label_new("");
	gtk_misc_set_padding(GTK_MISC(label1), margin_left, 5);
	gtk_container_add(GTK_CONTAINER(align1), label1);	

	PangoFontDescription* pangofont = pango_font_description_from_string("Sans-Serif 18");
	gtk_widget_modify_font(label1, pangofont);
	pango_font_description_free(pangofont);

	//-----------

	i->image = gtk_image_new_from_pixbuf(NULL);
	gtk_misc_set_alignment(GTK_MISC(i->image), 0.0, 0.5);
	gtk_misc_set_padding(GTK_MISC(i->image), margin_left, 2);
	gtk_box_pack_start(GTK_BOX(vbox), i->image, EXPAND_FALSE, FILL_FALSE, 0);

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

	GtkWidget* table = i->table = gtk_table_new(2, n_rows + N_EBUR_ROWS, HOMOGENOUS);

	int s = 0;
	//first two rows are special case - colspan=2

	GtkWidget* label = i->filename = gtk_label_new("Filename");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), margin_left, 0);
	gtk_widget_set_size_request(label, 20, -1);
	gtk_table_attach(GTK_TABLE(table), label, 0, 2, s, s+1, GTK_FILL, GTK_FILL, 0, 0);
	s++;

	{
		//the tags label has an event box to catch mouse clicks.
		i->tags_ev = gtk_event_box_new ();
		g_signal_connect((gpointer)i->tags_ev, "button-release-event", G_CALLBACK(inspector_on_tags_clicked), NULL);

		GtkWidget* label = i->tags = gtk_label_new("Tags");
		gtk_misc_set_padding(GTK_MISC(label), margin_left, 0);
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
		gtk_container_add(GTK_CONTAINER(i->tags_ev), label);
		gtk_table_attach(GTK_TABLE(table), i->tags_ev, 0, 2, s, s+1, GTK_FILL, GTK_FILL, 0, 0);
		s++;
	}

	for(;s<n_rows;s++){
		GtkWidget* label = gtk_label_new(rows[s].description);
		gtk_misc_set_padding(GTK_MISC(label), margin_left, 0);
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, s, s+1, GTK_FILL/* | GTK_EXPAND*/, GTK_FILL, 0, 0);

		label = *rows[s].widget = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 1, 2, s, (s)+1, GTK_FILL/* | GTK_EXPAND*/, GTK_FILL, 0, 0);
	}

	GtkWidget* ebur_rows[N_EBUR_ROWS];
	int r; for(r=s;r<N_EBUR_ROWS+s;r++){
		GtkWidget* row00 = ebur_rows[r - s] = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(row00), 0.0, 0.5);
		gtk_misc_set_padding(GTK_MISC(row00), margin_left, 0);
		gtk_table_attach(GTK_TABLE(table), row00, 0, 1, r, r+1, GTK_FILL, GTK_FILL, 0, 0);
		GtkWidget* row01 = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(row01), 0.0, 0.5);
		gtk_table_attach(GTK_TABLE(table), row01, 1, 2, r, r+1, GTK_FILL, GTK_FILL, 0, 0);
	}
	gtk_box_pack_start(GTK_BOX(vbox), table, EXPAND_FALSE, FILL_FALSE, 0);
	i->ebur0 = ebur_rows[0];

	GtkWidget* label11 = i->metadata = gtk_label_new("Meta-Data");
	gtk_misc_set_padding(GTK_MISC(label11), margin_left, 2);
	gtk_misc_set_alignment(GTK_MISC(label11), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label11, EXPAND_FALSE, FILL_FALSE, 0);

	// notes box:

	GtkWidget *text1 = inspector->text = gtk_text_view_new();
	inspector->notes = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text1));
	gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(text1), FALSE);
#ifdef USE_TRACKER
	if(BACKEND_IS_TRACKER){
		gtk_widget_set_no_show_all(text1, true);
	}
#endif
	g_signal_connect(G_OBJECT(text1), "focus-out-event", G_CALLBACK(on_notes_focus_out), NULL);
	g_signal_connect(G_OBJECT(text1), "insert-at-cursor", G_CALLBACK(inspector_on_notes_insert), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), text1, FALSE, TRUE, 2);

	GValue gval = {0,};
	g_value_init(&gval, G_TYPE_CHAR);
	g_value_set_char(&gval, margin_left);
	g_object_set_property(G_OBJECT(text1), "border-width", &gval);

	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text1), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(text1), 5);
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(text1), 5);

	//invisible edit widget:
	GtkWidget* edit = inspector->edit = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(edit), 64);
	gtk_entry_set_text(GTK_ENTRY(edit), "");
	g_object_ref(edit); //stops gtk deleting the unparented widget.

	g_signal_connect((gpointer)app, "selection-changed", G_CALLBACK(inspector_update), NULL);

	gtk_widget_set_size_request(inspector->widget, 20, 20);
	return vbox;
}


void
inspector_free(Inspector* inspector)
{
	g_free(inspector->priv);
	g_free(inspector);
}


void
inspector_set_labels(Sample* sample)
{
	Inspector* i = app->inspector;
	if(!i) return;
	g_return_if_fail(sample);
	InspectorPriv* i_ = i->priv;

	char* ch_str = channels_format(sample->channels);
	char* level  = gain2dbstring(sample->peaklevel);

	char fs_str[32]; samplerate_format(fs_str, sample->sample_rate); strcpy(fs_str + strlen(fs_str), " kHz");
	char length[64]; smpte_format(length, sample->length);
	char frames[32]; sprintf(frames, "%"PRIi64"", sample->frames);
	char bitrate[32]; bitrate_format(bitrate, sample->bit_rate);
	char bitdepth[32]; bitdepth_format(bitdepth, sample->bit_depth);

	char* keywords = (sample->keywords && strlen(sample->keywords)) ? sample->keywords : "<no tags>";

	gtk_label_set_text(GTK_LABEL(i_->name),       sample->sample_name);
	gtk_label_set_text(GTK_LABEL(i_->filename),   to_utf8(sample->full_path));
	gtk_label_set_text(GTK_LABEL(i_->tags),       keywords);
	gtk_label_set_text(GTK_LABEL(i_->length),     length);
	gtk_label_set_text(GTK_LABEL(i_->frames),     frames);
	gtk_label_set_text(GTK_LABEL(i_->samplerate), fs_str);
	gtk_label_set_text(GTK_LABEL(i_->channels),   ch_str);
	gtk_label_set_text(GTK_LABEL(i_->mimetype),   sample->mimetype);
	gtk_label_set_text(GTK_LABEL(i_->level),      level);
	gtk_label_set_text(GTK_LABEL(i_->bitrate),    bitrate);
	gtk_label_set_text(GTK_LABEL(i_->bitdepth),   bitdepth);
	gtk_label_set_markup(GTK_LABEL(i_->metadata), sample->meta_data?sample->meta_data:"");
	gtk_text_buffer_set_text(i->notes,           sample->notes?sample->notes:"", -1);

	char* ebu = sample->ebur ? sample->ebur : "";
	if(ebu && strlen(ebu)){
		gchar** lines = g_strsplit(ebu, "\n", N_EBUR_ROWS);

		GList* rows = g_list_reverse(gtk_container_get_children(GTK_CONTAINER(i_->table)));
		GList* l = rows; int i = 0;
		for(;l;l=l->next, i++){
			if((GtkWidget*)l->data == i_->ebur0) break;
		}
		for(i=0;l && i<N_EBUR_ROWS;l=l->next, i++){
			GtkWidget* child = l->data;
			char* line = lines[i];
			if(!line) break;
			gchar** c = g_strsplit(line, ":", 2);
			gtk_label_set_text(GTK_LABEL(child), c[0]);
			l = l->next, child = l->data;
			gtk_label_set_text(GTK_LABEL(child), c[1]);
		}
		g_strfreev(lines);
		g_list_free(rows);
	}

	if (i->show_waveform) {
		if (sample->overview) {
			gtk_image_set_from_pixbuf(GTK_IMAGE(i_->image), sample->overview);
		} else {
			gtk_image_clear(GTK_IMAGE(i_->image));
		}
		gtk_widget_show(i_->image);
	} else {
		gtk_widget_hide(i_->image);
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
		gtk_widget_hide(GTK_WIDGET(i_->tags));
	}

	if (app->auditioner->status && app->auditioner->status() != -1.0 && app->view_options[SHOW_PLAYER].value){
		show_player(true); // show/hide player
	}
}


static void
inspector_update(Application* a, Sample* sample, gpointer user_data)
{
	PF;
	Inspector* i = app->inspector;
	if(!i) return;

	if(!sample){
		inspector_clear();
		gtk_label_set_text(GTK_LABEL(i->priv->name), "");
		return;
	}

	// forget previous inspector item
	if(i->row_ref) gtk_tree_row_reference_free(i->row_ref);
	i->row_ref = NULL;
	i->row_id = 0;

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
	if(sample_get_file_info(sample)){
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
	GtkWidget* fields[] = {i->filename, i->tags, i->length, i->samplerate, i->channels, i->mimetype, i->table, i->level, app->inspector->text, i->bitdepth, i->bitrate, i->metadata};
	int f = 0; for(;f<G_N_ELEMENTS(fields);f++){
		gtk_widget_hide(GTK_WIDGET(fields[f]));
	}
}


static void
show_fields()
{
	InspectorPriv* i = app->inspector->priv;
	GtkWidget* fields[] = {i->filename, i->tags, i->length, i->samplerate, i->channels, i->mimetype, i->table, i->level, app->inspector->text, i->bitdepth, i->bitrate, i->metadata};
	int f = 0; for(;f<G_N_ELEMENTS(fields);f++){
		gtk_widget_show(GTK_WIDGET(fields[f]));
	}
}


/** a single click on the Tags label puts us into edit mode.*/
static gboolean
inspector_on_tags_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if(event->button == 3) return FALSE;
	tag_edit_start(0);
	return TRUE;
}


static void
inspector_on_notes_insert(GtkTextView *textview, gchar *arg1, gpointer user_data)
{
	dbg(0, "...");
}


static gboolean
on_notes_focus_out(GtkWidget *widget, gpointer userdata)
{
	GtkTextBuffer* textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
	g_return_val_if_fail(textbuf, false);

	GtkTextIter start_iter, end_iter;
	gtk_text_buffer_get_start_iter(textbuf,  &start_iter);
	gtk_text_buffer_get_end_iter  (textbuf,  &end_iter);
	gchar* notes = gtk_text_buffer_get_text(textbuf, &start_iter, &end_iter, true);
	dbg(2, "start=%i end=%i", gtk_text_iter_get_offset(&start_iter), gtk_text_iter_get_offset(&end_iter));

	Sample* sample = listview__get_sample_by_rowref(app->inspector->row_ref);
	if(sample){
		if(listmodel__update_sample(sample, COLX_NOTES, notes)){
			statusbar_print(1, "notes updated");
		} else {
			dbg(0, "failed to update notes");
		}
	}
	g_free(notes);
	return FALSE;
}


gboolean
row_set_tags_from_id(int id, GtkTreeRowReference* row_ref, const char* tags_new)
{
	if(!id){ perr("bad arg: id (%i)\n", id); return false; }
	g_return_val_if_fail(row_ref, false);
	dbg(0, "id=%i", id);

	if(!listmodel__update_by_rowref(row_ref, COL_KEYWORDS, (void*)tags_new)) {
		statusbar_print(1, "database error. keywords not updated");
		return false;
	}
	return true;
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
	GtkWidget* edit   = app->inspector->edit;

	if(!GTK_IS_WIDGET(app->inspector->edit)){ perr("edit widget missing.\n"); return;}
	if(!GTK_IS_WIDGET(label))              { perr("label widget is missing.\n"); return;}

	Sample* s = sample_get_by_row_ref(app->inspector->row_ref);
	if (!s) {
		dbg(0,"sample not found");
	} else {
		//show the rename widget:
		gtk_entry_set_text(GTK_ENTRY(app->inspector->edit), s->keywords?s->keywords:"");
		sample_unref(s);
	}
	g_object_ref(label); //stops gtk deleting the widget.
	gtk_container_remove(GTK_CONTAINER(parent), label);
	gtk_container_add   (GTK_CONTAINER(parent), app->inspector->edit);
	gtk_widget_show(edit);

	//our focus-out could be improved by properly focussing other widgets when clicked on (widgets that dont catch events?).

	if(handler1) g_signal_handler_disconnect((gpointer)edit, handler1);

	handler1 = g_signal_connect((gpointer)app->inspector->edit, "focus-out-event", G_CALLBACK(tag_edit_stop), GINT_TO_POINTER(0));

	/*
	if(handler2) g_signal_handler_disconnect((gpointer)arrange->wrename, handler2);
	handler2 =   g_signal_connect(G_OBJECT(arrange->wrename), "key-press-event", 
									G_CALLBACK(track_entry_key_press), GINT_TO_POINTER(tnum));//traps the RET key.
	*/

	//grab_focus allows us to start typing imediately. It also selects the whole string.
	gtk_widget_grab_focus(GTK_WIDGET(app->inspector->edit));
}


static void
tag_edit_stop(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
  /*
  tidy up widgets and notify core of label change.
  called following a focus-out-event for the gtkEntry renaming widget.
  -track number should correspond to the one being edited...???

  warning! if called incorrectly, this fn can cause mysterious segfaults!!
  Is it something to do with multiple focus-out events? This function causes the
  rename widget to lose focus, it can run a 2nd time before completing the first: segfault!
  Currently it works ok if focus is removed before calling.
  */

  //static gboolean busy = false;
  //if(busy){"track_label_edit_stop(): busy!\n"; return;} //only run once at a time!
  //busy = true;

	InspectorPriv* i = app->inspector->priv;
	GtkWidget* edit = app->inspector->edit;
	GtkWidget* parent = i->tags_ev;

  //g_signal_handler_disconnect((gpointer)edit, handler_id);
  
  //printf("track_label_edit_stop(): wrename_parent=%p.\n", arrange->wrename->parent);
  //printf("track_label_edit_stop(): label_parent  =%p.\n", trkA[tnum]->wlabel);
  
  //printf("w:%p is_widget: %i\n", trkA[tnum]->wlabel, GTK_IS_WIDGET(trkA[tnum]->wlabel));

  //make sure the rename widget is being shown:
  /*
  if(edit->parent != parent){printf(
    "tags_edit_stop(): info: entry widget not in correct container! trk probably changed. parent=%p\n",
	   							arrange->wrename->parent); }
  */
	   
	//change the text in the label:
	const char * newtxt = gtk_entry_get_text(GTK_ENTRY(edit));
	const char* keywords = (newtxt && strlen(newtxt)) ? newtxt : "<no tags>";
	gtk_label_set_text(GTK_LABEL(i->tags), keywords);

	//update the data:
	//update the string for the channel that the current track is using:
	//int ch = track_get_ch_idx(tnum);
	row_set_tags_from_id(app->inspector->row_id, app->inspector->row_ref, (void*)newtxt);

	//swap back to the normal label:
	gtk_container_remove(GTK_CONTAINER(parent), edit);
	//gtk_container_remove(GTK_CONTAINER(arrange->wrename->parent), arrange->wrename);
	//printf("track_label_edit_stop(): wrename unparented.\n");

	gtk_container_add(GTK_CONTAINER(parent), i->tags);
	g_object_unref(i->tags); //remove 'artificial' ref added in edit_start.
	dbg(0, "finished.");
	//busy = false;
}
