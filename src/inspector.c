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
  #include <src/db/tracker_.h>
#endif

extern struct _app app;

static gboolean   inspector_on_tags_clicked(GtkWidget*, GdkEventButton*, gpointer user_data);
static gboolean   on_notes_focus_out       (GtkWidget*, gpointer userdata);
static void       on_notes_insert          (GtkTextView*, gchar *arg1, gpointer user_data);


GtkWidget*
inspector_pane()
{
	//close up on a single sample. Bottom left of main window.

	app.inspector = malloc(sizeof(*app.inspector));
	app.inspector->row_ref = NULL;

	int margin_left = 5;

	GtkWidget *vbox = app.inspector->widget = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);

	//left align the label:
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align1);
	gtk_box_pack_start(GTK_BOX(vbox), align1, EXPAND_FALSE, FILL_FALSE, 0);

	// sample name:
	GtkWidget* label1 = app.inspector->name = gtk_label_new("");
	gtk_misc_set_padding(GTK_MISC(label1), margin_left, 5);
	gtk_widget_show(label1);
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
	gtk_widget_show(align2);
	gtk_box_pack_start(GTK_BOX(vbox), align2, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label2 = app.inspector->filename = gtk_label_new("Filename");
	gtk_misc_set_padding(GTK_MISC(label2), margin_left, 2);
	gtk_widget_show(label2);
	gtk_container_add(GTK_CONTAINER(align2), label2);	

	//-----------

	GtkWidget* align3 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align3);
	gtk_box_pack_start(GTK_BOX(vbox), align3, EXPAND_FALSE, FILL_FALSE, 0);

	//the tags label has an event box to catch mouse clicks.
	GtkWidget *ev_tags = app.inspector->tags_ev = gtk_event_box_new ();
	gtk_widget_show(ev_tags);
	gtk_container_add(GTK_CONTAINER(align3), ev_tags);	
	g_signal_connect((gpointer)app.inspector->tags_ev, "button-release-event", G_CALLBACK(inspector_on_tags_clicked), NULL);

	GtkWidget* label3 = app.inspector->tags = gtk_label_new("Tags");
	gtk_misc_set_padding(GTK_MISC(label3), margin_left, 2);
	gtk_widget_show(label3);
	//gtk_container_add(GTK_CONTAINER(align3), label3);	
	gtk_container_add(GTK_CONTAINER(ev_tags), label3);	
	

	//-----------

	GtkWidget* align4 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align4);
	gtk_box_pack_start(GTK_BOX(vbox), align4, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label4 = app.inspector->length = gtk_label_new("Length");
	gtk_misc_set_padding(GTK_MISC(label4), margin_left, 2);
	gtk_widget_show(label4);
	gtk_container_add(GTK_CONTAINER(align4), label4);	

	//-----------
	
	GtkWidget* align5 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align5);
	gtk_box_pack_start(GTK_BOX(vbox), align5, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label5 = app.inspector->samplerate = gtk_label_new("Samplerate");
	gtk_misc_set_padding(GTK_MISC(label5), margin_left, 2);
	gtk_widget_show(label5);
	gtk_container_add(GTK_CONTAINER(align5), label5);	

	//-----------

	GtkWidget* align6 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align6);
	gtk_box_pack_start(GTK_BOX(vbox), align6, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label6 = app.inspector->channels = gtk_label_new("Channels");
	gtk_misc_set_padding(GTK_MISC(label6), margin_left, 2);
	gtk_widget_show(label6);
	gtk_container_add(GTK_CONTAINER(align6), label6);	

	//-----------

	GtkWidget* align7 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align7);
	gtk_box_pack_start(GTK_BOX(vbox), align7, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label7 = app.inspector->mimetype = gtk_label_new("Mimetype");
	gtk_misc_set_padding(GTK_MISC(label7), margin_left, 2);
	gtk_widget_show(label7);
	gtk_container_add(GTK_CONTAINER(align7), label7);	
	
	//-----------

	//notes:

	GtkTextBuffer *txt_buf1;
	GtkWidget *text1 = gtk_text_view_new();
	//gtk_widget_show(text1);
	txt_buf1 = app.inspector->notes = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text1));
	//gtk_text_buffer_set_text(txt_buf1, "this sample works really well on mid-tempo tracks", -1);
	gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(text1), FALSE);
	g_signal_connect(G_OBJECT(text1), "focus-out-event", G_CALLBACK(on_notes_focus_out), NULL);
	g_signal_connect(G_OBJECT(text1), "insert-at-cursor", G_CALLBACK(on_notes_insert), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), text1, FALSE, TRUE, 2);

	GValue gval = {0,};
	g_value_init(&gval, G_TYPE_CHAR);
	g_value_set_char(&gval, margin_left);
	g_object_set_property(G_OBJECT(text1), "border-width", &gval);

	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text1), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(text1), 5);
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(text1), 5);

	//invisible edit widget:
	GtkWidget *edit = app.inspector->edit = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(edit), 64);
	gtk_entry_set_text(GTK_ENTRY(edit), "");
	gtk_widget_show(edit);
	/*
	g_signal_connect(G_OBJECT(entry), "focus-out-event", G_CALLBACK(new_search), NULL);
	//this is supposed to enable REUTRN to enter the text - does it work?
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	//g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_entry_activate), NULL);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(new_search), NULL);
	*/
	gtk_widget_ref(edit);//stops gtk deleting the widget.

	//this also sets the margin:
	//gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(text1), GTK_TEXT_WINDOW_LEFT, 20);

	return vbox;
}


void
inspector_update_from_listview(GtkTreePath *path)
{
	if(!app.inspector) return;
	ASSERT_POINTER(path, "path");

	if (app.inspector->row_ref){ gtk_tree_row_reference_free(app.inspector->row_ref); app.inspector->row_ref = NULL; }

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
			if(!get_file_info_sndfile(sample)){
				perr("cannot open file?\n");
				return;
			}
			char l[64]; format_time_int(l, sample->length);
			char samplerate_s[32]; float samplerate = sample->sample_rate; samplerate_format(samplerate_s, samplerate);
			gtk_list_store_set(app.store, &iter, COL_LENGTH, l, COL_SAMPLERATE, samplerate_s, COL_CHANNELS, sample->channels, -1);
		}
	}
	#endif

	char ch_str[64]; snprintf(ch_str, 63, "%u channels", sample->channels);
	char fs_str[64]; snprintf(fs_str, 63, "%i kHz",      sample->sample_rate);
	char len   [32]; snprintf(len,    31, "%i",          sample->length);

	gtk_label_set_text(GTK_LABEL(app.inspector->name),       fname);
	gtk_label_set_text(GTK_LABEL(app.inspector->tags),       tags);
	gtk_label_set_text(GTK_LABEL(app.inspector->samplerate), fs_str);
	gtk_label_set_text(GTK_LABEL(app.inspector->channels),   ch_str);
	gtk_label_set_text(GTK_LABEL(app.inspector->filename),   sample->filename);
	gtk_label_set_text(GTK_LABEL(app.inspector->mimetype),   mimetype);
	gtk_label_set_text(GTK_LABEL(app.inspector->length),     len);
#ifdef USE_TRACKER
	gtk_list_store_set(app.store, &iter, COL_OVERVIEW, sample->pixbuf, -1); //check this
	gtk_image_set_from_pixbuf(GTK_IMAGE(app.inspector->image), sample->pixbuf);
#else
	gtk_text_buffer_set_text(app.inspector->notes, notes ? notes : "", -1);
	gtk_image_set_from_pixbuf(GTK_IMAGE(app.inspector->image), pixbuf);
#endif

	//store a reference to the row id in the inspector widget:
	//g_object_set_data(G_OBJECT(app.inspector->name), "id", GUINT_TO_POINTER(id));
	app.inspector->row_id = id;
	app.inspector->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), path);
	if(!app.inspector->row_ref) perr("setting row_ref failed!\n");

	free(sample);
}


void
inspector_update_from_fileview(GtkTreeView* treeview)
{
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

			if(get_file_info(sample)){
				char ch_str[64]; snprintf(ch_str, 64, "%u channels", sample->channels);
				char fs_str[64]; snprintf(fs_str, 64, "%u kHz",      sample->sample_rate);
				char length[64]; snprintf(length, 64, "%u",          sample->length);

				//GdkPixbuf* pixbuf = NULL;

				gtk_label_set_text(GTK_LABEL(app.inspector->name),       basename(sample->filename));
				gtk_label_set_text(GTK_LABEL(app.inspector->filename),   sample->filename);
				gtk_label_set_text(GTK_LABEL(app.inspector->tags),       "");
				gtk_label_set_text(GTK_LABEL(app.inspector->length),     length);
				gtk_label_set_text(GTK_LABEL(app.inspector->channels),   ch_str);
				gtk_label_set_text(GTK_LABEL(app.inspector->samplerate), fs_str);
				gtk_label_set_text(GTK_LABEL(app.inspector->mimetype),   mime_string);
				gtk_text_buffer_set_text(app.inspector->notes, "", -1);
				//gtk_image_set_from_pixbuf(GTK_IMAGE(app.inspector->image), pixbuf);
				gtk_image_clear(GTK_IMAGE(app.inspector->image));
			}
		}
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
on_notes_insert(GtkTextView *textview, gchar *arg1, gpointer user_data)
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


