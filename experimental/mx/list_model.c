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
#include "config.h"
#include "list_model.h"

static void sample_list_model_dispose      (GObject*);
static void sample_list_model_set_property (GObject*, guint prop_id, const GValue*, GParamSpec*);
static void sample_list_model_get_property (GObject*, guint prop_id, GValue*, GParamSpec*);
#if 0
static void set_container                  (SampleListModel*, GObject* container);
#endif

enum {
	PROP_0,
	PROP_SOURCE,
	PROP_CONTAINER,
	PROP_SEARCH
};

const gchar* sample_list_model_column_names[] = {
	"uri",
	"name",
	"length",
	"type",
	"samplerate",
	"instance"
};

GType sample_list_model_column_types[] = {
	G_TYPE_STRING,
	G_TYPE_STRING,
	G_TYPE_INT,
	G_TYPE_INT,
	G_TYPE_INT,
	G_TYPE_OBJECT
};

G_DEFINE_TYPE (SampleListModel, sample_list_model, CLUTTER_TYPE_LIST_MODEL);

typedef struct _SampleListModelPrivate
{
	GObject* source;
	GObject* container;
	gchar*   search_string;
	guint    op_id;

} SampleListModelPrivate;

#define SAMPLE_LIST_MODEL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), \
	SAMPLE_TYPE_LIST_MODEL, \
	SampleListModelPrivate))


#if 0
static void
on_source_added (Sample* sample, SampleListModel* self)
{
	sample_list_model_add_item(self, Sample*);
}

static gboolean
foreach_remove_plugin (ClutterModel* model, ClutterModelIter* iter, Sample* sample)
{
	gchar* id;

	clutter_model_iter_get (iter, SAMPLE_LIST_MODEL_COLUMN_ID, &id, -1);
	gboolean match = g_strcmp0 (id, sample->id) == 0;
	if (match) {
		clutter_model_remove (model, clutter_model_iter_get_row (iter));
	}
	g_free (id);
	return !match;
}


static void
on_source_removed (Sample* sample, SampleListModel *self)
{
	clutter_model_foreach (CLUTTER_MODEL (self), (ClutterModelForeachFunc) foreach_remove_plugin, plugin);
}
#endif


static void
sample_list_model_class_init (SampleListModelClass* klass)
{
	GObjectClass* gobject_class;
	GParamSpec* pspec;

	gobject_class = (GObjectClass*) klass;

	gobject_class->dispose = sample_list_model_dispose;
	gobject_class->set_property = sample_list_model_set_property;
	gobject_class->get_property = sample_list_model_get_property;

#if 0
	pspec = g_param_spec_object ("source",
				     "Source",
				     "Sample source model",
				     TYPE_SOURCE,
				     G_PARAM_CONSTRUCT_ONLY | G_PARAM_READABLE | G_PARAM_WRITABLE);
	g_object_class_install_property (gobject_class, PROP_SOURCE, pspec);
#endif

	pspec = g_param_spec_string ("search",
				     "Search string",
				     "String to search",
				     NULL, 
				     G_PARAM_CONSTRUCT_ONLY | G_PARAM_READABLE | G_PARAM_WRITABLE);
	g_object_class_install_property (gobject_class, PROP_SEARCH, pspec);

	g_type_class_add_private (gobject_class, sizeof(SampleListModelPrivate));
}


static void
sample_list_model_dispose (GObject *self)
{
	SampleListModelPrivate* priv = SAMPLE_LIST_MODEL_GET_PRIVATE (self);

	if (priv->op_id) {
		//media_source_cancel (priv->source, priv->op_id);
		priv->op_id = 0;
	}
	if (priv->source) {
		g_object_unref (priv->source);
		priv->source = NULL;
	}
	if (priv->source) {
/* TODO
		if (priv->container) {
			g_signal_handlers_disconnect_by_func (priv->container, G_CALLBACK (on_source_added), self);
			g_signal_handlers_disconnect_by_func (priv->container, G_CALLBACK (on_source_removed), self);
		}
		g_object_unref (priv->container);
		priv->container = NULL;
*/
	}

	G_OBJECT_CLASS (sample_list_model_parent_class)->dispose (G_OBJECT (self));
}


static void
sample_list_model_set_property (GObject* gobject, guint prop_id, const GValue* value, GParamSpec* pspec)
{
	SampleListModelPrivate* priv = SAMPLE_LIST_MODEL_GET_PRIVATE (gobject);

	switch (prop_id) {
		case PROP_SOURCE:
			priv->source = g_value_dup_object (value);
			break;
#if 0
		case PROP_CONTAINER:
			set_container (SAMPLE_LIST_MODEL (gobject), g_value_get_object (value));
			break;
#endif
		case PROP_SEARCH:
			priv->search_string = g_value_dup_string (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
			break;
	}
}


static void
sample_list_model_get_property (GObject* gobject, guint prop_id, GValue* value, GParamSpec* pspec)
{
	SampleListModelPrivate* priv = SAMPLE_LIST_MODEL_GET_PRIVATE (gobject);

	switch (prop_id) {
		case PROP_SOURCE:
			g_value_set_object (value, priv->source);
			break;
		case PROP_SEARCH:
			g_value_set_string (value, priv->search_string);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
			break;
	}
}


#if 0
static void
set_container (SampleListModel* self, GObject* container)
{
	SampleListModelPrivate* priv = SAMPLE_LIST_MODEL_GET_PRIVATE (self);

	priv->container = g_object_ref (container);
	if (container) {
		clutter_model_append (CLUTTER_MODEL (self), 
					  SAMPLE_LIST_MODEL_COLUMN_ID, get_id(),
					  SAMPLE_LIST_MODEL_COLUMN_NAME, get_name(),
					  SAMPLE_LIST_MODEL_COLUMN_TYPE, SAMPLE_LIST_MODEL_TYPE_SOURCE,
					  SAMPLE_LIST_MODEL_COLUMN_SAMPLERATE, 888,
					  SAMPLE_LIST_MODEL_COLUMN_INSTANCE, *current,
					  -1);

		g_signal_connect (priv->container, "source-added", G_CALLBACK (on_source_added), self);
		g_signal_connect (priv->container, "source-removed", G_CALLBACK (on_source_added), self);

	} else if (container) {
		if (priv->search_string) {
		}
	}
}
#endif


void
sample_list_model_add_item(SampleListModel* list_model, Sample* sample)
{
	g_return_if_fail(sample);

	clutter_model_append((ClutterModel*)list_model,
		SAMPLE_LIST_MODEL_COLUMN_ID, "42",
		SAMPLE_LIST_MODEL_COLUMN_NAME, g_strdup(sample->name),
		SAMPLE_LIST_MODEL_COLUMN_LENGTH, (int)sample->length,
		SAMPLE_LIST_MODEL_COLUMN_TYPE, SAMPLE_LIST_MODEL_TYPE_MEDIA,
		SAMPLE_LIST_MODEL_COLUMN_SAMPLERATE, sample->sample_rate,
		//SAMPLE_LIST_MODEL_COLUMN_INSTANCE, NULL,
	-1);
}


static gint
model_sort_func (ClutterModel* model, const GValue* a, const GValue* b, gpointer notused)
{
	const gchar* a_str = g_value_get_string (a);
	const gchar* b_str = g_value_get_string (b);

	return g_strcmp0 (a_str, b_str);
}


static void
sample_list_model_init (SampleListModel* self)
{
	SampleListModelPrivate* priv = SAMPLE_LIST_MODEL_GET_PRIVATE (self);

	priv->op_id = 0;
	priv->source = NULL;

	clutter_model_set_names (CLUTTER_MODEL (self), SAMPLE_LIST_MODEL_N_COLUMNS, sample_list_model_column_names);
	clutter_model_set_types (CLUTTER_MODEL (self), SAMPLE_LIST_MODEL_N_COLUMNS, sample_list_model_column_types);

	clutter_model_set_sort (CLUTTER_MODEL (self), SAMPLE_LIST_MODEL_COLUMN_NAME, model_sort_func, NULL, NULL);
}


ClutterModel*
sample_list_model_new (SamplecatModel* source_model)
{
	return g_object_new (SAMPLE_TYPE_LIST_MODEL/*, "source", source_model*/, NULL);
}


GObject*
sample_list_model_get_item (SampleListModel* self, const gchar* id)
{
	ClutterModelIter* iter = clutter_model_get_first_iter (CLUTTER_MODEL (self));

	GObject* result = NULL;
	while (!result && !clutter_model_iter_is_last (iter)) {
		gchar *iter_id;
		GObject *instance;

		clutter_model_iter_get (iter, SAMPLE_LIST_MODEL_COLUMN_ID, &iter_id, SAMPLE_LIST_MODEL_COLUMN_INSTANCE, &instance, -1);
		if (g_strcmp0 (id, iter_id) == 0) {
			result = g_object_ref (instance);
		}
		g_object_unref (instance);
		g_free (iter_id);
		clutter_model_iter_next (iter);
	}

	return result;
}


