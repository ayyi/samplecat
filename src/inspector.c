#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <gtk/gtk.h>
#include "file_manager/file_manager.h"
#include "gqview_view_dir_tree.h"
#include "typedefs.h"
#include "types.h"
#include "support.h"
#include "dh-link.h"
#include "mimetype.h"
#include "main.h"
#include "mimetype.h"
#include "sample.h"
#include "listview.h"
#include "inspector.h"
#ifdef USE_TRACKER
  #include "src/db/tracker.h"
#endif

extern struct _app app;
extern int debug;

static void       inspector_clear           ();
static void       hide_fields               ();
static void       show_fields               ();
static gboolean   inspector_on_tags_clicked (GtkWidget*, GdkEventButton*, gpointer);
static gboolean   on_notes_focus_out        (GtkWidget*, gpointer userdata);
static void       inspector_on_notes_insert (GtkTextView*, gchar *arg1, gpointer);
static void       tag_edit_start            (int tnum);
static void       tag_edit_stop             (GtkWidget*, GdkEventCrossing*, gpointer);



GtkWidget*
inspector_pane()
{
	//close up on a single sample. Bottom left of main window.

	app.inspector = malloc(sizeof(*app.inspector));
	app.inspector->row_ref = NULL;
	Inspector* inspector = app.inspector;

	int margin_left = 5;

	GtkWidget *vbox = app.inspector->widget = gtk_vbox_new(FALSE, 0);

	//left align the label:
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), align1, EXPAND_FALSE, FILL_FALSE, 0);

	// sample name:
	GtkWidget* label1 = app.inspector->name = gtk_label_new("");
	gtk_misc_set_padding(GTK_MISC(label1), margin_left, 5);
	gtk_container_add(GTK_CONTAINER(align1), label1);	

	PangoFontDescription* pangofont = pango_font_description_from_string("San-Serif 18");
	gtk_widget_modify_font(label1, pangofont);
	pango_font_description_free(pangofont);

	//-----------

	app.inspector->image = gtk_image_new_from_pixbuf(NULL);
	gtk_misc_set_alignment(GTK_MISC(app.inspector->image), 0.0, 0.5);
	gtk_misc_set_padding(GTK_MISC(app.inspector->image), margin_left, 2);
	gtk_box_pack_start(GTK_BOX(vbox), app.inspector->image, EXPAND_FALSE, FILL_FALSE, 0);
	gtk_widget_show(app.inspector->image);

	//-----------

	GtkWidget* align2 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), align2, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label2 = app.inspector->filename = gtk_label_new("Filename");
	gtk_misc_set_padding(GTK_MISC(label2), margin_left, 2);
	gtk_container_add(GTK_CONTAINER(align2), label2);	

	//-----------

	GtkWidget* align3 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), align3, EXPAND_FALSE, FILL_FALSE, 0);

	//the tags label has an event box to catch mouse clicks.
	GtkWidget *ev_tags = app.inspector->tags_ev = gtk_event_box_new ();
	gtk_container_add(GTK_CONTAINER(align3), ev_tags);	
	g_signal_connect((gpointer)app.inspector->tags_ev, "button-release-event", G_CALLBACK(inspector_on_tags_clicked), NULL);

	GtkWidget* label3 = app.inspector->tags = gtk_label_new("Tags");
	gtk_misc_set_padding(GTK_MISC(label3), margin_left, 2);
	//gtk_container_add(GTK_CONTAINER(align3), label3);	
	gtk_container_add(GTK_CONTAINER(ev_tags), label3);	

	//-----------

	GtkWidget* align4 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), align4, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label4 = app.inspector->length = gtk_label_new("Length");
	gtk_misc_set_padding(GTK_MISC(label4), margin_left, 2);
	gtk_container_add(GTK_CONTAINER(align4), label4);	

	//-----------
	
	GtkWidget* align5 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), align5, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label5 = app.inspector->samplerate = gtk_label_new("Samplerate");
	gtk_misc_set_padding(GTK_MISC(label5), margin_left, 2);
	gtk_container_add(GTK_CONTAINER(align5), label5);	

	//-----------

	GtkWidget* align6 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align6);
	gtk_box_pack_start(GTK_BOX(vbox), align6, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label6 = app.inspector->channels = gtk_label_new("Channels");
	gtk_misc_set_padding(GTK_MISC(label6), margin_left, 2);
	gtk_container_add(GTK_CONTAINER(align6), label6);	

	//-----------

	GtkWidget* align7 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), align7, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label7 = app.inspector->mimetype = gtk_label_new("Mimetype");
	gtk_misc_set_padding(GTK_MISC(label7), margin_left, 2);
	gtk_container_add(GTK_CONTAINER(align7), label7);	
	
	//-----------

	//notes:

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
	GtkWidget *edit = inspector->edit = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(edit), 64);
	gtk_entry_set_text(GTK_ENTRY(edit), "");
	gtk_widget_show(edit);
	gtk_widget_ref(edit);//stops gtk deleting the unparented widget.

	gtk_widget_show_all(vbox);
	return vbox;
}


void
inspector_update_from_listview(GtkTreePath *path)
{
	PF;
	Inspector* i = app.inspector;
	if(!i) return;
	g_return_if_fail(path);

	if (i->row_ref){ gtk_tree_row_reference_free(i->row_ref); i->row_ref = NULL; }

	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);
	gchar *tags;
	gchar *fpath;
	gchar *fname;
	gchar *mimetype;
	gchar *notes;
	GdkPixbuf* pixbuf = NULL;
	gchar *length;
	int id;
	gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_NAME, &fname, COL_FNAME, &fpath, COL_LENGTH, &length, COL_KEYWORDS, &tags, COL_MIMETYPE, &mimetype, COL_NOTES, &notes, COL_OVERVIEW, &pixbuf, COL_IDX, &id, -1);

	sample* sample = sample_new_from_model(path);
	#ifdef USE_TRACKER
	if(BACKEND_IS_TRACKER){
		g_return_if_fail(length);
		if(!strlen(length)){
			//this sample hasnt been previously selected, and non-db info isnt available.
			//-get the info directly from the file, and set it into the main treeview.
			if(mimestring_is_unsupported(mimetype)){
				inspector_clear();
				gtk_label_set_text(GTK_LABEL(i->name), basename(sample->filename));
				return;
			}
			if(!sample_get_file_sndfile_info(sample)){
				perr("cannot open file?\n");
				return;
			}
			char l[64]; format_time_int(l, sample->length);
			char samplerate_s[32]; float samplerate = sample->sample_rate; samplerate_format(samplerate_s, samplerate);
			gtk_list_store_set(app.store, &iter, COL_LENGTH, l, COL_SAMPLERATE, samplerate_s, COL_CHANNELS, sample->channels, -1);
		}
	}
	#endif

	char* ch_str = channels_format(sample->channels);
	char fs_str[64]; snprintf(fs_str, 63, "%i kHz",      sample->sample_rate);
	char len   [32]; snprintf(len,    31, "%i",          sample->length);

	gtk_label_set_text(GTK_LABEL(i->name),       fname);
	gtk_label_set_text(GTK_LABEL(i->tags),       tags);
	gtk_label_set_text(GTK_LABEL(i->samplerate), fs_str);
	gtk_label_set_text(GTK_LABEL(i->channels),   ch_str);
	gtk_label_set_text(GTK_LABEL(i->filename),   sample->filename);
	gtk_label_set_text(GTK_LABEL(i->mimetype),   mimetype);
	gtk_label_set_text(GTK_LABEL(i->length),     len);
	gtk_text_buffer_set_text(i->notes, notes ? notes : "", -1);
	gtk_image_set_from_pixbuf(GTK_IMAGE(i->image), pixbuf);

	show_fields();
	g_free(ch_str);

	//store a reference to the row id in the inspector widget:
	//g_object_set_data(G_OBJECT(app.inspector->name), "id", GUINT_TO_POINTER(id));
	i->row_id = id;
	i->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), path);
	if(!i->row_ref) perr("setting row_ref failed!\n");

	free(sample);
}


void
inspector_update_from_fileview(GtkTreeView* treeview)
{
	Inspector* i = app.inspector;
	GtkTreeSelection* selection = gtk_tree_view_get_selection(treeview);
	GList* list                 = gtk_tree_selection_get_selected_rows(selection, NULL);
	if(list){
		GtkTreePath* path = list->data;

		GtkTreeModel* model = gtk_tree_view_get_model(treeview);
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, path);

		g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (list);

		sample* sample = sample_new_from_fileview(model, &iter);

		if(sample){
			MIME_type* mime_type = type_from_path(sample->filename);
			char mime_string[64];
			snprintf(mime_string, 64, "%s/%s", mime_type->media_type, mime_type->subtype);
			sample_set_type_from_mime_string(sample, mime_string);

			if(mimestring_is_unsupported(mime_string)){
				inspector_clear();
				gtk_label_set_text(GTK_LABEL(i->name), basename(sample->filename));
			}
			else if(get_file_info(sample)){
				char ch_str[64]; snprintf(ch_str, 64, "%u channels", sample->channels);
				char fs_str[64]; snprintf(fs_str, 64, "%u kHz",      sample->sample_rate);
				char length[64]; snprintf(length, 64, "%u",          sample->length);

				gtk_label_set_text(GTK_LABEL(i->name),       basename(sample->filename));
				gtk_label_set_text(GTK_LABEL(i->filename),   sample->filename);
				gtk_label_set_text(GTK_LABEL(i->tags),       "");
				gtk_label_set_text(GTK_LABEL(i->length),     length);
				gtk_label_set_text(GTK_LABEL(i->channels),   ch_str);
				gtk_label_set_text(GTK_LABEL(i->samplerate), fs_str);
				gtk_label_set_text(GTK_LABEL(i->mimetype),   mime_string);
				gtk_text_buffer_set_text(app.inspector->notes, "", -1);
				gtk_image_clear(GTK_IMAGE(app.inspector->image));

				show_fields();
			}
		}
	}
}


static void
inspector_clear()
{
	Inspector* i = app.inspector;
	if(!i) return;

	hide_fields();
	gtk_image_clear(GTK_IMAGE(i->image));
}


static void
hide_fields()
{
	Inspector* i = app.inspector;
	GtkWidget* fields[] = {i->filename, i->tags, i->length, i->samplerate, i->channels, i->mimetype};
	int f = 0; for(;f<G_N_ELEMENTS(fields);f++){
		gtk_widget_hide(fields[f]);
	}
}


static void
show_fields()
{
	Inspector* i = app.inspector;
	GtkWidget* fields[] = {i->filename, i->tags, i->length, i->samplerate, i->channels, i->mimetype};
	int f = 0; for(;f<G_N_ELEMENTS(fields);f++){
		gtk_widget_show(fields[f]);
	}
}


static gboolean
inspector_on_tags_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	/*
	a single click on the Tags label puts us into edit mode.
	*/

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
	if(!backend.update_notes) return FALSE;

	GtkTextBuffer* textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
	g_return_val_if_fail(textbuf, false);

	GtkTextIter start_iter, end_iter;
	gtk_text_buffer_get_start_iter(textbuf,  &start_iter);
	gtk_text_buffer_get_end_iter  (textbuf,  &end_iter);
	gchar* notes = gtk_text_buffer_get_text(textbuf, &start_iter, &end_iter, TRUE);
	dbg(2, "start=%i end=%i", gtk_text_iter_get_offset(&start_iter), gtk_text_iter_get_offset(&end_iter));

	unsigned id = app.inspector->row_id;
	if(backend.update_notes(id, notes)){
		statusbar_print(1, "notes updated");
		GtkTreePath *path;
		if((path = gtk_tree_row_reference_get_path(app.inspector->row_ref))){
			GtkTreeIter iter;
			gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);
			gtk_list_store_set(app.store, &iter, COL_NOTES, notes, -1);
			gtk_tree_path_free(path);
		}
	}

	g_free(notes);
	return FALSE;
}


static void
tag_edit_start(int tnum)
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

  static gulong handler1 = 0; // the edit box "focus_out" handler.
  //static gulong handler2 = 0; // the edit box RETURN key trap.
  
  GtkWidget* parent = app.inspector->tags_ev;
  GtkWidget* edit   = app.inspector->edit;
  GtkWidget *label = app.inspector->tags;

  if(!GTK_IS_WIDGET(app.inspector->edit)){ perr("edit widget missing.\n"); return;}
  if(!GTK_IS_WIDGET(label))              { perr("label widget is missing.\n"); return;}

  //show the rename widget:
  gtk_entry_set_text(GTK_ENTRY(app.inspector->edit), gtk_label_get_text(GTK_LABEL(label)));
  gtk_widget_ref(label);//stops gtk deleting the widget.
  gtk_container_remove(GTK_CONTAINER(parent), label);
  gtk_container_add   (GTK_CONTAINER(parent), app.inspector->edit);

  //our focus-out could be improved by properly focussing other widgets when clicked on (widgets that dont catch events?).

  if(handler1) g_signal_handler_disconnect((gpointer)edit, handler1);
  
  handler1 = g_signal_connect((gpointer)app.inspector->edit, "focus-out-event", G_CALLBACK(tag_edit_stop), GINT_TO_POINTER(0));

  /*
  //this signal is no good as it quits when you move the mouse:
  //g_signal_connect ((gpointer)arrange->wrename, "leave-notify-event", G_CALLBACK(track_label_edit_stop), 
  //								GINT_TO_POINTER(tnum));
  
  if(handler2) g_signal_handler_disconnect((gpointer)arrange->wrename, handler2);
  handler2 =   g_signal_connect(G_OBJECT(arrange->wrename), "key-press-event", 
  								G_CALLBACK(track_entry_key_press), GINT_TO_POINTER(tnum));//traps the RET key.
  */

  //grab_focus allows us to start typing imediately. It also selects the whole string.
  gtk_widget_grab_focus(GTK_WIDGET(app.inspector->edit));
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

  GtkWidget* edit = app.inspector->edit;
  GtkWidget* parent = app.inspector->tags_ev;

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
  gtk_label_set_text(GTK_LABEL(app.inspector->tags), gtk_entry_get_text(GTK_ENTRY(edit)));
  //printf("track_label_edit_stop(): text set.\n");

  //update the data:
  //update the string for the channel that the current track is using:
  //int ch = track_get_ch_idx(tnum);
  row_set_tags_from_id(app.inspector->row_id, app.inspector->row_ref, gtk_entry_get_text(GTK_ENTRY(edit)));

  //swap back to the normal label:
  gtk_container_remove(GTK_CONTAINER(parent), edit);
  //gtk_container_remove(GTK_CONTAINER(arrange->wrename->parent), arrange->wrename);
  //printf("track_label_edit_stop(): wrename unparented.\n");
  
  gtk_container_add(GTK_CONTAINER(parent), app.inspector->tags);
  gtk_widget_unref(app.inspector->tags); //remove 'artificial' ref added in edit_start.
  
  dbg(0, "finished.");
  //busy = false;
}


