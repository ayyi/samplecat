/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 | LibraryView
 |
 | The main Samplecat pane.
 | Contains a GtkColumnView.
 | Uses the SamplecatListStore (GListModel<SampleRow>) model.
 | A GtkMultiSelection provids multi-row selection.
 |
 */

#include "config.h"
#define GDK_VERSION_MIN_REQUIRED GDK_VERSION_4_6
#include <gtk/gtk.h>
#include <pango/pango.h>
#include "debug/debug.h"
#ifdef USE_AYYI
#include <ayyi/ayyi.h>
#endif
#include "gdl/gdl-dock-item.h"
#include "gtk/menu.h"
#include "file_manager/support.h"
#include "file_manager/mimetype.h"
#include "file_manager/pixmaps.h"

#include "types.h"
#include "support.h"
#include "model.h"
#include "application.h"
#include "sample.h"
#include "worker.h"
#include "library.h"
#include "samplecat/list_store.h"



extern GtkWidget* make_panel_context_menu (GtkWidget* widget, int size, MenuDef defn[size]);

typedef struct {
	GdlDockItem          parent_instance;
	GtkWidget*           scroll;
	GtkWidget*           column_view;
	GtkSelectionModel*   selection;
	GtkSortListModel*    sort_model;
	GtkWidget*           menu;

	GtkListItemFactory* factory_icon;
	GtkListItemFactory* factory_name;
	GtkListItemFactory* factory_path;
	GtkListItemFactory* factory_colour;
	GtkListItemFactory* factory_tags;
	GtkListItemFactory* factory_waveform;
	GtkListItemFactory* factory_length;

	gint                selected_id;
} Library;

typedef struct {
	GdlDockItemClass parent_class;
} LibraryClass;

#define TYPE_LIBRARY            (library_get_type ())
#define LIBRARY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_LIBRARY, Library))
#define LIBRARY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_LIBRARY, LibraryClass))
#define IS_LIBRARY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_LIBRARY))
#define IS_LIBRARY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_LIBRARY))
#define LIBRARY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_LIBRARY, LibraryClass))

static gpointer library_parent_class = NULL;
static Library* instance = NULL;

/* Helpers */
static Sample*  library_get_sample_at_index (Library* self, guint position);
static void     library_selection_changed   (GtkSelectionModel*, guint position, guint n_items, gpointer user_data);
static void     library_on_store_changed    (SamplecatListStore*, gpointer);
static void     library_on_player_state     (GObject*, GParamSpec* pspec, gpointer);
static void     library_on_drag_begin       (GtkDragSource*, GdkDrag* drag, gpointer);
static GdkContentProvider*
                library_drag_prepare        (GtkDragSource*, double x, double y, gpointer);

static void     library_update_selected     (GSimpleAction*, GVariant*, gpointer);
static void     library_delete_selected     (GSimpleAction*, GVariant*, gpointer);
static void     library_reset_colours       (GSimpleAction*, GVariant*, gpointer);
static gboolean library_on_colour_drop      (GtkDropTarget*, const GValue*, double, double, gpointer);

static void     library_refresh_colour_swatch (SampleRow* row, gint colour_index, GtkWidget* widget);
static void     library_attach_colour_drop_target (GtkWidget* widget, SampleRow* row);
static void     draw_colour_swatch          (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data);

typedef struct {
	GtkWidget* box;
	GtkWidget* icon;
	GtkWidget* label_primary;
	GtkWidget* label_secondary;
} RowWidgets;


static RowWidgets*
row_widgets_new_icon_text (void)
{
	RowWidgets* rw = SC_NEW(RowWidgets,
		.box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6),
		.icon = gtk_image_new (),
		.label_primary = ({
			GtkWidget* label = gtk_label_new (NULL);
			gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);
			gtk_label_set_xalign (GTK_LABEL (label), 0.0);
			gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
			label;
		}),
	);
	gtk_box_append (GTK_BOX (rw->box), rw->icon);
	gtk_box_append (GTK_BOX (rw->box), rw->label_primary);

	return rw;
}


static RowWidgets*
row_widgets_new_single_label (void)
{
	RowWidgets* rw = SC_NEW(RowWidgets,
		.box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0),
		.label_primary = gtk_label_new (NULL),
	);
	gtk_widget_set_valign (rw->box, GTK_ALIGN_CENTER);
	gtk_label_set_xalign (GTK_LABEL (rw->label_primary), 0.0);
	gtk_label_set_ellipsize (GTK_LABEL (rw->label_primary), PANGO_ELLIPSIZE_MIDDLE);
	gtk_label_set_max_width_chars (GTK_LABEL (rw->label_primary), 48);
	gtk_widget_set_valign (rw->label_primary, GTK_ALIGN_CENTER);
	gtk_box_append (GTK_BOX (rw->box), rw->label_primary);

	return rw;
}

static RowWidgets*
row_widgets_new_path_label (void)
{
	RowWidgets* rw = SC_NEW(RowWidgets,
		.box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0),
		.label_primary = gtk_label_new (NULL),
	);
	gtk_widget_set_valign (rw->box, GTK_ALIGN_CENTER);
	gtk_label_set_xalign (GTK_LABEL (rw->label_primary), 0.0);
	gtk_label_set_ellipsize (GTK_LABEL (rw->label_primary), PANGO_ELLIPSIZE_MIDDLE);
	gtk_label_set_max_width_chars (GTK_LABEL (rw->label_primary), 32);
	gtk_widget_set_valign (rw->label_primary, GTK_ALIGN_CENTER);
	gtk_box_append (GTK_BOX (rw->box), rw->label_primary);

	return rw;
}


static void
bind_icon (GtkListItemFactory* factory, GtkListItem* item, gpointer user_data)
{
	(void)factory;
	GtkWidget* child = gtk_list_item_get_child (item);
	RowWidgets* rw = g_object_get_data (G_OBJECT (child), "rw");
	if (!rw) return;

	SampleRow* row = gtk_list_item_get_item (item);
	if (!row) return;

	library_attach_colour_drop_target (rw->box, row);

	Sample* s = row->sample;

	GdkPixbuf* pix = NULL;
	if (s->mimetype) {
		MIME_type* mt = mime_type_lookup (s->mimetype);
		if (mt) pix = mime_type_get_pixbuf (mt);
	}

	if (pix) {
		gtk_image_set_from_pixbuf (GTK_IMAGE (rw->icon), pix);
	} else {
		gtk_image_clear (GTK_IMAGE (rw->icon));
	}
}


static void
bind_name (GtkListItemFactory* factory, GtkListItem* item, gpointer user_data)
{
	(void)factory;
	GtkWidget* child = gtk_list_item_get_child (item);
	RowWidgets* rw = g_object_get_data (G_OBJECT (child), "rw");
	if (!rw) return;

	SampleRow* row = gtk_list_item_get_item (item);
	if (!row) return;

	library_attach_colour_drop_target (rw->box, row);

	Sample* s = row->sample;

	gtk_label_set_text (GTK_LABEL (rw->label_primary), s->name ? s->name : "");

	if (play && play->sample && play->sample->id == s->id) {
		gtk_widget_add_css_class (rw->box, "playing");
	} else {
		gtk_widget_remove_css_class (rw->box, "playing");
	}
}


static void
bind_path (GtkListItemFactory* factory, GtkListItem* item, gpointer user_data)
{
	(void)factory;
	GtkWidget* child = gtk_list_item_get_child (item);
	RowWidgets* rw = g_object_get_data (G_OBJECT (child), "rw");
	if (!rw) return;

	SampleRow* row = gtk_list_item_get_item (item);
	if (!row) return;

	library_attach_colour_drop_target (rw->box, row);

	Sample* s = row->sample;

	gtk_label_set_text (GTK_LABEL (rw->label_primary), s->sample_dir ? dir_format (s->sample_dir) : "");
}


static void
on_tags_changed (GtkEditable* editable, gpointer user_data)
{
	SampleRow* row = g_object_get_data (G_OBJECT (editable), "row");
	if (!row) return;

	const char* text = gtk_editable_get_text (editable);

	samplecat_model_update_sample (samplecat.model, row->sample, COL_KEYWORDS, (void*)text);

	/* Return to read-only after editing */
	gtk_editable_set_editable (editable, FALSE);
}

static void
on_tags_notify_editing (GObject* obj, GParamSpec* pspec, gpointer user_data)
{
	(void)pspec;
	if (!gtk_editable_label_get_editing (GTK_EDITABLE_LABEL (obj))) {
		on_tags_changed (GTK_EDITABLE (obj), user_data);
	}
}

static gboolean   is_tag_separator               (gunichar ch);
static void       select_word_at_x               (GtkEditable*, double x);
static int        tags_byte_index_from_x         (GtkWidget*, const char*, double x);
static void       underline_word_at_x            (GtkWidget*, double x);
static GtkWidget* editable_label_get_text_widget (GtkWidget*);
static void       apply_text_attributes          (GtkWidget*, PangoAttrList*);

static void
on_tags_hover_enter (GtkEventControllerMotion* motion, double x, double y, gpointer user_data)
{
	(void)motion;
	(void)y;
	GtkWidget* widget = GTK_WIDGET (user_data);
	if (!widget) return;
	underline_word_at_x (widget, x);
}

static GtkWidget*
editable_label_get_text_widget (GtkWidget* widget)
{
	if (!widget) return NULL;

	GtkWidget* cached = g_object_get_data (G_OBJECT (widget), "tag-label");
	if (cached && gtk_widget_get_visible (cached)) return cached;

	GtkWidget* fallback = cached;

#if GTK_CHECK_VERSION(4,10,0)
	if (GTK_IS_EDITABLE (widget)) {
		GtkEditable* delegate = gtk_editable_get_delegate (GTK_EDITABLE (widget));
		if (delegate && GTK_IS_WIDGET (delegate) && GTK_WIDGET (delegate) != widget) {
			GtkWidget* lbl = editable_label_get_text_widget (GTK_WIDGET (delegate));
			if (lbl) {
				g_object_set_data (G_OBJECT (widget), "tag-label", lbl);
				return lbl;
			}
		}
	}
#endif

	if (GTK_IS_LABEL (widget) || GTK_IS_TEXT (widget)) {
		if (gtk_widget_get_visible (widget)) return widget;
		if (!fallback) fallback = widget;
	}

	GtkWidget* child = gtk_widget_get_first_child (widget);
	while (child) {
		GtkWidget* found = editable_label_get_text_widget (child);
		if (found && gtk_widget_get_visible (found)) {
			g_object_set_data (G_OBJECT (widget), "tag-label", found);
			return found;
		}
		if (!fallback && found) fallback = found;
		child = gtk_widget_get_next_sibling (child);
	}
	if (fallback) {
		g_object_set_data (G_OBJECT (widget), "tag-label", fallback);
		return fallback;
	}
	return NULL;
}

static void
apply_text_attributes (GtkWidget* widget, PangoAttrList* attrs)
{
	if (!widget) return;

	if (GTK_IS_LABEL (widget)) {
		gtk_label_set_attributes (GTK_LABEL (widget), attrs);
		gtk_widget_queue_draw (widget);
		return;
	} else if (GTK_IS_TEXT (widget)) {
		gtk_text_set_attributes (GTK_TEXT (widget), attrs);
		gtk_widget_queue_draw (widget);
		return;
	}

	GtkWidget* child = gtk_widget_get_first_child (widget);
	while (child) {
		if (gtk_widget_get_visible (child)) {
			apply_text_attributes (child, attrs);
		}
		child = gtk_widget_get_next_sibling (child);
	}
}

static void
on_tags_hover_leave (GtkEventControllerMotion* motion, gpointer user_data)
{
	(void)motion;
	GtkWidget* widget = GTK_WIDGET (user_data);
	if (!widget) return;
	GtkWidget* label = editable_label_get_text_widget (widget);
	if (label) {
		apply_text_attributes (widget, NULL);
	}
}

static void
on_tags_hover_motion (GtkEventControllerMotion* motion, double x, double y, gpointer user_data)
{
	(void)motion;
	(void)y;
	GtkWidget* widget = GTK_WIDGET (user_data);
	if (!widget) return;
	underline_word_at_x (widget, x);
}

static void
underline_word_at_x (GtkWidget* widget, double x)
{
	if (!widget) return;
	GtkWidget* label = editable_label_get_text_widget (widget);
	const char* text = gtk_editable_get_text (GTK_EDITABLE (widget));
	if (!label || !text) {
		return;
	}

	int byte_index = tags_byte_index_from_x (widget, text, x);
	int text_len = (int)strlen (text);
	if (byte_index < 0) byte_index = text_len;
	if (byte_index > text_len) byte_index = text_len;

	const char* text_end = text + text_len;
	const char* cursor = text + byte_index;
	if (cursor > text_end) cursor = text_end;
	const char* safe_cursor = g_utf8_find_prev_char (text, cursor + 1);
	if (safe_cursor) cursor = safe_cursor;

	const char* start = cursor;
	const char* end = cursor;

	while (start > text) {
		const char* prev = g_utf8_find_prev_char (text, start);
		if (!prev) break;
		gunichar ch = g_utf8_get_char (prev);
		if (is_tag_separator (ch)) break;
		start = prev;
	}

	while (end < text_end) {
		gunichar ch = g_utf8_get_char (end);
		if (is_tag_separator (ch)) break;
		end = g_utf8_next_char (end);
	}

	if (start >= text_end || start == end || is_tag_separator (g_utf8_get_char (start))) {
		apply_text_attributes (widget, NULL);
		return;
	}

	int start_index = (int)(start - text);
	int end_index = (int)(end - text);
	if (end_index <= start_index) {
		apply_text_attributes (widget, NULL);
		return;
	}

	PangoAttrList* attrs = pango_attr_list_new ();
	PangoAttribute* u = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
	u->start_index = start_index;
	u->end_index = end_index;
	pango_attr_list_insert (attrs, u);
	apply_text_attributes (widget, attrs);
	pango_attr_list_unref (attrs);
}

static int
tags_byte_index_from_x (GtkWidget* widget, const char* text, double x)
{
	if (!widget || !text) return -1;

	PangoLayout* layout = gtk_widget_create_pango_layout (widget, text);
	if (!layout) return -1;

	int width = 0;
	int height = 0;
	pango_layout_get_size (layout, &width, &height);

	double x_pango = x * PANGO_SCALE;
	if (x_pango < 0) x_pango = 0;
	if (width > 0 && x_pango > width) x_pango = width;

	int index = -1;
	int trailing = 0;
	if (!pango_layout_xy_to_index (layout, (int)x_pango, 0, &index, &trailing)) {
		g_object_unref (layout);
		return (int)strlen (text);
	}
	g_object_unref (layout);

	if (index < 0) return -1;

	return index + trailing;
}

static gboolean
is_tag_separator (gunichar ch)
{
	return g_unichar_isspace (ch) || ch == ',' || ch == ';' || ch == ':' || ch == '/' || ch == '|' || ch == '\\';
}

static void
select_word_at_x (GtkEditable* editable, double x)
{
	const char* text = gtk_editable_get_text (editable);
	if (!text || !*text) return;

	int byte_index = tags_byte_index_from_x (GTK_WIDGET (editable), text, x);
	int text_len = (int)strlen (text);
	if (byte_index < 0) byte_index = text_len;
	if (byte_index > text_len) byte_index = text_len;

	const char* text_end = text + text_len;
	const char* cursor = text + byte_index;
	if (cursor > text_end) cursor = text_end;
	const char* safe_cursor = g_utf8_find_prev_char (text, cursor + 1);
	if (safe_cursor) cursor = safe_cursor;

	const char* start = cursor;
	const char* end = cursor;

	while (start > text) {
		const char* prev = g_utf8_find_prev_char (text, start);
		if (!prev) break;
		gunichar ch = g_utf8_get_char (prev);
		if (is_tag_separator (ch)) break;
		start = prev;
	}

	while (end < text_end) {
		gunichar ch = g_utf8_get_char (end);
		if (is_tag_separator (ch)) break;
		end = g_utf8_next_char (end);
	}

	gint start_pos = g_utf8_pointer_to_offset (text, start);
	gint end_pos = g_utf8_pointer_to_offset (text, end);

	gtk_editable_set_position (editable, start_pos);
	gtk_editable_select_region (editable, start_pos, end_pos);
}

static void
on_tags_clicked (GtkGestureClick* gesture, int n_press, double x, double y, gpointer user_data)
{
	(void)gesture;
	(void)y;
	GtkWidget* widget = GTK_WIDGET (user_data);
	if (!widget) return;

	const char* text = gtk_editable_get_text (GTK_EDITABLE (widget));
	if (!text) return;

	if (n_press >= 2) {
		gtk_editable_set_editable (GTK_EDITABLE (widget), TRUE);
		gtk_widget_grab_focus (widget);
		select_word_at_x (GTK_EDITABLE (widget), x);
		return;
	}

	if (*text) {
		int byte_index = tags_byte_index_from_x (GTK_WIDGET (widget), text, x);
		int text_len = (int)strlen (text);
		if (byte_index < 0) byte_index = text_len;
		if (byte_index > text_len) byte_index = text_len;

		const char* text_end = text + text_len;
		const char* cursor = text + byte_index;
		if (cursor > text_end) cursor = text_end;
		const char* safe_cursor = g_utf8_find_prev_char (text, cursor + 1);
		if (safe_cursor) cursor = safe_cursor;

		const char* start = cursor;
		const char* end = cursor;

		while (start > text) {
			const char* prev = g_utf8_find_prev_char (text, start);
			if (!prev) break;
			gunichar ch = g_utf8_get_char (prev);
			if (is_tag_separator (ch)) break;
			start = prev;
		}

		while (end < text_end) {
			gunichar ch = g_utf8_get_char (end);
			if (is_tag_separator (ch)) break;
			end = g_utf8_next_char (end);
		}

		if (start < end) {
			gint len_bytes = (gint)(end - start);
			char* word = g_strndup (start, len_bytes);
			observable_string_set (samplecat.model->filters2.search, word);
		} else {
			observable_string_set (samplecat.model->filters2.search, g_strdup (text));
		}
	}
}


static void
bind_tags (GtkListItemFactory* factory, GtkListItem* item, gpointer user_data)
{
	GtkWidget* child = gtk_list_item_get_child (item);
	SampleRow* row = gtk_list_item_get_item (item);
	if (!row || !child) return;

	library_attach_colour_drop_target (child, row);

	Sample* s = row->sample;

	gtk_editable_set_text (GTK_EDITABLE (child), s->keywords ? s->keywords : "");
}


static void
bind_length (GtkListItemFactory* factory, GtkListItem* item, gpointer user_data)
{
	(void)factory;
	GtkWidget* child = gtk_list_item_get_child (item);
	GtkLabel* label = GTK_LABEL (child);

	SampleRow* row = gtk_list_item_get_item (item);
	if (!row) return;

	library_attach_colour_drop_target (GTK_WIDGET (label), row);

	char smpte[32]; format_smpte (smpte, row->sample->length);
	gtk_label_set_text (label, smpte);
}


static void
library_refresh_colour_swatch (SampleRow* row, gint colour_index, GtkWidget* widget)
{
	if (!row) return;
	if (!widget) widget = g_object_get_data (G_OBJECT (row), "swatch-widget");
	if (!widget) return;

	if (colour_index <= 0) {
		gtk_widget_set_visible (widget, FALSE);
		g_object_set_data (G_OBJECT (widget), "swatch-colour", NULL);
		gtk_widget_queue_draw (widget);
		return;
	}

	GdkRGBA rgba = {0};
	if (!palette_rgba_from_index (colour_index, &rgba)) {
		rgba.red = 0.6;
		rgba.green = 0.6;
		rgba.blue = 0.6;
		rgba.alpha = 1.0;
	}

	gtk_widget_set_visible (widget, TRUE);
	GdkRGBA* stored = g_new (GdkRGBA, 1);
	*stored = rgba;
	g_object_set_data_full (G_OBJECT (widget), "swatch-colour", stored, g_free);
	gtk_widget_queue_draw (widget);
}


static void
library_attach_colour_drop_target (GtkWidget* widget, SampleRow* row)
{
	if (!widget || !row) return;

	g_object_set_data_full (G_OBJECT (widget), "row", g_object_ref (row), g_object_unref);

	GtkDropTarget* drop = g_object_get_data (G_OBJECT (widget), "colour-drop-target");
	if (!drop) {
		drop = gtk_drop_target_new (AYYI_TYPE_COLOUR, GDK_ACTION_COPY);
		g_signal_connect (drop, "drop", G_CALLBACK (library_on_colour_drop), NULL);
		gtk_widget_add_controller (widget, GTK_EVENT_CONTROLLER (drop));
		g_object_set_data (G_OBJECT (widget), "colour-drop-target", drop);
	}
}

static void
bind_colour (GtkListItemFactory* factory, GtkListItem* item, gpointer user_data)
{
	(void)factory;
	GtkWidget* child = gtk_list_item_get_child (item);
	if (!child) return;

	SampleRow* row = gtk_list_item_get_item (item);
	if (!row) return;

	library_attach_colour_drop_target (child, row);
	g_object_set_data (G_OBJECT (row), "swatch-widget", child);

	Sample* s = row->sample;
	gint colour_index = s ? s->colour_index : 0;

	library_refresh_colour_swatch (row, colour_index, child);
}

static void
library_update_overview (SampleRow* row)
{
	gtk_picture_set_pixbuf (GTK_PICTURE (row->overview), row->sample->overview);
}

static void
bind_overview (GtkListItemFactory* factory, GtkListItem* item, gpointer user_data)
{
	GtkWidget* box = gtk_list_item_get_child (item);
	if (!box) return;
	GtkWidget* pic = gtk_widget_get_first_child (box);
	if (!pic) return;

	SampleRow* row = gtk_list_item_get_item (item);
	if (!row) return;

	row->overview = pic;

	library_attach_colour_drop_target (box, row);

	library_update_overview(row);
}

static GtkOrdering
sort_row_by_name (gconstpointer a, gconstpointer b, gpointer user_data)
{
	(void)user_data;
	SampleRow *ra = SAMPLECAT_SAMPLE_ROW (a);
	SampleRow *rb = SAMPLECAT_SAMPLE_ROW (b);
	if (!ra || !rb) return GTK_ORDERING_EQUAL;

	Sample* sa = ra->sample;
	Sample* sb = rb->sample;

	const char* na = (sa && sa->name) ? sa->name : "";
	const char* nb = (sb && sb->name) ? sb->name : "";
	char* fa = g_utf8_casefold (na, -1);
	char* fb = g_utf8_casefold (nb, -1);

	int cmp = g_strcmp0 (fa, fb);

	g_free (fa);
	g_free (fb);

	return cmp < 0 ? GTK_ORDERING_SMALLER : cmp > 0 ? GTK_ORDERING_LARGER : GTK_ORDERING_EQUAL;
}

static GtkOrdering
sort_row_by_path (gconstpointer a, gconstpointer b, gpointer user_data)
{
	(void)user_data;
	SampleRow* ra = SAMPLECAT_SAMPLE_ROW (a);
	SampleRow* rb = SAMPLECAT_SAMPLE_ROW (b);
	if (!ra || !rb) return GTK_ORDERING_EQUAL;

	Sample* sa = ra->sample;
	Sample* sb = rb->sample;

	const char* pa = (sa && sa->sample_dir) ? sa->sample_dir : "";
	const char* pb = (sb && sb->sample_dir) ? sb->sample_dir : "";
	char* fa = g_utf8_casefold (pa, -1);
	char* fb = g_utf8_casefold (pb, -1);

	int cmp = g_strcmp0 (fa, fb);

	g_free (fa);
	g_free (fb);

	return cmp < 0 ? GTK_ORDERING_SMALLER : cmp > 0 ? GTK_ORDERING_LARGER : GTK_ORDERING_EQUAL;
}

static GtkOrdering
sort_row_by_length (gconstpointer a, gconstpointer b, gpointer user_data)
{
	(void)user_data;
	SampleRow* ra = SAMPLECAT_SAMPLE_ROW (a);
	SampleRow* rb = SAMPLECAT_SAMPLE_ROW (b);
	if (!ra || !rb) return GTK_ORDERING_EQUAL;

	Sample* sa = sample_row_get_sample (ra);
	Sample* sb = sample_row_get_sample (rb);

	int64_t la = sa ? sa->length : 0;
	int64_t lb = sb ? sb->length : 0;

	if (sa) sample_unref (sa);
	if (sb) sample_unref (sb);

	if (la == lb) return GTK_ORDERING_EQUAL;
	return la < lb ? GTK_ORDERING_SMALLER : GTK_ORDERING_LARGER;
}

static GtkOrdering
sort_row_by_colour (gconstpointer a, gconstpointer b, gpointer user_data)
{
	(void)user_data;
	SampleRow* ra = SAMPLECAT_SAMPLE_ROW (a);
	SampleRow* rb = SAMPLECAT_SAMPLE_ROW (b);
	if (!ra || !rb) return GTK_ORDERING_EQUAL;

	Sample* sa = sample_row_get_sample (ra);
	Sample* sb = sample_row_get_sample (rb);

	int ca = sa ? sa->colour_index : 0;
	int cb = sb ? sb->colour_index : 0;

	if (sa) sample_unref (sa);
	if (sb) sample_unref (sb);

	if (ca == cb) return GTK_ORDERING_EQUAL;
	return ca < cb ? GTK_ORDERING_SMALLER : GTK_ORDERING_LARGER;
}

static GtkOrdering
sort_row_by_tags (gconstpointer a, gconstpointer b, gpointer user_data)
{
	(void)user_data;
	SampleRow* ra = SAMPLECAT_SAMPLE_ROW (a);
	SampleRow* rb = SAMPLECAT_SAMPLE_ROW (b);
	if (!ra || !rb) return GTK_ORDERING_EQUAL;

	Sample* sa = sample_row_get_sample (ra);
	Sample* sb = sample_row_get_sample (rb);

	const char* ta = (sa && sa->keywords) ? sa->keywords : "";
	const char* tb = (sb && sb->keywords) ? sb->keywords : "";
	int cmp = g_strcmp0 (ta, tb);

	if (sa) sample_unref (sa);
	if (sb) sample_unref (sb);

	return cmp < 0 ? GTK_ORDERING_SMALLER : cmp > 0 ? GTK_ORDERING_LARGER : GTK_ORDERING_EQUAL;
}

/* Factory setup callbacks */
static void
setup_icon (GtkListItemFactory *factory, GtkListItem *item, gpointer user_data)
{
	(void)factory;
	RowWidgets* rw = row_widgets_new_icon_text ();
	g_object_set_data_full (G_OBJECT (rw->box), "rw", rw, g_free);
	gtk_list_item_set_child (item, rw->box);
}


static void
setup_name_single (GtkListItemFactory* factory, GtkListItem* item, gpointer user_data)
{
	(void)factory;
	RowWidgets* rw = row_widgets_new_single_label ();
	g_object_set_data_full (G_OBJECT (rw->box), "rw", rw, g_free);
	gtk_list_item_set_child (item, rw->box);
}

static void
setup_path_single (GtkListItemFactory* factory, GtkListItem* item, gpointer user_data)
{
	(void)factory;
	RowWidgets* rw = row_widgets_new_path_label ();
	g_object_set_data_full (G_OBJECT (rw->box), "rw", rw, g_free);
	gtk_list_item_set_child (item, rw->box);
}

static void
setup_label (GtkListItemFactory* factory, GtkListItem* item, gpointer user_data)
{
	(void)factory;
	GtkWidget* lbl = gtk_label_new (NULL);
	gtk_label_set_xalign (GTK_LABEL (lbl), 0.0);
	gtk_list_item_set_child (item, lbl);
}

static void
setup_tags (GtkListItemFactory* factory, GtkListItem* item, gpointer user_data)
{
	(void)factory;
	GtkWidget* lbl = NULL;
	lbl = gtk_editable_label_new ("");
	gtk_widget_set_halign (lbl, GTK_ALIGN_START);
	gtk_widget_set_hexpand (lbl, TRUE);
	gtk_widget_add_css_class (lbl, "tag-link");
	g_signal_connect (lbl, "notify::editing", G_CALLBACK (on_tags_notify_editing), NULL);

	GtkWidget* inner_label = editable_label_get_text_widget (lbl);
	if (inner_label) {
		g_object_set_data (G_OBJECT (lbl), "tag-label", inner_label);
	}

	/* Default to read-only; double-click to edit */
	gtk_editable_set_editable (GTK_EDITABLE (lbl), FALSE);

	GtkGesture* gesture = gtk_gesture_click_new ();
	gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (gesture), GTK_PHASE_CAPTURE);
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 0);
	g_signal_connect (gesture, "pressed", G_CALLBACK (on_tags_clicked), lbl);
	gtk_widget_add_controller (lbl, GTK_EVENT_CONTROLLER (gesture));

	GtkEventController* motion = gtk_event_controller_motion_new ();
	gtk_event_controller_set_propagation_phase (motion, GTK_PHASE_CAPTURE);
	g_signal_connect (motion, "enter", G_CALLBACK (on_tags_hover_enter), lbl);
	g_signal_connect (motion, "motion", G_CALLBACK (on_tags_hover_motion), lbl);
	g_signal_connect (motion, "leave", G_CALLBACK (on_tags_hover_leave), lbl);
	gtk_widget_add_controller (lbl, motion);

	gtk_list_item_set_child (item, lbl);
}

static void
draw_colour_swatch (GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer user_data)
{
	(void)user_data;
	GdkRGBA* rgba = g_object_get_data (G_OBJECT (area), "swatch-colour");
	if (!rgba) return;

	cairo_rectangle (cr, 0.0, 0.0, width, height);
	cairo_set_source_rgba (cr, rgba->red, rgba->green, rgba->blue, rgba->alpha);
	cairo_fill (cr);
}

static void
setup_colour (GtkListItemFactory* factory, GtkListItem* item, gpointer user_data)
{
	(void)factory;
	GtkWidget* area = gtk_drawing_area_new ();
	gtk_widget_set_size_request (area, 16, 12);
	gtk_widget_add_css_class (area, "library-colour-swatch");
	gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (area), draw_colour_swatch, NULL, NULL);
	gtk_list_item_set_child (item, area);
}



static void
setup_overview (GtkListItemFactory* factory, GtkListItem* item, gpointer user_data)
{
	(void)factory;
	GtkWidget* box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_hexpand (box, TRUE);
	gtk_widget_set_halign (box, GTK_ALIGN_FILL);

	GtkWidget* pic = gtk_picture_new ();
	gtk_widget_set_size_request (pic, OVERVIEW_WIDTH, OVERVIEW_HEIGHT);
	gtk_picture_set_can_shrink (GTK_PICTURE (pic), FALSE);
	gtk_picture_set_content_fit (GTK_PICTURE (pic), GTK_CONTENT_FIT_FILL);
	gtk_widget_set_hexpand (pic, TRUE);
	gtk_widget_set_halign (pic, GTK_ALIGN_FILL);
	gtk_widget_add_css_class (pic, "library-overview");
	gtk_box_append (GTK_BOX (box), pic);

	gtk_list_item_set_child (item, box);
}

/* GObject boilerplate */
static void library_realize (GtkWidget* widget);
static void library_dispose (GObject* object);
static void library_finalize (GObject* object);
static GObject* library_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties);

static void
library_class_init (LibraryClass* klass)
{
	library_parent_class = g_type_class_peek_parent (klass);

	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	object_class->dispose = library_dispose;
	object_class->finalize = library_finalize;
	object_class->constructor = library_constructor;

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->realize = library_realize;

	gtk_widget_class_add_binding_action (widget_class, GDK_KEY_Delete, 0, "library.delete-rows", NULL);
	gtk_widget_class_add_binding_action (widget_class, GDK_KEY_u, 0, "library.update-rows", NULL);
	gtk_widget_class_add_binding_action (widget_class, GDK_KEY_c, 0, "library.reset-colours", NULL);
}

static void
library_instance_init (Library* self)
{
	instance = self;

	self->scroll = scrolled_window_new ();
	gdl_dock_item_set_child (GDL_DOCK_ITEM (self), self->scroll);

	/* Sorted model + selection */
	self->sort_model = gtk_sort_list_model_new (G_LIST_MODEL (samplecat.store), NULL);
	self->selection = GTK_SELECTION_MODEL (gtk_multi_selection_new (G_LIST_MODEL (self->sort_model)));

	/* ColumnView */
	self->column_view = gtk_column_view_new (self->selection);
	gtk_widget_add_css_class (self->column_view, "library-view");
	gtk_sort_list_model_set_sorter (self->sort_model, gtk_column_view_get_sorter (GTK_COLUMN_VIEW (self->column_view)));
	gtk_column_view_set_show_row_separators (GTK_COLUMN_VIEW (self->column_view), FALSE);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (self->scroll), self->column_view);

	/* Factories */
	self->factory_icon = gtk_signal_list_item_factory_new ();
	g_signal_connect (self->factory_icon, "setup", G_CALLBACK (setup_icon), NULL);
	g_signal_connect (self->factory_icon, "bind", G_CALLBACK (bind_icon), NULL);

	self->factory_name = gtk_signal_list_item_factory_new ();
	g_signal_connect (self->factory_name, "setup", G_CALLBACK (setup_name_single), NULL);
	g_signal_connect (self->factory_name, "bind", G_CALLBACK (bind_name), NULL);

	self->factory_path = gtk_signal_list_item_factory_new ();
	g_signal_connect (self->factory_path, "setup", G_CALLBACK (setup_path_single), NULL);
	g_signal_connect (self->factory_path, "bind", G_CALLBACK (bind_path), NULL);

	self->factory_colour = gtk_signal_list_item_factory_new ();
	g_signal_connect (self->factory_colour, "setup", G_CALLBACK (setup_colour), NULL);
	g_signal_connect (self->factory_colour, "bind", G_CALLBACK (bind_colour), NULL);

	self->factory_tags = gtk_signal_list_item_factory_new ();
	g_signal_connect (self->factory_tags, "setup", G_CALLBACK (setup_tags), NULL);
	g_signal_connect (self->factory_tags, "bind", G_CALLBACK (bind_tags), NULL);

	self->factory_waveform = gtk_signal_list_item_factory_new ();
	g_signal_connect (self->factory_waveform, "setup", G_CALLBACK (setup_overview), NULL);
	g_signal_connect (self->factory_waveform, "bind", G_CALLBACK (bind_overview), NULL);

	self->factory_length = gtk_signal_list_item_factory_new ();
	g_signal_connect (self->factory_length, "setup", G_CALLBACK (setup_label), NULL);
	g_signal_connect (self->factory_length, "bind", G_CALLBACK (bind_length), NULL);

	/* Columns */
	GtkColumnViewColumn* col;

	col = gtk_column_view_column_new ("", self->factory_colour);
	gtk_column_view_column_set_fixed_width (col, 16);
	gtk_column_view_column_set_resizable (col, FALSE);
	{
		GtkSorter* sorter = GTK_SORTER (gtk_custom_sorter_new (sort_row_by_colour, NULL, NULL));
		gtk_column_view_column_set_sorter (col, sorter);
		g_object_unref (sorter);
	}
	gtk_column_view_append_column (GTK_COLUMN_VIEW (self->column_view), col);

	col = gtk_column_view_column_new ("", self->factory_icon);
	gtk_column_view_append_column (GTK_COLUMN_VIEW (self->column_view), col);

	col = gtk_column_view_column_new ("Name", self->factory_name);
	gtk_column_view_column_set_resizable (col, TRUE);
	{
		GtkSorter* sorter = GTK_SORTER (gtk_custom_sorter_new (sort_row_by_name, NULL, NULL));
		gtk_column_view_column_set_sorter (col, sorter);
		g_object_unref (sorter);
	}
	gtk_column_view_append_column (GTK_COLUMN_VIEW (self->column_view), col);

	col = gtk_column_view_column_new ("Path", self->factory_path);
	gtk_column_view_column_set_resizable (col, TRUE);
	gtk_column_view_column_set_expand (col, TRUE);
	{
		GtkSorter* sorter = GTK_SORTER (gtk_custom_sorter_new (sort_row_by_path, NULL, NULL));
		gtk_column_view_column_set_sorter (col, sorter);
		g_object_unref (sorter);
	}
	gtk_column_view_append_column (GTK_COLUMN_VIEW (self->column_view), col);

	col = gtk_column_view_column_new ("Tags", self->factory_tags);
	{
		GtkSorter* sorter = GTK_SORTER (gtk_custom_sorter_new (sort_row_by_tags, NULL, NULL));
		gtk_column_view_column_set_sorter (col, sorter);
		g_object_unref (sorter);
	}
	gtk_column_view_append_column (GTK_COLUMN_VIEW (self->column_view), col);

	col = gtk_column_view_column_new ("Overview", self->factory_waveform);
	gtk_column_view_column_set_fixed_width (col, OVERVIEW_WIDTH);
	gtk_column_view_column_set_resizable (col, FALSE);
	    gtk_column_view_column_set_expand (col, FALSE);
	gtk_column_view_append_column (GTK_COLUMN_VIEW (self->column_view), col);

	col = gtk_column_view_column_new ("Length", self->factory_length);
	{
		GtkSorter* sorter = GTK_SORTER (gtk_custom_sorter_new (sort_row_by_length, NULL, NULL));
		gtk_column_view_column_set_sorter (col, sorter);
		g_object_unref (sorter);
	}
	gtk_column_view_append_column (GTK_COLUMN_VIEW (self->column_view), col);

	g_signal_connect (self->selection, "selection-changed", G_CALLBACK (library_selection_changed), self);
	g_signal_connect (samplecat.store, "content-changed", G_CALLBACK (library_on_store_changed), self);

	void library_on_sample_changed (SamplecatModel* m, Sample* sample, int prop, void* val, gpointer _app)
	{
		if (prop == COL_COLOUR) {
			int* colour_index = val;
			library_refresh_colour_swatch (sample->row_ref, *colour_index, NULL);
		}
		if (prop == COL_OVERVIEW) {
			library_update_overview(sample->row_ref);
		}
	}
	g_signal_connect((gpointer)samplecat.model, "sample-changed", G_CALLBACK(library_on_sample_changed), NULL);

	if (play) {
		g_signal_connect (play, "notify::state", G_CALLBACK (library_on_player_state), self);
	}

	/* Drag source */
	GtkDragSource* drag_source = gtk_drag_source_new ();
	gtk_drag_source_set_actions (drag_source, GDK_ACTION_COPY);
	g_signal_connect (drag_source, "prepare", G_CALLBACK (library_drag_prepare), self);
	g_signal_connect (drag_source, "drag-begin", G_CALLBACK (library_on_drag_begin), self);
	gtk_widget_add_controller (self->column_view, GTK_EVENT_CONTROLLER (drag_source));

	GActionEntry entries[] = {
		{ "delete-rows", library_delete_selected, NULL, NULL, NULL },
		{ "update-rows", library_update_selected, NULL, NULL, NULL },
		{ "reset-colours", library_reset_colours, NULL, NULL, NULL },
	};
	GSimpleActionGroup* group = g_simple_action_group_new ();
	g_action_map_add_action_entries (G_ACTION_MAP (group), entries, G_N_ELEMENTS (entries), self);
	gtk_widget_insert_action_group (GTK_WIDGET (gtk_application_get_active_window (GTK_APPLICATION (app))), "library", G_ACTION_GROUP (group));
}

static GObject*
library_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (library_parent_class);
	return parent_class->constructor (type, n_construct_properties, construct_properties);
}

static void
library_realize (GtkWidget* widget)
{
	GTK_WIDGET_CLASS (library_parent_class)->realize (widget);

	/* Ensure there is a selection on first realize */
	Library* self = LIBRARY (widget);
	GtkBitset* sel = gtk_selection_model_get_selection (self->selection);
	if (g_list_model_get_n_items (G_LIST_MODEL (self->selection)) > 0 && (!sel || gtk_bitset_is_empty (sel))) {
		gtk_selection_model_select_item (self->selection, 0, TRUE);
	}
	if (sel) gtk_bitset_unref (sel);
}

static void
library_dispose (GObject* object)
{
	Library* self = LIBRARY (object);

	if (play) {
		g_signal_handlers_disconnect_by_func (play, library_on_player_state, self);
	}

	g_clear_object (&self->factory_icon);
	g_clear_object (&self->factory_name);
	g_clear_object (&self->factory_path);
	g_clear_object (&self->factory_colour);
	g_clear_object (&self->factory_tags);
	g_clear_object (&self->factory_waveform);
	g_clear_object (&self->factory_length);
	g_clear_object (&self->selection);
	g_clear_object (&self->sort_model);

	G_OBJECT_CLASS (library_parent_class)->dispose (object);
}

static void
library_finalize (GObject* object)
{
	G_OBJECT_CLASS (library_parent_class)->finalize (object);
}

static GType
library_get_type_once (void)
{
	static const GTypeInfo info = { sizeof (LibraryClass), NULL, NULL, (GClassInitFunc)library_class_init, NULL, NULL, sizeof (Library), 0, (GInstanceInitFunc)library_instance_init, NULL };
	return g_type_register_static (GDL_TYPE_DOCK_ITEM, "Library", &info, 0);
}

GType
library_get_type (void)
{
	static gsize type_id__once = 0;
	if (g_once_init_enter (&type_id__once)) {
		GType t = library_get_type_once ();
		g_once_init_leave (&type_id__once, t);
	}
	return type_id__once;
}

/* Helpers */

static Sample*
library_get_sample_at_index (Library* self, guint position)
{
	if (!self || !self->selection) return NULL;

	GListModel *model = gtk_multi_selection_get_model (GTK_MULTI_SELECTION (self->selection));
	if (!model) return NULL;

	SampleRow *row = SAMPLECAT_SAMPLE_ROW (g_list_model_get_item (model, position));
	if (!row) return NULL;

	Sample *sample = sample_row_get_sample (row);
	g_object_unref (row);
	return sample;
}

static void
library_selection_changed (GtkSelectionModel* model, guint position, guint n_items, gpointer user_data)
{
	Library* self = LIBRARY (user_data);
	(void)position;
	(void)n_items;

	GtkBitset *set = gtk_selection_model_get_selection (model);
	if (!set) return;

	GtkBitsetIter iter;
	guint idx = 0;
	if (!gtk_bitset_iter_init_first (&iter, set, &idx)) {
		gtk_bitset_unref (set);
		return;
	}
	gtk_bitset_unref (set);

	Sample *s = library_get_sample_at_index (self, idx);
	if (!s) return;

	if (s->id != self->selected_id) {
		self->selected_id = s->id;
		samplecat_model_set_selection (samplecat.model, s);
	}
	sample_unref (s);
}

static void
library_on_store_changed (SamplecatListStore* store, gpointer user_data)
{
	Library* self = LIBRARY (user_data);
	(void)store;

	/* Ensure selection exists */
	GtkBitset* sel = gtk_selection_model_get_selection (self->selection);
	if (g_list_model_get_n_items (G_LIST_MODEL (self->selection)) > 0 && sel && gtk_bitset_is_empty (sel)) {
		gtk_selection_model_select_item (self->selection, 0, TRUE);
	}
    if (sel) gtk_bitset_unref (sel);
}

static void
library_on_player_state (GObject* obj, GParamSpec* pspec, gpointer user_data)
{
	(void)obj;
	(void)pspec;
	guint n = g_list_model_get_n_items (G_LIST_MODEL (samplecat.store));
	if (n > 0) {
		g_list_model_items_changed (G_LIST_MODEL (samplecat.store), 0, n, n);
	}
}

static gboolean
library_on_colour_drop (GtkDropTarget* target, const GValue* value, double x, double y, gpointer user_data)
{
	(void)x;
	(void)y;
	(void)user_data;
	if (!value) return FALSE;
	if (!G_VALUE_HOLDS (value, AYYI_TYPE_COLOUR)) return FALSE;

	GtkWidget* widget = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (target));
	if (!widget) return FALSE;

	SampleRow* row = g_object_get_data (G_OBJECT (widget), "row");
	if (!row) return FALSE;

	gint colour_index = value->data[0].v_int;
	if (colour_index <= 0) return FALSE;

	samplecat_model_update_sample (samplecat.model, row->sample, COL_COLOUR, &colour_index);

	return TRUE;
}

/* Drag source */
static GdkContentProvider*
library_drag_prepare (GtkDragSource* source, double x, double y, gpointer user_data)
{
	Library* self = LIBRARY (user_data);
	(void)source;
	(void)x;
	(void)y;

	GtkBitset *sel = gtk_selection_model_get_selection (self->selection);
	if (!sel) return NULL;

	GSList* files = NULL;
	GtkBitsetIter iter;
	guint idx = 0;
	if (gtk_bitset_iter_init_first (&iter, sel, &idx)) {
		do {
			Sample* s = library_get_sample_at_index (self, idx);
			if (s && s->full_path) {
				files = g_slist_prepend (files, g_file_new_for_path (s->full_path));
			}
			if (s) sample_unref (s);
		} while (gtk_bitset_iter_next (&iter, &idx));
	}
	gtk_bitset_unref (sel);

	if (!files) return NULL;

	GValue value = G_VALUE_INIT;
	g_value_init (&value, GDK_TYPE_FILE_LIST);
	g_value_set_boxed (&value, files);
	g_slist_free_full (files, g_object_unref);

	return gdk_content_provider_new_for_value (&value);
}

static void
library_on_drag_begin (GtkDragSource* source, GdkDrag* drag, gpointer user_data)
{
	(void)source;
	(void)drag;
	(void)user_data;
	/* Icon could be set here if desired */
}

/* Actions */

static void
library_delete_selected (GSimpleAction* action, GVariant* v, gpointer user_data)
{
	Library *self = LIBRARY (user_data);
	(void)action; (void)v;

	GtkBitset *sel = gtk_selection_model_get_selection (self->selection);
	if (!sel) return;

	GtkBitsetIter iter;
	GPtrArray* rows = g_ptr_array_new ();
	guint idx = 0;
	if (gtk_bitset_iter_init_first (&iter, sel, &idx)) {
		do {
			g_ptr_array_add (rows, GUINT_TO_POINTER (idx));
		} while (gtk_bitset_iter_next (&iter, &idx));
	}
	gtk_bitset_unref (sel);

	/* delete from highest index to lowest to keep indices valid */
	for (gint i = (gint)rows->len - 1; i >= 0; i--) {
		guint idx = GPOINTER_TO_UINT (rows->pdata[i]);

		Sample *s = library_get_sample_at_index (self, idx);
		if (!s) continue;

		guint child_idx = samplecat_list_store_find_sample_index (s);
		if (child_idx == G_MAXUINT) {
			sample_unref (s);
			continue;
		}

		if (!samplecat_model_remove (samplecat.model, s->id)) {
			sample_unref (s);
			continue;
		}
		sample_unref (s);

		samplecat_list_store_remove_at (child_idx);
	}
	g_ptr_array_free (rows, TRUE);
	statusbar_print (1, "files deleted");
}

static void
library_update_selected (GSimpleAction* action, GVariant* v, gpointer user_data)
{
	Library* self = LIBRARY (user_data);
	(void)action;
	(void)v;

	GtkBitset *sel = gtk_selection_model_get_selection (self->selection);
	if (!sel) return;

	GtkBitsetIter iter;
	guint idx = 0;
	if (gtk_bitset_iter_init_first (&iter, sel, &idx)) {
		do {
			Sample* s = library_get_sample_at_index (self, idx);
			if (s) {
				samplecat_model_refresh_sample (samplecat.model, s, true);
				sample_unref (s);
			}
		} while (gtk_bitset_iter_next (&iter, &idx));
	}
	gtk_bitset_unref (sel);
	statusbar_print (1, "update complete");
}

static void
library_reset_colours (GSimpleAction* action, GVariant* v, gpointer user_data)
{
	Library* self = LIBRARY (user_data);
	(void)action;
	(void)v;

	GtkBitset *sel = gtk_selection_model_get_selection (self->selection);
	if (!sel) return;

	GtkBitsetIter iter;
	guint idx = 0;
	if (gtk_bitset_iter_init_first (&iter, sel, &idx)) {
		do {
			Sample* s = library_get_sample_at_index (self, idx);
			if (s) {
				samplecat_model_update_sample (samplecat.model, s, COL_COLOUR, GINT_TO_POINTER (s->colour_index));
				sample_unref (s);
			}
		} while (gtk_bitset_iter_next (&iter, &idx));
	}
	gtk_bitset_unref (sel);
	statusbar_print (1, "colours reset");
}

GList*
listview__get_selection ()
{
	if (!instance || !instance->selection) return NULL;

	GtkBitset *sel = gtk_selection_model_get_selection (instance->selection);
	if (!sel) return NULL;

	GList* samples = NULL;
	GtkBitsetIter iter;
	guint idx = 0;
	if (gtk_bitset_iter_init_first (&iter, sel, &idx)) {
		do {
			Sample* s = library_get_sample_at_index (instance, idx);
			if (s) {
				samples = g_list_append (samples, s);
			}
		} while (gtk_bitset_iter_next (&iter, &idx));
	}
	gtk_bitset_unref (sel);

	return samples;
}
