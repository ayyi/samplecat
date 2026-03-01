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
 */

#include "config.h"
#include <gio/gio.h>
#include <glib-object.h>
#include <debug/debug.h>
#include <samplecat.h>
#include <samplecat/list_store.h>
#include <samplecat/model.h>
#include <samplecat/sample_row.h>
#include <file_manager/mimetype.h>
#include <file_manager/pixmaps.h>

static void samplecat_list_store_list_model_init (GListModelInterface* iface);
static void samplecat_list_store_on_sample_modified (SamplecatModel*, Sample*, gint prop, void* val, gpointer);

G_DEFINE_TYPE_WITH_CODE (SamplecatListStore, samplecat_list_store, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, samplecat_list_store_list_model_init))

static void samplecat_list_store_clear (SamplecatListStore *self);


static GType
samplecat_list_store_get_item_type (GListModel* list)
{
	(void)list;
	return SAMPLECAT_TYPE_SAMPLE_ROW;
}

static guint
samplecat_list_store_get_n_items (GListModel* list)
{
	SamplecatListStore* self = SAMPLECAT_LIST_STORE (list);
	return self->rows ? g_list_model_get_n_items (G_LIST_MODEL (self->rows)) : 0;
}

static gpointer
samplecat_list_store_get_item (GListModel* list, guint position)
{
	SamplecatListStore* self = SAMPLECAT_LIST_STORE (list);
	if (!self->rows) return NULL;
	return g_list_model_get_item (G_LIST_MODEL (self->rows), position);
}

static void
samplecat_list_store_list_model_init (GListModelInterface* iface)
{
	iface->get_item_type = samplecat_list_store_get_item_type;
	iface->get_n_items   = samplecat_list_store_get_n_items;
	iface->get_item      = samplecat_list_store_get_item;
}

static void
samplecat_list_store_dispose (GObject* object)
{
	SamplecatListStore* self = SAMPLECAT_LIST_STORE (object);

	g_clear_object (&self->playing);
	g_clear_object (&self->rows);

	G_OBJECT_CLASS (samplecat_list_store_parent_class)->dispose (object);
}

static void
samplecat_list_store_class_init (SamplecatListStoreClass* klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	object_class->dispose = samplecat_list_store_dispose;

	g_signal_new ("content-changed", SAMPLECAT_TYPE_LIST_STORE, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
samplecat_list_store_init (SamplecatListStore* self)
{
	self->rows = g_list_store_new (SAMPLECAT_TYPE_SAMPLE_ROW);
	self->row_count = 0;
	self->playing = NULL;

	g_signal_connect((gpointer)samplecat.model, "sample-changed", G_CALLBACK(samplecat_list_store_on_sample_modified), NULL);
}

/* Public API */

SamplecatListStore*
samplecat_list_store_new (void)
{
	return g_object_new (SAMPLECAT_TYPE_LIST_STORE, NULL);
}

gint
samplecat_list_store_get_row_count ()
{
	return samplecat.store->row_count;
}

SampleRow*
samplecat_list_store_get_row (guint position)
{
	g_return_val_if_fail (samplecat.store->rows, NULL);

	if (position >= g_list_model_get_n_items (G_LIST_MODEL (samplecat.store->rows))) {
		return NULL;
	}

	return SAMPLECAT_SAMPLE_ROW (g_list_model_get_item (G_LIST_MODEL (samplecat.store->rows), position));
}

Sample*
samplecat_list_store_get_sample_by_index (guint position)
{
	SampleRow* row = samplecat_list_store_get_row (position);
	if (!row) return NULL;

	Sample* sample = sample_row_get_sample (row);
	g_object_unref (row);
	return sample;
}

static void
samplecat_list_store_clear (SamplecatListStore* self)
{
	g_return_if_fail (SAMPLECAT_IS_LIST_STORE (self));

	guint old_count = self->rows ? g_list_model_get_n_items (G_LIST_MODEL (self->rows)) : 0;

	if (self->rows) {
		g_list_store_remove_all (self->rows);
	}
	self->row_count = 0;

	g_list_model_items_changed (G_LIST_MODEL (self), 0, old_count, 0);
	g_signal_emit_by_name (self, "content-changed");
}

static void
samplecat_list_store_append_row (Sample* sample)
{
	SampleRow* row = sample_row_new (sample);
	samplecat_list_store_set_row_ref (sample, row);
	g_list_store_append (samplecat.store->rows, row);
	g_object_unref (row);
}

void
samplecat_list_store_add (Sample* sample)
{
	g_return_if_fail (sample);

	guint old_count = g_list_model_get_n_items (G_LIST_MODEL (samplecat.store->rows));
	samplecat_list_store_append_row (sample);
	samplecat.store->row_count = g_list_model_get_n_items (G_LIST_MODEL (samplecat.store->rows));

	g_list_model_items_changed (G_LIST_MODEL (samplecat.store), old_count, 0, 1);
	g_signal_emit_by_name (samplecat.store, "content-changed");
}

void
samplecat_list_store_remove_at (guint position)
{
	g_return_if_fail (samplecat.store->rows);

	if (position >= g_list_model_get_n_items (G_LIST_MODEL (samplecat.store->rows))) {
		return;
	}

	SampleRow* row = SAMPLECAT_SAMPLE_ROW (g_list_model_get_item (G_LIST_MODEL (samplecat.store->rows), position));
	if (row) {
		Sample* sample = row->sample;
		if (sample) {
			if (sample->row_ref == row) {
				g_clear_object (&sample->row_ref);
			}
		}
		g_object_unref (row);
	}

	g_list_store_remove (samplecat.store->rows, position);
	samplecat.store->row_count = g_list_model_get_n_items (G_LIST_MODEL (samplecat.store->rows));

	g_list_model_items_changed (G_LIST_MODEL (samplecat.store), position, 1, 0);
	g_signal_emit_by_name (samplecat.store, "content-changed");
}

guint
samplecat_list_store_find_sample_index (Sample* sample)
{
	guint n = g_list_model_get_n_items (G_LIST_MODEL (samplecat.store->rows));
	for (guint i = 0; i < n; i++) {
		SampleRow* row = SAMPLECAT_SAMPLE_ROW (g_list_model_get_item (G_LIST_MODEL (samplecat.store->rows), i));
		if (!row) continue;
		Sample *s = row->sample;
		bool match = (s == sample) || (s && sample && s->id == sample->id);
		g_object_unref (row);
		if (match) {
			return i;
		}
	}
	return G_MAXUINT;
}

static void
samplecat_list_store_on_sample_modified (SamplecatModel* _, Sample* sample, gint prop, void* val, gpointer __)
{
	g_return_if_fail (sample);

	guint idx = samplecat_list_store_find_sample_index (sample);
	if (idx == G_MAXUINT) {
		return;
	}

	/* Notify views that the row changed */
	/*
	 *  GListModel itself does not report changes to individual items. It only reports changes to the list membership.
	 *  Hence this signal indicates that one row was removed and added
	 */
	g_list_model_items_changed (G_LIST_MODEL (samplecat.store), idx, 1, 1);

	/* Playing colour tracking */
	if (prop == COL_ICON || prop == COL_COLOUR || prop == COL_KEYWORDS || prop == COL_OVERVIEW || prop == COL_FNAME || prop == COL_PEAKLEVEL) {
		/* already notified above; nothing else to do */
	}

	(void)val;
}

void
samplecat_list_store_set_row_ref (Sample* sample, SampleRow* row)
{
	g_return_if_fail (sample);
	g_return_if_fail (SAMPLECAT_IS_SAMPLE_ROW (row));

	g_clear_object (&sample->row_ref);
	sample->row_ref = g_object_ref (row);
}

void
samplecat_list_store_do_search ()
{
	samplecat_list_store_clear (samplecat.store);

	int n_results = 0;
	if (!samplecat.model->backend.search_iter_new (&n_results)) {
		return;
	}

	int row_count = 0;
	unsigned long* lengths;
	Sample* result;
	while ((result = samplecat.model->backend.search_iter_next (&lengths)) && row_count < LIST_STORE_MAX_ROWS) {
		Sample* s = sample_dup (result);
		samplecat_list_store_add (s);
		sample_unref (s);
		row_count++;
	}

	samplecat.model->backend.search_iter_free ();

	samplecat.store->row_count = MAX (n_results, row_count);

	/* Ensure selection exists */
	if (!samplecat.model->selection && g_list_model_get_n_items (G_LIST_MODEL (samplecat.store->rows)) > 0) {
		Sample* first = samplecat_list_store_get_sample_by_index (0);
		if (first) {
			samplecat_model_set_selection (samplecat.model, first);
			sample_unref (first);
		}
	}

	g_signal_emit_by_name (samplecat.store, "content-changed");
}
