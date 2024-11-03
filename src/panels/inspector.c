/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 | Inspector
 |
 | Show detailed information on a single sample.
 |
 */

#include "config.h"
#include <gtk/gtk.h>
#include "gtk/scrolled_window.h"
#include "debug/debug.h"
#include "file_manager/support.h" // to_utf8()
#include "file_manager/mimetype.h"
#include "support.h"
#include "model.h"
#include "application.h"
#include "sample.h"
#include "gdl/utils.h"
#include "widgets/tagged-entry.h"

#define TYPE_INSPECTOR            (inspector_get_type ())
#define INSPECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_INSPECTOR, Inspector))
#define INSPECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_INSPECTOR, InspectorClass))
#define IS_INSPECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_INSPECTOR))
#define IS_INSPECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_INSPECTOR))
#define INSPECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_INSPECTOR, InspectorClass))

typedef struct _Inspector Inspector;
typedef struct _InspectorClass InspectorClass;
typedef struct _InspectorPrivate InspectorPrivate;

struct _Inspector
{
	GtkScrolledWindow parent;
	InspectorPrivate* priv;
	gboolean          show_waveform;
	int               preferred_height;
};

struct _InspectorClass {
	GtkScrolledWindowClass parent_class;
};

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

typedef struct {
	char*      name;
	GtkWidget* widget;
} Row;

typedef struct {
	Row filename;
	Row tags;
	Row length;
	Row frames;
	Row samplerate;
	Row channels;
	Row mimetype;
	Row level;
	Row bitrate;
	Row bitdepth;
} Rows;

Rows rows = {
	{"filename"  },
	{"tags",     },
	{"length",   },
	{"frames",   },
	{"samplerate"},
	{"channels"  },
	{"mimetype", },
	{"level",    },
	{"bitrate",  },
	{"bitdepth", },
};
#define N_ROWS (sizeof rows / sizeof(Row))

#define ROW(NAME) (i->rows.a.NAME.widget)

struct _InspectorPrivate
{
	unsigned       row_id;

	GtkWidget*     name;
	GtkWidget*     image;
	GtkWidget*     table;
	union {
		Row  row[N_ROWS];
		Rows a;
	}              rows;
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
static void     inspector_size_allocate (GtkWidget*, int, int, int);

G_DEFINE_TYPE_WITH_PRIVATE (Inspector, inspector, GTK_TYPE_SCROLLED_WINDOW)

static void inspector_clear              (Inspector*);
static void inspector_update             (SamplecatModel*, Sample*, gpointer);
static void inspector_set_labels         (Inspector*, Sample*);
static void hide_fields                  (Inspector*);
static void show_fields                  (Inspector*);
static void inspector_remove_cells       (Inspector*, RowRange*);
#ifdef GTK4_TODO
static bool on_notes_focus_out           (GtkWidget*, GdkEvent*, gpointer);
#endif


static void
inspector_init (Inspector* inspector)
{
	inspector->priv = inspector_get_instance_private(inspector);

	inspector->preferred_height = 200;
	inspector->show_waveform = true;
	InspectorPrivate* i = inspector->priv;
	i->rows.a = rows;

	GtkWidget* scroll = (GtkWidget*)inspector;
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	i->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), i->vbox);

	GtkWidget* label_new (const char* text)
	{
		GtkWidget* label = gtk_label_new (text);
		gtk_widget_set_halign (label, GTK_ALIGN_START);
		gtk_widget_set_sensitive(label, false);
		return label;
	}

	// sample name
	GtkWidget* label1 = i->name = label_new("");
	gtk_label_set_ellipsize((GtkLabel*)label1, PANGO_ELLIPSIZE_MIDDLE);
//	gtk_misc_set_padding(GTK_MISC(i->name), MARGIN_LEFT, 5);
	gtk_box_append(GTK_BOX(i->vbox), i->name);	

#ifdef GTK4_TODO
	PangoFontDescription* pangofont = pango_font_description_from_string("Sans-Serif 18");
	gtk_widget_modify_font(label1, pangofont);
	pango_font_description_free(pangofont);
#endif

	//-----------

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	i->image = gtk_image_new_from_pixbuf(NULL);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
	gtk_widget_set_valign (i->image, GTK_ALIGN_START);
//	gtk_misc_set_padding(GTK_MISC(i->image), MARGIN_LEFT, 2);
	gtk_box_append(GTK_BOX(i->vbox), i->image);

	GtkWidget* table = i->table = gtk_grid_new();

	int s = 0;
	// first two rows are special case - colspan=2

	i->rows.a.filename.widget = label_new("Filename");
//	gtk_misc_set_padding(GTK_MISC(label), MARGIN_LEFT, 0);
	gtk_label_set_ellipsize((GtkLabel*)ROW(filename), PANGO_ELLIPSIZE_MIDDLE);
	gtk_grid_attach(GTK_GRID(table), ROW(filename), 0, s, 2, 1);
	s++;

#ifdef GTK4_TODO
	i->rows.a.tags.widget = (GtkWidget*)editable_label_button_new("Tags");
#else
	{
		ROW(tags) = demo_tagged_entry_new();

		void set_tags (DemoTaggedEntry* entry)
		{
			const char* text = gtk_editable_get_text(GTK_EDITABLE(entry));

			GPtrArray* tags = tagged_entry_get_tags(entry);
			g_ptr_array_add(tags, (char*)text);
			char str[128] = {0,};
			for (int i=0;i<tags->len;i++) {
				if (i) g_strlcat(str, " ", 128);
				g_strlcat(str, (char*)g_ptr_array_index(tags, i), 128);
			}
			g_ptr_array_free(tags, false);

			if (samplecat_model_update_sample(samplecat.model, samplecat.model->selection, COL_KEYWORDS, (void*)str)) {
				statusbar_print(1, "keywords updated");
			} else {
				statusbar_print(1, "failed to update keywords");
			}
		}

		void add_tag (GtkButton* button, DemoTaggedEntry* entry)
		{
			set_tags(entry);
		}

		GtkWidget* button = gtk_button_new_with_mnemonic ("Add _Tag");
		g_signal_connect (button, "clicked", G_CALLBACK (add_tag), ROW(tags));
		gtk_grid_attach(GTK_GRID(table), button, 1, 1, 1, 1);

		void tag_removed (DemoTaggedEntry* entry, gpointer user_data)
		{
			set_tags(entry);
		}

		g_signal_connect (i->rows.a.tags.widget, "tag-removed", G_CALLBACK (tag_removed), i->rows.a.tags.widget);
	}
#endif
	gtk_widget_set_halign (ROW(tags), GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(table), ROW(tags), 0, s, 2, 1);
	s++;

	for (;s<N_ROWS;s++) {
		GtkWidget* label = label_new(i->rows.row[s].name);
		gtk_widget_set_halign (label, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(table), label, 0, s+1, 1, 1);

		label = i->rows.row[s].widget = label_new("");
		gtk_grid_attach(GTK_GRID(table), label, 1, s+1, 1, 1);
	}

	gtk_box_append(GTK_BOX(i->vbox), table);

	{
		// notes box

		i->text = gtk_text_view_new();
		i->notes = gtk_text_view_get_buffer(GTK_TEXT_VIEW(i->text));
		gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(i->text), false);
		gtk_widget_set_sensitive(i->text, false);

#ifdef GTK4_TODO
		g_signal_connect(G_OBJECT(i->text), "focus-out-event", G_CALLBACK(on_notes_focus_out), inspector);
#endif
		gtk_box_append(GTK_BOX(i->vbox), i->text);

#ifdef GTK4_TODO
		GValue gval = {0,};
		g_value_init(&gval, G_TYPE_CHAR);
		g_value_set_schar(&gval, MARGIN_LEFT);
		g_object_set_property(G_OBJECT(i->text), "border-width", &gval);
#endif

		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(i->text), GTK_WRAP_WORD_CHAR);
		gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(i->text), 5);
		gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(i->text), 5);
	}

	g_signal_connect((gpointer)samplecat.model, "selection-changed", G_CALLBACK(inspector_update), inspector);

	void sample_changed (SamplecatModel* m, Sample* sample, int what, void* data, Inspector* inspector)
	{
		if (sample->id == (inspector)->priv->row_id)
			inspector_update(m, sample, inspector);
	}
	g_signal_connect((gpointer)samplecat.model, "sample-changed", G_CALLBACK(sample_changed), inspector);

	gtk_widget_set_size_request((GtkWidget*)inspector, 20, 20);

	void _inspector_on_layout_changed (GObject* object, gpointer user_data)
	{
	}

	Idle* idle = idle_new(_inspector_on_layout_changed, NULL);
	g_signal_connect(app, "layout-changed", (GCallback)idle->run, idle);
}


static void
inspector_finalize (GObject * obj)
{
	G_OBJECT_CLASS (inspector_parent_class)->finalize (obj);
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

	((GtkWidgetClass *) klass)->size_allocate = inspector_size_allocate;

	G_OBJECT_CLASS (klass)->constructor = inspector_constructor;
	G_OBJECT_CLASS (klass)->finalize = inspector_finalize;
}


static void
inspector_size_allocate (GtkWidget* base, int width, int height, int baseline)
{
	Inspector* inspector = (Inspector*) base;
	InspectorPrivate* i = inspector->priv;

	GTK_WIDGET_CLASS (inspector_parent_class)->size_allocate ((GtkWidget*) G_TYPE_CHECK_INSTANCE_CAST (inspector, GTK_TYPE_SCROLLED_WINDOW, GtkScrolledWindow), width, height, baseline);

	bool wide = width > 320 && width > height;
	if (wide != i->wide) {
		dbg(1, "-> allocated %i x %i wide=%i %s", width, height, wide, wide != i->wide ? "CHANGED" : "");
		inspector_remove_cells(inspector, &i->ebur);
		if (wide) i->meta.start = 3;
		i->wide = wide;
		if (i->row_ref) {
			Sample* sample = samplecat_list_store_get_sample_by_row_ref(i->row_ref);
			if (sample) {
				inspector_set_labels(inspector, sample);
				sample_unref(sample);
			}
		}
	}

#ifdef GTK4_TODO
	gtk_widget_set_size_request(i->filename, width / (wide ? 2 : 1) -2, -1);
	gtk_widget_set_size_request(i->name, width / (wide ? 2 : 1) -2, -1);
#endif
}


static void
inspector_add_ebu_cells (Inspector* inspector)
{
	InspectorPrivate* i = inspector->priv;

	if (i->ebur.first_child) return;

	#define SPACER_ROW 1
	guint s = i->wide ? 2 : 11;
	guint x;
	if (i->wide) {
		x = 2;
#ifdef GTK4_TODO
		gtk_table_resize(GTK_TABLE(i->table), GTK_TABLE(i->table)->nrows, 7);
		simple_table_set_col_spacing(GTK_TABLE(i->table), 2, 10);
#endif
	} else {
		x = 0;
	}

	GtkWidget* ebur_rows[N_EBUR_ROWS];
	int r;
	for (r=s+SPACER_ROW;r<N_EBUR_ROWS+s+SPACER_ROW;r++) {
		GtkWidget* row00 = ebur_rows[r - s - SPACER_ROW] = gtk_label_new("");
		gtk_widget_set_halign(row00, GTK_ALIGN_START);
#ifdef GTK4_TODO
		gtk_misc_set_padding(GTK_MISC(row00, MARGIN_LEFT, 0);
#endif
		gtk_grid_attach(GTK_GRID(i->table), row00, x, r, 1, 1);

		GtkWidget* row01 = gtk_label_new("");
		gtk_widget_set_halign(row01, GTK_ALIGN_END);
		gtk_grid_attach(GTK_GRID(i->table), row01, x+1, r, 1, 1);
	}
	i->ebur = (RowRange){
		.first_child = ebur_rows[0],
		.n = N_EBUR_ROWS
	};
	if (!i->wide) i->meta.start = r + SPACER_ROW;
}


#ifdef GTK4_TODO
static void
inspector_add_meta_cells (Inspector* inspector, GPtrArray* meta_data)
{
	InspectorPrivate* i = inspector->priv;

	if (!meta_data) return;

	// currently we are not able to add ebu cells after metadata cells so they must be added first.
	if (!i->meta.start) return pwarn("start not set");

	if (i->meta.first_child) inspector_remove_cells(inspector, &i->meta);

	int rows_needed = meta_data->len / 2;
	int n_to_add = rows_needed - i->meta.n;

	int x;
	if (i->wide) {
		x = 4;
		i->meta.start = 3;
		simple_table_set_col_spacing(GTK_TABLE(i->table), x-1, 10);
		simple_table_set_col_spacing(GTK_TABLE(i->table), x, 10);
	} else {
		x = 0;
	}

	if (n_to_add > 0) {
		GtkWidget* meta_rows[n_to_add];
		for (int r = i->meta.start + i->meta.n; r < i->meta.start + meta_data->len/2; r++) {
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
	} else if(n_to_add < 0) {
	}

	i->meta.n = meta_data->len / 2;
	gtk_widget_show(i->table);
}
#endif


static void
inspector_remove_cells (Inspector* inspector, RowRange* cells)
{
#ifdef GTK4_TODO
	InspectorPrivate* i = inspector->priv;

	if (!cells->first_child) return;

	GList* children = g_list_copy(((GtkTable*)i->table)->children);
	GList* l = children;
	for (;l;l=l->next) {
		GtkSimpleTableChild* child = l->data;
		if (child->widget == cells->first_child) {
			int n = 0;
			for (;l && n<(cells->n*2); l=l->prev,n++) {
				GtkSimpleTableChild* child = l->data;
				gtk_widget_destroy(child->widget);
			}
			break;
		}
	}
	g_list_free(children);
	int start = cells->start;
	*cells = (RowRange){0, start};
#endif
}


static void
inspector_set_labels (Inspector* inspector, Sample* sample)
{
	g_return_if_fail(sample);
	InspectorPrivate* i = inspector->priv;
	Rows* rows = &i->rows.a;
#ifdef GTK4_TODO
	GtkLabel* tags = ((EditableLabelButton*)i->tags)->label;
#endif

	{
		gtk_widget_set_sensitive(i->name, true);
		gtk_widget_set_sensitive(rows->tags.widget, true);
		gtk_widget_set_sensitive(rows->filename.widget, true);
		gtk_widget_set_sensitive(i->text, true);

		for (GtkWidget* c = gtk_widget_get_first_child(i->table); c; (c = gtk_widget_get_next_sibling (c))) {
			gtk_widget_set_sensitive(c, true);
		}
	}

	char* ch_str = format_channels(sample->channels);
	char* level  = gain2dbstring(sample->peaklevel);

	char fs_str[32]; samplerate_format(fs_str, sample->sample_rate); strcpy(fs_str + strlen(fs_str), " kHz");
	char length[64]; format_smpte(length, sample->length);
	char frames[32]; sprintf(frames, "%"PRIi64"", sample->frames);
	char bitrate[32]; bitrate_format(bitrate, sample->bit_rate);
	char bitdepth[32]; bitdepth_format(bitdepth, sample->bit_depth);

	g_autofree char* path = to_utf8(sample->full_path);

	gtk_label_set_text(GTK_LABEL(i->name),       sample->name);
	gtk_label_set_text(GTK_LABEL(rows->filename.widget), path);
	tagged_entry_set_tags_from_string((DemoTaggedEntry*)i->rows.a.tags.widget, sample->keywords);
	gtk_label_set_text(GTK_LABEL(ROW(length)),    length);
	gtk_label_set_text(GTK_LABEL(ROW(frames)),    frames);
	gtk_label_set_text(GTK_LABEL(ROW(samplerate)),fs_str);
	gtk_label_set_text(GTK_LABEL(ROW(channels)),  ch_str);
	gtk_label_set_text(GTK_LABEL(ROW(mimetype)),  sample->mimetype);
	gtk_label_set_text(GTK_LABEL(ROW(level)),     level);
	gtk_label_set_text(GTK_LABEL(ROW(bitrate)),   bitrate);
	gtk_label_set_text(GTK_LABEL(ROW(bitdepth)),  bitdepth);
	gtk_text_buffer_set_text(i->notes,            sample->notes ? sample->notes : "", -1);

	char* ebu = sample->ebur ? sample->ebur : "";
	if (ebu && strlen(ebu) > 1) {
		if (!i->ebur.first_child) inspector_add_ebu_cells(inspector);
	} else {
		inspector_remove_cells(inspector, &i->ebur);
		if (!i->wide) i->meta.start = 12;
	}

#ifdef GTK4_TODO
	if (sample->meta_data && sample->meta_data->len) {
		inspector_add_meta_cells(inspector, sample->meta_data);
	} else {
		inspector_remove_cells(inspector, &i->meta);
	}

	GList* rows = g_list_reverse(gtk_container_get_children(GTK_CONTAINER(i->table)));
	GList* l = rows; int r = 0;

	if (ebu && strlen(ebu) > 1) {

		gchar** lines = g_strsplit(ebu, "\n", N_EBUR_ROWS + 1);

		for (;l;l=l->next, r++) {
			if ((GtkWidget*)l->data == i->ebur.first_child) break;
		}
		for (r=0;l && r<N_EBUR_ROWS;l=l->next, r++) {
			GtkWidget* child = l->data;
			char* line = lines[r];
			if (!line) break;
			gchar** c = g_strsplit(line, ":", 2);
			gtk_label_set_text(GTK_LABEL(child), c[0]);
			l = l->next, child = l->data;
			gtk_label_set_text(GTK_LABEL(child), c[1]);

			g_strfreev(c);
		}
		g_strfreev(lines);
	}

	if (sample->meta_data) {
		char** meta_data = (char**)sample->meta_data->pdata;

		for (;l;l=l->next, r++) {
			if ((GtkWidget*)l->data == i->meta.first_child) break;
		}

		for (r=0;l && r<sample->meta_data->len/2;l=l->next, r++) {
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
		gtk_widget_set_visible(i->image, true);
	} else {
		gtk_widget_set_visible(i->image, false);
	}

#endif
	show_fields(inspector);
	g_free(level);
	g_free(ch_str);

	//store a reference to the row id in the inspector widget:
	// needed to later updated "notes" for that sample
	// as well to check for updates
	// *** row_ref is deprecated. use database id instead.
	i->row_id = sample->id;
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	_gtk_tree_row_reference_free0(i->row_ref);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
	if (sample->row_ref) {
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		i->row_ref = gtk_tree_row_reference_copy(sample->row_ref);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
	} else {
		dbg(2, "setting row_ref failed");
		i->row_ref = NULL;
		/* can not edit tags or notes w/o reference */
		gtk_widget_set_visible(GTK_WIDGET(i->text), false);
		gtk_widget_set_visible(GTK_WIDGET(i->rows.a.tags.widget), false);
	}
}


static void
inspector_update (SamplecatModel* m, Sample* sample, gpointer user_data)
{
	PF;
	Inspector* inspector = (Inspector*)user_data;
	InspectorPrivate* i = inspector->priv;

	if (!sample) {
		inspector_clear(inspector);
		gtk_label_set_text(GTK_LABEL(i->name), "");
		return;
	}

	// forget previous inspector item
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	_gtk_tree_row_reference_free0(i->row_ref);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
	i->row_id = 0;

#ifdef USE_TRACKER
	if (BACKEND_IS_TRACKER) {
		if (!sample->length) {
			// this sample hasnt been previously selected, and non-db info isnt available.
			// -get the info directly from the file, and set it into the main treeview.
			MIME_type* mime_type = type_from_path(sample->name);
			char mime_string[64];
			snprintf(mime_string, 64, "%s/%s", mime_type->media_type, mime_type->subtype);
			if (mimestring_is_unsupported(mime_string)) {
				inspector_clear(inspector);
				gtk_label_set_text(GTK_LABEL(i->filename), basename(sample->name));
				return;
			}
			if (!sample_get_file_info(sample)) {
				perr("cannot open file?\n");
				return;
			}
		}
	}
#endif

	// - check if file is already in DB -> load sample-info
	// - check if file is an audio-file -> read basic info directly from file
	// - else just display the base-name in the inspector..
	if (sample->id > -1 || sample_get_file_info(sample)) {
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
#ifdef GTK4_TODO
	InspectorPrivate* i = inspector->priv;

	GtkWidget* fields[] = {i->filename, i->tags, i->length, SAMPLERATE, CHANNELS, ROW(mimetype), i->table, ROW(level), i->text, i->bitdepth, i->bitrate};
	for (int f=0;f<G_N_ELEMENTS(fields);f++) {
		gtk_widget_set_visible(GTK_WIDGET(fields[f]), false);
	}
#endif
}


static void
show_fields (Inspector* inspector)
{
	InspectorPrivate* i = inspector->priv;
	Rows* rows = &i->rows.a;

	GtkWidget* fields[] = {rows->filename.widget, ROW(tags), rows->length.widget, rows->samplerate.widget, rows->channels.widget, ROW(mimetype), i->table, ROW(level), i->text, rows->bitdepth.widget, rows->bitrate.widget};
	for (int f=0;f<G_N_ELEMENTS(fields);f++) {
		gtk_widget_set_visible(GTK_WIDGET(fields[f]), true);
	}
}


#ifdef GTK4_TODO
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
	if (sample && (sample->id == i->row_id) && (!sample->notes || strcmp(notes, sample->notes))) {
		statusbar_print(1,
			samplecat_model_update_sample (samplecat.model, sample, COL_X_NOTES, notes)
				? "notes updated"
				: "failed to update notes");
	}
	g_free(notes);
	return false;
}
#endif
