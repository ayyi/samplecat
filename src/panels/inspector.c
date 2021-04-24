/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include "debug/debug.h"
#include "file_manager/support.h" // to_utf8()
#include "file_manager/mimetype.h"
#include "support.h"
#include "model.h"
#include "application.h"
#include "sample.h"
#include "editablelabelbutton.h"
#ifdef USE_TRACKER
  #include "src/db/tracker.h"
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

#define _gtk_tree_row_reference_free0(var) ((var == NULL) ? NULL : (var = (gtk_tree_row_reference_free (var), NULL)))

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

struct _InspectorPrivate
{
	unsigned       row_id;

	GtkWidget*     name;
	GtkWidget*     image;
	GtkWidget*     table;
	GtkWidget*     filename;
	GtkWidget*     tags;
	GtkWidget*     length;
	GtkWidget*     frames;
	GtkWidget*     samplerate;
	GtkWidget*     channels;
	GtkWidget*     mimetype;
	GtkWidget*     level;
	GtkWidget*     bitrate;
	GtkWidget*     bitdepth;
	RowRange       ebur;
	RowRange       meta;
	GtkWidget*     vbox;
	GtkWidget*     text;
	GtkTextBuffer* notes;
	GtkTreeRowReference* row_ref; // TODO remove

	bool           wide;
};

static GObject* inspector_constructor   (GType, guint, GObjectConstructParam*);
static void     inspector_finalize      (GObject*);
static void     inspector_class_init    (InspectorClass*);
static void     inspector_size_allocate (GtkWidget*, GdkRectangle*);

G_DEFINE_TYPE_WITH_PRIVATE (Inspector, inspector, GTK_TYPE_SCROLLED_WINDOW)

static void inspector_clear              (Inspector*);
static void inspector_update             (SamplecatModel*, Sample*, gpointer);
static void inspector_set_labels         (Inspector*, Sample*);
static void hide_fields                  (Inspector*);
static void show_fields                  (Inspector*);
static void inspector_remove_rows        (Inspector*, int, int, int);
static void inspector_remove_cells       (Inspector*, RowRange*);
static bool on_notes_focus_out           (GtkWidget*, GdkEvent*, gpointer);


static void
inspector_init (Inspector * self)
{
	self->priv = inspector_get_instance_private(self);
}


static void
inspector_finalize (GObject * obj)
{
	G_OBJECT_CLASS (inspector_parent_class)->finalize (obj);
}


Inspector*
construct (GType object_type)
{
	return (Inspector*) g_object_new (object_type, NULL);
}


GtkWidget*
inspector_new ()
{
	// detailed information on a single sample. LHS of main window.

	Inspector* inspector = construct (TYPE_INSPECTOR);
	inspector->preferred_height = 200;
	inspector->show_waveform = true;
	InspectorPrivate* i = inspector->priv;

	GtkWidget* scroll = (GtkWidget*)inspector;
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

	// left align the label
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(i->vbox), align1, EXPAND_FALSE, FILL_FALSE, 0);

	// sample name
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

	GtkWidget* table = i->table = gtk_table_new(n_rows + 1, 3, HOMOGENOUS);

	int s = 0;
	// first two rows are special case - colspan=2

	GtkWidget* label = i->filename = label_new("Filename");
	gtk_misc_set_padding(GTK_MISC(label), MARGIN_LEFT, 0);
	gtk_label_set_ellipsize((GtkLabel*)i->filename, PANGO_ELLIPSIZE_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), label, 0, 3, s, s+1, GTK_FILL, GTK_FILL, 0, 0);
	s++;

	i->tags = (GtkWidget*)editable_label_button_new("Tags");
	gtk_table_attach(GTK_TABLE(table), i->tags, 0, 2, s, s+1, GTK_FILL, GTK_FILL, 0, 0);
	s++;

	for(;s<n_rows;s++){
		GtkWidget* label = label_new(rows[s].description);
		gtk_misc_set_padding(GTK_MISC(label), MARGIN_LEFT, 0);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, s+1, s+2, GTK_FILL, GTK_FILL, 0, 0);

		label = *rows[s].widget = label_new("");
		gtk_table_attach(GTK_TABLE(table), label, 1, 2, s+1, s+2, GTK_FILL, GTK_FILL, 0, 0);
	}

	gtk_box_pack_start(GTK_BOX(i->vbox), table, EXPAND_FALSE, FILL_FALSE, 0);

	{
		// notes box

		i->text = gtk_text_view_new();
		i->notes = gtk_text_view_get_buffer(GTK_TEXT_VIEW(i->text));
		gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(i->text), false);
		gtk_widget_set_sensitive(i->text, false);

#ifdef USE_TRACKER
		if(BACKEND_IS_TRACKER){
			gtk_widget_set_no_show_all(i->text, true);
		}
#endif
		g_signal_connect(G_OBJECT(i->text), "focus-out-event", G_CALLBACK(on_notes_focus_out), inspector);
		gtk_box_pack_start(GTK_BOX(i->vbox), i->text, false, true, 2);

		GValue gval = {0,};
		g_value_init(&gval, G_TYPE_CHAR);
		g_value_set_schar(&gval, MARGIN_LEFT);
		g_object_set_property(G_OBJECT(i->text), "border-width", &gval);

		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(i->text), GTK_WRAP_WORD_CHAR);
		gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(i->text), 5);
		gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(i->text), 5);
	}

	g_signal_connect((gpointer)samplecat.model, "selection-changed", G_CALLBACK(inspector_update), inspector);

	void sample_changed(SamplecatModel* m, Sample* sample, int what, void* data, gpointer _inspector)
	{
		if (sample->id == ((Inspector*)_inspector)->priv->row_id)
			inspector_update(m, sample, _inspector);
	}
	g_signal_connect((gpointer)samplecat.model, "sample-changed", G_CALLBACK(sample_changed), inspector);

	gtk_widget_set_size_request((GtkWidget*)inspector, 20, 20);

#ifdef USE_GDL
	void _inspector_on_layout_changed(GObject* object, gpointer user_data)
	{
	}

	Idle* idle = idle_new(_inspector_on_layout_changed, NULL);
	g_signal_connect(app, "layout-changed", (GCallback)idle->run, idle);
#endif


	void on_tags_changed(GtkWidget* label, const char* new_text, gpointer user_data)
	{
		Inspector* inspector = user_data;
		InspectorPrivate* i = inspector->priv;

		g_return_if_fail(i->row_id);
		g_return_if_fail(i->row_ref);

		dbg(1, "id=%i", i->row_id);

		Sample* sample = samplecat_list_store_get_sample_by_row_ref(i->row_ref);
		g_return_if_fail(sample);
		if(samplecat_model_update_sample (samplecat.model, sample, COL_KEYWORDS, (void*)new_text)){
			statusbar_print(1, "keywords updated");
		}else{
			statusbar_print(1, "failed to update keywords");
		}
		sample_unref(sample);
	}
	g_signal_connect(G_OBJECT(i->tags), "edited", G_CALLBACK(on_tags_changed), inspector);

	return scroll;
}


static GObject *
inspector_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (inspector_parent_class);
	return parent_class->constructor (type, n_construct_properties, construct_properties);
}


static void
inspector_class_init (InspectorClass * klass)
{
	inspector_parent_class = g_type_class_peek_parent (klass);

	((GtkWidgetClass *) klass)->size_allocate = (void (*) (GtkWidget*, GdkRectangle*)) inspector_size_allocate;

	G_OBJECT_CLASS (klass)->constructor = inspector_constructor;
	G_OBJECT_CLASS (klass)->finalize = inspector_finalize;
}


static void
inspector_size_allocate (GtkWidget* base, GdkRectangle* allocation)
{
	Inspector* inspector = (Inspector*) base;
	InspectorPrivate* i = inspector->priv;
	g_return_if_fail (allocation);

	GTK_WIDGET_CLASS (inspector_parent_class)->size_allocate ((GtkWidget*) G_TYPE_CHECK_INSTANCE_CAST (inspector, GTK_TYPE_SCROLLED_WINDOW, GtkScrolledWindow), allocation);

	bool wide = allocation->width > 320 && allocation->width > allocation->height;
	if(wide != i->wide){
		dbg(1, "-> allocated %i x %i wide=%i %s", allocation->width, allocation->height, wide, wide != i->wide ? "CHANGED" : "");
		inspector_remove_cells(inspector, &i->ebur);
		if(wide) i->meta.start = 3;
		i->wide = wide;
		if(i->row_ref){
			Sample* sample = samplecat_list_store_get_sample_by_row_ref(i->row_ref);
			if(sample){
				inspector_set_labels(inspector, sample);
				sample_unref(sample);
			}
		}
	}

	gtk_widget_set_size_request(i->filename, allocation->width / (wide ? 2 : 1) -2, -1);
	gtk_widget_set_size_request(i->name, allocation->width / (wide ? 2 : 1) -2, -1);
}


static void
inspector_add_ebu_cells (Inspector* inspector)
{
	InspectorPrivate* i = inspector->priv;

	if(i->ebur.first_child) return;

	#define SPACER_ROW 1
	guint s = i->wide ? 2 : 11;
	guint x;
	if(i->wide){
		x = 2;
		gtk_table_resize(GTK_TABLE(i->table), GTK_TABLE(i->table)->nrows, 7);
		simple_table_set_col_spacing(GTK_TABLE(i->table), 2, 10);
	}else{
		x = 0;
	}

	GtkWidget* ebur_rows[N_EBUR_ROWS];
	int r; for(r=s+SPACER_ROW;r<N_EBUR_ROWS+s+SPACER_ROW;r++){
		GtkWidget* row00 = ebur_rows[r - s - SPACER_ROW] = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(row00), 0.0, 0.5);
		gtk_misc_set_padding(GTK_MISC(row00), MARGIN_LEFT, 0);
		gtk_table_attach(GTK_TABLE(i->table), row00, x+0, x+1, r, r+1, GTK_FILL, GTK_FILL, 0, 0);

		GtkWidget* row01 = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(row01), 1.0, 0.5);
		gtk_table_attach(GTK_TABLE(i->table), row01, x+1, x+2, r, r+1, GTK_FILL, GTK_FILL, 0, 0);
	}
	i->ebur = (RowRange){
		.first_child = ebur_rows[0],
		.n = N_EBUR_ROWS
	};
	if(!i->wide) i->meta.start = r + SPACER_ROW;

	gtk_widget_show_all(i->table);
}


static void
inspector_add_meta_cells (Inspector* inspector, GPtrArray* meta_data)
{
	InspectorPrivate* i = inspector->priv;

	if(!meta_data) return;

	// currently we are not able to add ebu cells after metadata cells so they must be added first.
	if(!i->meta.start) return pwarn("start not set");

	if(i->meta.first_child) inspector_remove_cells(inspector, &i->meta);

	int rows_needed = meta_data->len / 2;
	int n_to_add = rows_needed - i->meta.n;

	int x;
	if(i->wide){
		x = 4;
		i->meta.start = 3;
		simple_table_set_col_spacing(GTK_TABLE(i->table), x-1, 10);
		simple_table_set_col_spacing(GTK_TABLE(i->table), x, 10);
	}else{
		x = 0;
	}

	if(n_to_add > 0){
		GtkWidget* meta_rows[n_to_add];
		int r; for(r = i->meta.start + i->meta.n; r < i->meta.start + meta_data->len/2; r++){
			GtkWidget* cell1 = meta_rows[r - i->meta.start - i->meta.n] = gtk_label_new("");
			gtk_misc_set_alignment(GTK_MISC(cell1), 0.0, 0.5);
			gtk_misc_set_padding(GTK_MISC(cell1), MARGIN_LEFT, 0);
			gtk_table_attach(GTK_TABLE(i->table), cell1, x, x+1, r, r+1, GTK_FILL, GTK_FILL, 0, 0);

			GtkWidget* cell2 = gtk_label_new("");
			gtk_misc_set_alignment(GTK_MISC(cell2), 0.0, 0.5);
			gtk_label_set_ellipsize((GtkLabel*)cell2, PANGO_ELLIPSIZE_END);
			gtk_widget_set_size_request(cell2, 100, -1); // TODO
			gtk_table_attach(GTK_TABLE(i->table), cell2, x+1, x+2, r, r+1, GTK_FILL, GTK_FILL, 0, 0);
		}
		i->meta.first_child = meta_rows[0];
	}else if(n_to_add < 0){
		inspector_remove_rows(inspector, i->meta.start + meta_data->len / 2, -n_to_add, x);
	}

	i->meta.n = meta_data->len / 2;
	gtk_widget_show_all(i->table);
}


static void
inspector_remove_rows (Inspector* inspector, int n_rows, int n_to_remove, int column)
{
	// remove rows from the end of the table

	InspectorPrivate* i = inspector->priv;

	GList* rows = gtk_container_get_children(GTK_CONTAINER(i->table));
	GList* l = rows; int r = 0;
	for(;l && r < n_to_remove;l=l->next, r++){
		GtkWidget* child = l->data;
		gtk_widget_destroy(child);
		l = l->next, child = l->data;
		gtk_widget_destroy(child);
	}

	if(!i->wide) gtk_table_resize(GTK_TABLE(i->table), n_rows, 3);

	g_list_free(rows);
}


static void
inspector_remove_cells (Inspector* inspector, RowRange* cells)
{
	InspectorPrivate* i = inspector->priv;

	if(!cells->first_child) return;

	GList* children = g_list_copy(((GtkTable*)i->table)->children);
	GList* l = children;
	for(;l;l=l->next){
		GtkSimpleTableChild* child = l->data;
		if(child->widget == cells->first_child){
			int n = 0;
			for(;l && n<(cells->n*2); l=l->prev,n++){
				GtkSimpleTableChild* child = l->data;
				gtk_widget_destroy(child->widget);
			}
			break;
		}
	}
	g_list_free(children);
	int start = cells->start;
	*cells = (RowRange){0, start};
}


static void
inspector_set_labels (Inspector* inspector, Sample* sample)
{
	g_return_if_fail(sample);
	InspectorPrivate* i = inspector->priv;
	GtkLabel* tags = ((EditableLabelButton*)i->tags)->label;

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

	char* ch_str = format_channels(sample->channels);
	char* level  = gain2dbstring(sample->peaklevel);

	char fs_str[32]; samplerate_format(fs_str, sample->sample_rate); strcpy(fs_str + strlen(fs_str), " kHz");
	char length[64]; format_smpte(length, sample->length);
	char frames[32]; sprintf(frames, "%"PRIi64"", sample->frames);
	char bitrate[32]; bitrate_format(bitrate, sample->bit_rate);
	char bitdepth[32]; bitdepth_format(bitdepth, sample->bit_depth);

	char* keywords = (sample->keywords && strlen(sample->keywords)) ? sample->keywords : "<no tags>";
	char* path = to_utf8(sample->full_path);

	gtk_label_set_text(GTK_LABEL(i->name),       sample->name);
	gtk_label_set_text(GTK_LABEL(i->filename),   path);
	gtk_label_set_text(GTK_LABEL(tags),          keywords);
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
		if(!i->ebur.first_child) inspector_add_ebu_cells(inspector);
	} else{
		inspector_remove_cells(inspector, &i->ebur);
		if(!i->wide) i->meta.start = 12;
	}

	if(sample->meta_data && sample->meta_data->len){
		inspector_add_meta_cells(inspector, sample->meta_data);
	}else{
		inspector_remove_cells(inspector, &i->meta);
	}

	GList* rows = g_list_reverse(gtk_container_get_children(GTK_CONTAINER(i->table)));
	GList* l = rows; int r = 0;

	if(ebu && strlen(ebu) > 1){

		gchar** lines = g_strsplit(ebu, "\n", N_EBUR_ROWS + 1);

		for(;l;l=l->next, r++){
			if((GtkWidget*)l->data == i->ebur.first_child) break;
		}
		for(r=0;l && r<N_EBUR_ROWS;l=l->next, r++){
			GtkWidget* child = l->data;
			char* line = lines[r];
			if(!line) break;
			gchar** c = g_strsplit(line, ":", 2);
			gtk_label_set_text(GTK_LABEL(child), c[0]);
			l = l->next, child = l->data;
			gtk_label_set_text(GTK_LABEL(child), c[1]);

			g_strfreev(c);
		}
		g_strfreev(lines);
	}

	if(sample->meta_data){
		char** meta_data = (char**)sample->meta_data->pdata;

		for(;l;l=l->next, r++){
			if((GtkWidget*)l->data == i->meta.first_child) break;
		}

		for(r=0;l && r<sample->meta_data->len/2;l=l->next, r++){
			gtk_label_set_markup(GTK_LABEL(l->data), meta_data[2*r]); // markup used for backwards compatibility. previous format contained italic tags.
			l = l->next;
			gtk_label_set_text(GTK_LABEL(l->data), meta_data[2*r+1]);
		}
	}

	// remove unused rows
	// (apparently it is safe to set size too small - cells will not be removed)
	gtk_table_resize(GTK_TABLE(i->table), i->meta.start + i->meta.n, 3);

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

	show_fields(inspector);
	g_free(level);
	g_free(ch_str);
	g_free(path);

	//store a reference to the row id in the inspector widget:
	// needed to later updated "notes" for that sample
	// as well to check for updates
	// *** row_ref is deprecated. use database id instead.
	i->row_id = sample->id;
	_gtk_tree_row_reference_free0(i->row_ref);
	if(sample->row_ref){
		i->row_ref = gtk_tree_row_reference_copy(sample->row_ref);
	} else {
		dbg(2, "setting row_ref failed");
		i->row_ref = NULL;
		/* can not edit tags or notes w/o reference */
		gtk_widget_hide(GTK_WIDGET(i->text));
		gtk_widget_hide(GTK_WIDGET(i->tags));
	}
}


static void
inspector_update (SamplecatModel* m, Sample* sample, gpointer user_data)
{
	PF;
	Inspector* inspector = (Inspector*)user_data;
	InspectorPrivate* i = inspector->priv;

	if(!sample){
		inspector_clear(inspector);
		gtk_label_set_text(GTK_LABEL(i->name), "");
		return;
	}

	// forget previous inspector item
	_gtk_tree_row_reference_free0(i->row_ref);
	i->row_id = 0;

#ifdef USE_TRACKER
	if(BACKEND_IS_TRACKER){
		if(!sample->length){
			//this sample hasnt been previously selected, and non-db info isnt available.
			//-get the info directly from the file, and set it into the main treeview.
			MIME_type* mime_type = type_from_path(sample->name);
			char mime_string[64];
			snprintf(mime_string, 64, "%s/%s", mime_type->media_type, mime_type->subtype);
			if(mimestring_is_unsupported(mime_string)){
				inspector_clear(inspector);
				gtk_label_set_text(GTK_LABEL(i->filename), basename(sample->name));
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
	if(sample->id > -1 || sample_get_file_info(sample)){
		inspector_set_labels(inspector, sample);
	} else {
		inspector_clear(inspector);
		gtk_label_set_text(GTK_LABEL(i->name), sample->name);
	}
}


static void
inspector_clear (Inspector* inspector)
{
	hide_fields(inspector);
	gtk_image_clear(GTK_IMAGE(inspector->priv->image));
}


static void
hide_fields (Inspector* inspector)
{
	InspectorPrivate* i = inspector->priv;

	GtkWidget* fields[] = {i->filename, i->tags, i->length, i->samplerate, i->channels, i->mimetype, i->table, i->level, i->text, i->bitdepth, i->bitrate};
	int f = 0; for(;f<G_N_ELEMENTS(fields);f++){
		gtk_widget_hide(GTK_WIDGET(fields[f]));
	}
}


static void
show_fields (Inspector* inspector)
{
	InspectorPrivate* i = inspector->priv;

	GtkWidget* fields[] = {i->filename, i->tags, i->length, i->samplerate, i->channels, i->mimetype, i->table, i->level, i->text, i->bitdepth, i->bitrate};
	int f = 0; for(;f<G_N_ELEMENTS(fields);f++){
		gtk_widget_show(GTK_WIDGET(fields[f]));
	}
}


static bool
on_notes_focus_out (GtkWidget* widget, GdkEvent* event, gpointer userdata)
{
	Inspector* inspector = (Inspector*)userdata;
	InspectorPrivate* i = inspector->priv;

	GtkTextBuffer* textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
	g_return_val_if_fail(textbuf, false);

	GtkTextIter start_iter, end_iter;
	gtk_text_buffer_get_start_iter(textbuf, &start_iter);
	gtk_text_buffer_get_end_iter  (textbuf, &end_iter);
	gchar* notes = gtk_text_buffer_get_text(textbuf, &start_iter, &end_iter, true);

	Sample* sample = samplecat.model->selection;
	if(sample && (sample->id == i->row_id) && (!sample->notes || strcmp(notes, sample->notes))){
		statusbar_print(1,
			samplecat_model_update_sample (samplecat.model, sample, COL_X_NOTES, notes)
				? "notes updated"
				: "failed to update notes");
	}
	g_free(notes);
	return false;
}


