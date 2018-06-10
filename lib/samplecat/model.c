/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include <glib.h>
#include <glib-object.h>
#include <string.h>
#include <config.h>
#include <stdlib.h>
#include <time.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gobject/gvaluecollector.h>
#include <debug/debug.h>
#include <samplecat.h>
#include <db/db.h>

#define backend samplecat.model->backend
#define _g_free0(var) (var = (g_free (var), NULL))
#define _g_list_free0(var) ((var == NULL) ? NULL : (var = (g_list_free (var), NULL)))
#define _samplecat_idle_unref0(var) ((var == NULL) ? NULL : (var = (samplecat_idle_unref (var), NULL)))
#define _sample_unref0(var) ((var == NULL) ? NULL : (var = (sample_unref (var), NULL)))

typedef struct _SamplecatParamSpecFilter SamplecatParamSpecFilter;
typedef struct _SamplecatParamSpecIdle SamplecatParamSpecIdle;
typedef struct _SamplecatSampleChange SamplecatSampleChange;
typedef struct _SamplecatModelPrivate SamplecatModelPrivate;

struct _SamplecatIdlePrivate {
	guint id;
	GSourceFunc fn;
	gpointer fn_target;
	GDestroyNotify fn_target_destroy_notify;
};

struct _SamplecatSampleChange {
	Sample* sample;
	gint prop;
	void* val;
};

struct _SamplecatModelPrivate {
	SamplecatIdle* dir_idle;
	SamplecatIdle* sample_changed_idle;
	guint          selection_change_timeout;
};

static gpointer          samplecat_idle_ref        (gpointer instance);
static void              samplecat_idle_unref      (gpointer instance);
static GType             samplecat_idle_get_type   (void) G_GNUC_CONST;
static SamplecatIdle*    samplecat_idle_new        (GSourceFunc _fn, void* _fn_target);
static SamplecatIdle*    samplecat_idle_construct  (GType object_type, GSourceFunc _fn, void* _fn_target);
static void              samplecat_idle_queue      (SamplecatIdle* self);
#if 0
static GParamSpec*       samplecat_param_spec_idle (const gchar* name, const gchar* nick, const gchar* blurb, GType object_type, GParamFlags flags);
static void              samplecat_value_set_idle  (GValue* value, gpointer v_object);
static void              samplecat_value_take_idle (GValue* value, gpointer v_object);
static gpointer          samplecat_value_get_idle  (const GValue* value);
#endif


static gpointer samplecat_filter_parent_class = NULL;
static gpointer samplecat_idle_parent_class = NULL;
static gpointer samplecat_model_parent_class = NULL;
static gchar    samplecat_model_unk[32];
static gchar    samplecat_model_unk[32] = {0};

enum  {
	SAMPLECAT_FILTER_DUMMY_PROPERTY
};
static void     samplecat_filter_finalize (SamplecatFilter* obj);
#define SAMPLECAT_IDLE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SAMPLECAT_TYPE_IDLE, SamplecatIdlePrivate))
enum  {
	SAMPLECAT_IDLE_DUMMY_PROPERTY
};
static void     samplecat_idle_finalize   (SamplecatIdle* obj);
#define SAMPLECAT_MODEL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SAMPLECAT_TYPE_MODEL, SamplecatModelPrivate))
enum  {
	SAMPLECAT_MODEL_DUMMY_PROPERTY
};
static gboolean samplecat_model_queue_selection_changed (SamplecatModel*);
static gboolean _samplecat_model_queue_selection_changed_gsource_func (gpointer self);
static void     g_cclosure_user_marshal_VOID__POINTER_INT_POINTER (GClosure*, GValue* return_value, guint n_param_values, const GValue* param_values, gpointer invocation_hint, gpointer marshal_data);
static GObject* samplecat_model_constructor (GType type, guint n_construct_properties, GObjectConstructParam*);
static void     samplecat_model_finalize (GObject*);


SamplecatFilter*
samplecat_filter_construct (GType object_type, gchar* _name)
{
	SamplecatFilter* self = (SamplecatFilter*)g_type_create_instance(object_type);
	self->name = _name;
	return self;
}


SamplecatFilter*
samplecat_filter_new (gchar* _name)
{
	return samplecat_filter_construct (SAMPLECAT_TYPE_FILTER, _name);
}


void
samplecat_filter_set_value (SamplecatFilter* self, gchar* val)
{
	g_return_if_fail (self);
	_g_free0 (self->value);
	self->value = g_strdup ((const gchar*)val);
	g_signal_emit_by_name (self, "changed");
}


static void
samplecat_value_filter_init (GValue* value)
{
	value->data[0].v_pointer = NULL;
}


static void
samplecat_value_filter_free_value (GValue* value)
{
	if (value->data[0].v_pointer) {
		samplecat_filter_unref (value->data[0].v_pointer);
	}
}


static void
samplecat_value_filter_copy_value (const GValue* src_value, GValue* dest_value)
{
	if (src_value->data[0].v_pointer) {
		dest_value->data[0].v_pointer = samplecat_filter_ref (src_value->data[0].v_pointer);
	} else {
		dest_value->data[0].v_pointer = NULL;
	}
}


static gpointer
samplecat_value_filter_peek_pointer (const GValue* value)
{
	return value->data[0].v_pointer;
}


static gchar*
samplecat_value_filter_collect_value (GValue* value, guint n_collect_values, GTypeCValue* collect_values, guint collect_flags)
{
	if (collect_values[0].v_pointer) {
		SamplecatFilter* object;
		object = collect_values[0].v_pointer;
		if (object->parent_instance.g_class == NULL) {
			return g_strconcat ("invalid unclassed object pointer for value type `", G_VALUE_TYPE_NAME (value), "'", NULL);
		} else if (!g_value_type_compatible (G_TYPE_FROM_INSTANCE (object), G_VALUE_TYPE (value))) {
			return g_strconcat ("invalid object type `", g_type_name (G_TYPE_FROM_INSTANCE (object)), "' for value type `", G_VALUE_TYPE_NAME (value), "'", NULL);
		}
		value->data[0].v_pointer = samplecat_filter_ref (object);
	} else {
		value->data[0].v_pointer = NULL;
	}
	return NULL;
}


static gchar*
samplecat_value_filter_lcopy_value (const GValue* value, guint n_collect_values, GTypeCValue* collect_values, guint collect_flags)
{
	SamplecatFilter** object_p;
	object_p = collect_values[0].v_pointer;
	if (!object_p) {
		return g_strdup_printf ("value location for `%s' passed as NULL", G_VALUE_TYPE_NAME (value));
	}
	if (!value->data[0].v_pointer) {
		*object_p = NULL;
	} else if (collect_flags & G_VALUE_NOCOPY_CONTENTS) {
		*object_p = value->data[0].v_pointer;
	} else {
		*object_p = samplecat_filter_ref (value->data[0].v_pointer);
	}
	return NULL;
}


GParamSpec*
samplecat_param_spec_filter (const gchar* name, const gchar* nick, const gchar* blurb, GType object_type, GParamFlags flags)
{
	SamplecatParamSpecFilter* spec;
	g_return_val_if_fail (g_type_is_a (object_type, SAMPLECAT_TYPE_FILTER), NULL);
	spec = g_param_spec_internal (G_TYPE_PARAM_OBJECT, name, nick, blurb, flags);
	G_PARAM_SPEC (spec)->value_type = object_type;
	return G_PARAM_SPEC (spec);
}


gpointer
samplecat_value_get_filter (const GValue* value)
{
	g_return_val_if_fail (G_TYPE_CHECK_VALUE_TYPE (value, SAMPLECAT_TYPE_FILTER), NULL);
	return value->data[0].v_pointer;
}


void
samplecat_value_set_filter (GValue* value, gpointer v_object)
{
	SamplecatFilter* old;
	g_return_if_fail (G_TYPE_CHECK_VALUE_TYPE (value, SAMPLECAT_TYPE_FILTER));
	old = value->data[0].v_pointer;
	if (v_object) {
		g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (v_object, SAMPLECAT_TYPE_FILTER));
		g_return_if_fail (g_value_type_compatible (G_TYPE_FROM_INSTANCE (v_object), G_VALUE_TYPE (value)));
		value->data[0].v_pointer = v_object;
		samplecat_filter_ref (value->data[0].v_pointer);
	} else {
		value->data[0].v_pointer = NULL;
	}
	if (old) {
		samplecat_filter_unref (old);
	}
}


void
samplecat_value_take_filter (GValue* value, gpointer v_object)
{
	SamplecatFilter* old;
	g_return_if_fail (G_TYPE_CHECK_VALUE_TYPE (value, SAMPLECAT_TYPE_FILTER));
	old = value->data[0].v_pointer;
	if (v_object) {
		g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (v_object, SAMPLECAT_TYPE_FILTER));
		g_return_if_fail (g_value_type_compatible (G_TYPE_FROM_INSTANCE (v_object), G_VALUE_TYPE (value)));
		value->data[0].v_pointer = v_object;
	} else {
		value->data[0].v_pointer = NULL;
	}
	if (old) {
		samplecat_filter_unref (old);
	}
}


static void
samplecat_filter_class_init (SamplecatFilterClass* klass)
{
	samplecat_filter_parent_class = g_type_class_peek_parent (klass);
	SAMPLECAT_FILTER_CLASS (klass)->finalize = samplecat_filter_finalize;
	g_signal_new ("changed", SAMPLECAT_TYPE_FILTER, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}


static void
samplecat_filter_instance_init (SamplecatFilter * self)
{
	self->ref_count = 1;
}


static void
samplecat_filter_finalize (SamplecatFilter* obj)
{
	SamplecatFilter * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (obj, SAMPLECAT_TYPE_FILTER, SamplecatFilter);
	_g_free0 (self->value);
}


GType
samplecat_filter_get_type (void)
{
	static volatile gsize samplecat_filter_type_id__volatile = 0;
	if (g_once_init_enter (&samplecat_filter_type_id__volatile)) {
		static const GTypeValueTable g_define_type_value_table = { samplecat_value_filter_init, samplecat_value_filter_free_value, samplecat_value_filter_copy_value, samplecat_value_filter_peek_pointer, "p", samplecat_value_filter_collect_value, "p", samplecat_value_filter_lcopy_value };
		static const GTypeInfo g_define_type_info = { sizeof (SamplecatFilterClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) samplecat_filter_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (SamplecatFilter), 0, (GInstanceInitFunc) samplecat_filter_instance_init, &g_define_type_value_table };
		static const GTypeFundamentalInfo g_define_type_fundamental_info = { (G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_INSTANTIATABLE | G_TYPE_FLAG_DERIVABLE | G_TYPE_FLAG_DEEP_DERIVABLE) };
		GType samplecat_filter_type_id;
		samplecat_filter_type_id = g_type_register_fundamental (g_type_fundamental_next (), "SamplecatFilter", &g_define_type_info, &g_define_type_fundamental_info, 0);
		g_once_init_leave (&samplecat_filter_type_id__volatile, samplecat_filter_type_id);
	}
	return samplecat_filter_type_id__volatile;
}


gpointer
samplecat_filter_ref (gpointer instance)
{
	SamplecatFilter* self;
	self = instance;
	g_atomic_int_inc (&self->ref_count);
	return instance;
}


void
samplecat_filter_unref (gpointer instance)
{
	SamplecatFilter* self;
	self = instance;
	if (g_atomic_int_dec_and_test (&self->ref_count)) {
		SAMPLECAT_FILTER_GET_CLASS (self)->finalize (self);
		g_type_free_instance ((GTypeInstance *) self);
	}
}


SamplecatFilters*
samplecat_filters_dup (const SamplecatFilters* self)
{
	SamplecatFilters* dup;
	dup = g_new0 (SamplecatFilters, 1);
	memcpy (dup, self, sizeof (SamplecatFilters));
	return dup;
}


void
samplecat_filters_free (SamplecatFilters* self)
{
	g_free (self);
}


GType
samplecat_filters_get_type (void)
{
	static volatile gsize samplecat_filters_type_id__volatile = 0;
	if (g_once_init_enter (&samplecat_filters_type_id__volatile)) {
		GType samplecat_filters_type_id;
		samplecat_filters_type_id = g_boxed_type_register_static ("SamplecatFilters", (GBoxedCopyFunc) samplecat_filters_dup, (GBoxedFreeFunc) samplecat_filters_free);
		g_once_init_leave (&samplecat_filters_type_id__volatile, samplecat_filters_type_id);
	}
	return samplecat_filters_type_id__volatile;
}


static SamplecatIdle*
samplecat_idle_construct (GType object_type, GSourceFunc _fn, void* _fn_target)
{
	SamplecatIdle* self = (SamplecatIdle*) g_type_create_instance (object_type);

	(self->priv->fn_target_destroy_notify == NULL) ? NULL : (self->priv->fn_target_destroy_notify (self->priv->fn_target), NULL);
	self->priv->fn = _fn;
	self->priv->fn_target = _fn_target;
	self->priv->fn_target_destroy_notify = NULL;
	return self;
}


static SamplecatIdle*
samplecat_idle_new (GSourceFunc _fn, void* _fn_target)
{
	return samplecat_idle_construct (SAMPLECAT_TYPE_IDLE, _fn, _fn_target);
}


static void
samplecat_idle_queue (SamplecatIdle* idle)
{
	g_return_if_fail (idle);

	gboolean ____lambda_func (gpointer self)
	{
		SamplecatIdle* idle = self;

		idle->priv->fn (idle->priv->fn_target);
		idle->priv->id = 0;

		return G_SOURCE_REMOVE;
	}

	if (!idle->priv->id) {
		idle->priv->id = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, ____lambda_func, samplecat_idle_ref(idle), samplecat_idle_unref);
	}
}


static void
samplecat_value_idle_init (GValue* value)
{
	value->data[0].v_pointer = NULL;
}


static void
samplecat_value_idle_free_value (GValue* value)
{
	if (value->data[0].v_pointer) {
		samplecat_idle_unref (value->data[0].v_pointer);
	}
}


static void
samplecat_value_idle_copy_value (const GValue* src_value, GValue* dest_value)
{
	if (src_value->data[0].v_pointer) {
		dest_value->data[0].v_pointer = samplecat_idle_ref (src_value->data[0].v_pointer);
	} else {
		dest_value->data[0].v_pointer = NULL;
	}
}


static gpointer
samplecat_value_idle_peek_pointer (const GValue* value)
{
	return value->data[0].v_pointer;
}


static gchar*
samplecat_value_idle_collect_value (GValue* value, guint n_collect_values, GTypeCValue* collect_values, guint collect_flags)
{
	if (collect_values[0].v_pointer) {
		SamplecatIdle* object;
		object = collect_values[0].v_pointer;
		if (object->parent_instance.g_class == NULL) {
			return g_strconcat ("invalid unclassed object pointer for value type `", G_VALUE_TYPE_NAME (value), "'", NULL);
		} else if (!g_value_type_compatible (G_TYPE_FROM_INSTANCE (object), G_VALUE_TYPE (value))) {
			return g_strconcat ("invalid object type `", g_type_name (G_TYPE_FROM_INSTANCE (object)), "' for value type `", G_VALUE_TYPE_NAME (value), "'", NULL);
		}
		value->data[0].v_pointer = samplecat_idle_ref (object);
	} else {
		value->data[0].v_pointer = NULL;
	}
	return NULL;
}


static gchar*
samplecat_value_idle_lcopy_value (const GValue* value, guint n_collect_values, GTypeCValue* collect_values, guint collect_flags)
{
	SamplecatIdle** object_p;
	object_p = collect_values[0].v_pointer;
	if (!object_p) {
		return g_strdup_printf ("value location for `%s' passed as NULL", G_VALUE_TYPE_NAME (value));
	}
	if (!value->data[0].v_pointer) {
		*object_p = NULL;
	} else if (collect_flags & G_VALUE_NOCOPY_CONTENTS) {
		*object_p = value->data[0].v_pointer;
	} else {
		*object_p = samplecat_idle_ref (value->data[0].v_pointer);
	}
	return NULL;
}


#if 0
static GParamSpec*
samplecat_param_spec_idle (const gchar* name, const gchar* nick, const gchar* blurb, GType object_type, GParamFlags flags)
{
	SamplecatParamSpecIdle* spec;
	g_return_val_if_fail (g_type_is_a (object_type, SAMPLECAT_TYPE_IDLE), NULL);
	spec = g_param_spec_internal (G_TYPE_PARAM_OBJECT, name, nick, blurb, flags);
	G_PARAM_SPEC (spec)->value_type = object_type;
	return G_PARAM_SPEC (spec);
}
#endif


#if 0
static gpointer
samplecat_value_get_idle (const GValue* value)
{
	g_return_val_if_fail (G_TYPE_CHECK_VALUE_TYPE (value, SAMPLECAT_TYPE_IDLE), NULL);
	return value->data[0].v_pointer;
}
#endif


#if 0
static void
samplecat_value_set_idle (GValue* value, gpointer v_object)
{
	SamplecatIdle* old;
	g_return_if_fail (G_TYPE_CHECK_VALUE_TYPE (value, SAMPLECAT_TYPE_IDLE));
	old = value->data[0].v_pointer;
	if (v_object) {
		g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (v_object, SAMPLECAT_TYPE_IDLE));
		g_return_if_fail (g_value_type_compatible (G_TYPE_FROM_INSTANCE (v_object), G_VALUE_TYPE (value)));
		value->data[0].v_pointer = v_object;
		samplecat_idle_ref (value->data[0].v_pointer);
	} else {
		value->data[0].v_pointer = NULL;
	}
	if (old) {
		samplecat_idle_unref (old);
	}
}
#endif


#if 0
static void
samplecat_value_take_idle (GValue* value, gpointer v_object)
{
	SamplecatIdle* old;
	g_return_if_fail (G_TYPE_CHECK_VALUE_TYPE (value, SAMPLECAT_TYPE_IDLE));
	old = value->data[0].v_pointer;
	if (v_object) {
		g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (v_object, SAMPLECAT_TYPE_IDLE));
		g_return_if_fail (g_value_type_compatible (G_TYPE_FROM_INSTANCE (v_object), G_VALUE_TYPE (value)));
		value->data[0].v_pointer = v_object;
	} else {
		value->data[0].v_pointer = NULL;
	}
	if (old) {
		samplecat_idle_unref (old);
	}
}
#endif


static void
samplecat_idle_class_init (SamplecatIdleClass * klass)
{
	samplecat_idle_parent_class = g_type_class_peek_parent (klass);
	SAMPLECAT_IDLE_CLASS (klass)->finalize = samplecat_idle_finalize;
	g_type_class_add_private (klass, sizeof (SamplecatIdlePrivate));
}


static void
samplecat_idle_instance_init (SamplecatIdle * self)
{
	self->priv = SAMPLECAT_IDLE_GET_PRIVATE (self);
	self->priv->id = (guint) 0;
	self->ref_count = 1;
}


static void
samplecat_idle_finalize (SamplecatIdle* obj)
{
	SamplecatIdle * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (obj, SAMPLECAT_TYPE_IDLE, SamplecatIdle);
	(self->priv->fn_target_destroy_notify == NULL) ? NULL : (self->priv->fn_target_destroy_notify (self->priv->fn_target), NULL);
	self->priv->fn = NULL;
	self->priv->fn_target = NULL;
	self->priv->fn_target_destroy_notify = NULL;
}


static GType
samplecat_idle_get_type (void)
{
	static volatile gsize samplecat_idle_type_id__volatile = 0;
	if (g_once_init_enter (&samplecat_idle_type_id__volatile)) {
		static const GTypeValueTable g_define_type_value_table = { samplecat_value_idle_init, samplecat_value_idle_free_value, samplecat_value_idle_copy_value, samplecat_value_idle_peek_pointer, "p", samplecat_value_idle_collect_value, "p", samplecat_value_idle_lcopy_value };
		static const GTypeInfo g_define_type_info = { sizeof (SamplecatIdleClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) samplecat_idle_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (SamplecatIdle), 0, (GInstanceInitFunc) samplecat_idle_instance_init, &g_define_type_value_table };
		static const GTypeFundamentalInfo g_define_type_fundamental_info = { (G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_INSTANTIATABLE | G_TYPE_FLAG_DERIVABLE | G_TYPE_FLAG_DEEP_DERIVABLE) };
		GType samplecat_idle_type_id;
		samplecat_idle_type_id = g_type_register_fundamental (g_type_fundamental_next (), "SamplecatIdle", &g_define_type_info, &g_define_type_fundamental_info, 0);
		g_once_init_leave (&samplecat_idle_type_id__volatile, samplecat_idle_type_id);
	}
	return samplecat_idle_type_id__volatile;
}


static gpointer
samplecat_idle_ref (gpointer instance)
{
	SamplecatIdle* self = instance;
	g_atomic_int_inc (&self->ref_count);
	return instance;
}


static void
samplecat_idle_unref (gpointer instance)
{
	SamplecatIdle* self = instance;
	if (g_atomic_int_dec_and_test (&self->ref_count)) {
		SAMPLECAT_IDLE_GET_CLASS (self)->finalize (self);
		g_type_free_instance ((GTypeInstance *) self);
	}
}


	static gboolean ___lambda__gsource_func (gpointer self)
	{
		dir_list_update();
		g_signal_emit_by_name ((SamplecatModel*)self, "dir-list-changed");
		return G_SOURCE_REMOVE;
	}

	static gboolean __lambda (gpointer _self)
	{
		SamplecatModel* self = _self;

		GList* l = self->modified;
		for (;l;l=l->next) {
			SamplecatSampleChange* change = l->data;
			g_signal_emit_by_name (self, "sample-changed", change->sample, change->prop, change->val);
			sample_unref(change->sample);
			g_free (change);
		}
		_g_list_free0 (self->modified);

		return FALSE;
	}

SamplecatModel*
samplecat_model_construct (GType object_type)
{
	SamplecatModel* self = (SamplecatModel*) g_object_new (object_type, NULL);
	self->state = 1;
	self->cache_dir = g_build_filename (g_get_home_dir(), ".config", PACKAGE, "cache", NULL, NULL);


	_samplecat_idle_unref0 (self->priv->dir_idle);
	self->priv->dir_idle = samplecat_idle_new (___lambda__gsource_func, self);
#if 0 // updating the directory list is not currently done until a consumer needs it
	samplecat_idle_queue (self->priv->dir_idle);
#endif

	self->priv->sample_changed_idle = samplecat_idle_new(__lambda, self);

	return self;
}


SamplecatModel*
samplecat_model_new (void)
{
	return samplecat_model_construct (SAMPLECAT_TYPE_MODEL);
}


gboolean
samplecat_model_add (SamplecatModel* self)
{
	g_return_val_if_fail (self, FALSE);

	samplecat_idle_queue (self->priv->dir_idle);

	return TRUE;
}


gboolean
samplecat_model_remove (SamplecatModel* self, gint id)
{
	g_return_val_if_fail(self, FALSE);

	samplecat_idle_queue(self->priv->dir_idle);

	return backend.remove(id);
}


void
samplecat_model_set_search_dir (SamplecatModel* self, gchar* dir)
{
	SamplecatFilters _tmp0_;
	SamplecatFilter* _tmp1_;
	gchar* _tmp2_;
	g_return_if_fail (self != NULL);
	_tmp0_ = self->filters;
	_tmp1_ = _tmp0_.dir;
	_tmp2_ = dir;
	samplecat_filter_set_value (_tmp1_, _tmp2_);
}


static gboolean
_samplecat_model_queue_selection_changed_gsource_func (gpointer self)
{
	return samplecat_model_queue_selection_changed (self);
}


void
samplecat_model_set_selection (SamplecatModel* self, Sample* sample)
{
	g_return_if_fail (self);

	if (sample != self->selection) {
		if (self->selection) {
			sample_unref (self->selection);
		}
		self->selection = sample;
		sample_ref (self->selection);
		if (self->priv->selection_change_timeout) {
			g_source_remove (self->priv->selection_change_timeout);
		}
		self->priv->selection_change_timeout = g_timeout_add_full (G_PRIORITY_DEFAULT, (guint) 250, _samplecat_model_queue_selection_changed_gsource_func, g_object_ref (self), g_object_unref);
	}
}


static gboolean
samplecat_model_queue_selection_changed (SamplecatModel* self)
{
	g_return_val_if_fail (self != NULL, FALSE);

	self->priv->selection_change_timeout = 0;
	g_signal_emit_by_name (self, "selection-changed", self->selection);
	return FALSE;
}


static void
samplecat_model_queue_sample_changed (SamplecatModel* self, Sample* sample, gint prop, void* val)
{
	g_return_if_fail (self);

	GList* l = self->modified;
	for (;l;l=l->next) {
		SamplecatSampleChange* change = l->data;
		if (change->sample == sample) {
			change->prop = COL_ALL;
			change->val = NULL;
			return;
		}
	}

	SamplecatSampleChange* c = g_new(SamplecatSampleChange, 1);
	*c = (SamplecatSampleChange){
		.sample = sample_ref(sample),
		.prop = prop,
		.val = val,
	};
	self->modified = g_list_append (self->modified, c);

	samplecat_idle_queue (self->priv->sample_changed_idle);
}


void
samplecat_model_add_filter (SamplecatModel* self, SamplecatFilter* filter)
{
	g_return_if_fail (self);

	self->filters_ = g_list_prepend (self->filters_, filter);
}


void
samplecat_model_refresh_sample (SamplecatModel* self, Sample* sample, gboolean force_update)
{
	g_return_if_fail (self);

	gboolean online = sample->online;
	time_t mtime = file_mtime (sample->full_path);
	if (mtime > ((time_t) 0)) {
		if (force_update || sample->mtime < mtime) {
			Sample* test = sample_new_from_filename (sample->full_path, FALSE);
			if (!(test)) {
				_sample_unref0 (test);
				return;
			}
			if (sample_get_file_info (sample)) {
				sample->mtime = mtime;
				g_signal_emit_by_name (self, "sample-changed", sample, COL_ALL, NULL);
				request_peaklevel (sample);
				request_overview (sample);
				request_ebur128 (sample);
			} else {
				dbg (0, "full update - reading file info failed!");
			}
			_sample_unref0 (test);
		}
		sample->mtime = mtime;
		sample->online = TRUE;
	} else {
		sample->online = false;
	}

	if (sample->online != online) {
		samplecat_model_update_sample (self, sample, (gint) COL_ICON, NULL);
	}
}


gboolean
samplecat_model_update_sample (SamplecatModel* self, Sample* sample, gint prop, void* val)
{
	g_return_val_if_fail (self, FALSE);

	gboolean ok = FALSE;

	switch (prop) {
		case COL_ICON:
		{
			if (backend.update_int (sample->id, "online", (guint) sample->online ? 1 : 0)){
				ok = backend.update_int (sample->id, "mtime", (guint) sample->mtime);
			}
			break;
		}
		case COL_KEYWORDS:
		{
			if((ok = backend.update_string (sample->id, "keywords", (gchar*)val))){
				_g_free0 (sample->keywords);
				val = sample->keywords = g_strdup ((const gchar*)val);
			}
			break;
		}
		case COL_OVERVIEW:
		{
			if (sample->overview) {
				guint len = 0U;
				guint8* blob = pixbuf_to_blob (sample->overview, &len);
				ok = backend.update_blob (sample->id, "pixbuf", blob, len);
			}
			break;
		}
		case COL_COLOUR:
		{
			guint colour_index = *((guint*)val);
			if ((ok = backend.update_int (sample->id, "colour", colour_index))){
				sample->colour_index = (gint) colour_index;
			}
			break;
		}
		case COL_PEAKLEVEL:
		{
			ok = backend.update_float (sample->id, "peaklevel", sample->peaklevel);
			break;
		}
		case COL_X_NOTES:
		{
			Sample* _tmp46_;
			gint _tmp47_;
			void* _tmp48_;
			gboolean _tmp49_ = FALSE;
			gboolean _tmp50_;
			_tmp46_ = sample;
			_tmp47_ = _tmp46_->id;
			_tmp48_ = val;
			_tmp49_ = backend.update_string (_tmp47_, "notes", (gchar*) _tmp48_);
			ok = _tmp49_;
			_tmp50_ = ok;
			if (_tmp50_) {
				Sample* _tmp51_;
				void* _tmp52_;
				gchar* _tmp53_;
				_tmp51_ = sample;
				_tmp52_ = val;
				_tmp53_ = g_strdup ((const gchar*) _tmp52_);
				_g_free0 (_tmp51_->notes);
				_tmp51_->notes = _tmp53_;
			}
			break;
		}
		case COL_X_EBUR:
		{
			ok = backend.update_string (sample->id, "ebur", sample->ebur);
			break;
		}
		case COL_ALL:
		{
			gboolean _tmp61_;
			Sample* _tmp62_;
			gint _tmp63_;
			Sample* _tmp64_;
			guint _tmp65_;
			gboolean _tmp66_ = FALSE;
			gboolean _tmp67_;
			Sample* _tmp68_;
			gint _tmp69_;
			Sample* _tmp70_;
			guint _tmp71_;
			gboolean _tmp72_ = FALSE;
			gboolean _tmp73_;
			Sample* _tmp74_;
			gint _tmp75_;
			gboolean _tmp78_ = FALSE;
			gboolean _tmp79_;
			gboolean _tmp84_ = FALSE;
			gboolean _tmp97_;
			gboolean _tmp101_ = FALSE;
			gchar* metadata = sample_get_metadata_str (sample);
			ok = TRUE;
			_tmp61_ = ok;
			_tmp62_ = sample;
			_tmp63_ = _tmp62_->id;
			_tmp64_ = sample;
			_tmp65_ = _tmp64_->channels;
			_tmp66_ = backend.update_int (_tmp63_, "channels", _tmp65_);
			ok = _tmp61_ & _tmp66_;
			_tmp67_ = ok;
			_tmp68_ = sample;
			_tmp69_ = _tmp68_->id;
			_tmp70_ = sample;
			_tmp71_ = _tmp70_->sample_rate;
			_tmp72_ = backend.update_int (_tmp69_, "sample_rate", _tmp71_);
			ok = _tmp67_ & _tmp72_;
			_tmp73_ = ok;
			_tmp74_ = sample;
			_tmp75_ = _tmp74_->id;
			_tmp78_ = backend.update_int (_tmp75_, "length", (guint) sample->length);
			ok = _tmp73_ & _tmp78_;
			_tmp79_ = ok;
			_tmp84_ = backend.update_int (sample->id, "frames", (guint) sample->frames);
			ok = _tmp79_ & _tmp84_;

			ok &= backend.update_int   (sample->id, "bit_rate", (guint) sample->bit_rate);
			ok &= backend.update_int   (sample->id, "bit_depth", (guint) sample->bit_depth);

			if(sample->peaklevel) ok &= backend.update_float (sample->id, "peaklevel", sample->peaklevel);

			if(sample->ebur && sample->ebur[0]) ok = backend.update_string (sample->id, "ebur", sample->ebur);

			if (sample->overview) {
				guint len = 0U;
				guint8* blob = pixbuf_to_blob (sample->overview, &len);
				ok = backend.update_blob (sample->id, "pixbuf", blob, len);
			}
			_tmp97_ = ok;
			_tmp101_ = backend.update_string (sample->id, "meta_data", metadata);
			ok = _tmp97_ & _tmp101_;
			if (metadata) {
				g_free(metadata);
			}
			break;
		}
		default:
		{
			warnprintf2 ("model.update_sample", "unhandled property", NULL);
			break;
		}
	}

	if (ok) {
		samplecat_model_queue_sample_changed (self, sample, prop, val);
	} else {
		warnprintf2 ("model.update_sample", "database update failed for %s", samplecat_model_print_col_name ((guint)prop), NULL);
	}
	return ok;
}


gchar*
samplecat_model_print_col_name (guint prop_type)
{
	gchar* result = NULL;
	guint _tmp0_;
	_tmp0_ = prop_type;
	switch (_tmp0_) {
		case COL_ICON:
		{
			result = "ICON";
			return result;
		}
		case COL_IDX:
		{
			result = "IDX";
			return result;
		}
		case COL_NAME:
		{
			result = "NAME";
			return result;
		}
		case COL_FNAME:
		{
			result = "FILENAME";
			return result;
		}
		case COL_KEYWORDS:
		{
			result = "KEYWORDS";
			return result;
		}
		case COL_OVERVIEW:
		{
			result = "OVERVIEW";
			return result;
		}
		case COL_LENGTH:
		{
			result = "LENGTH";
			return result;
		}
		case COL_SAMPLERATE:
		{
			result = "SAMPLERATE";
			return result;
		}
		case COL_CHANNELS:
		{
			result = "CHANNELS";
			return result;
		}
		case COL_MIMETYPE:
		{
			result = "MIMETYPE";
			return result;
		}
		case COL_PEAKLEVEL:
		{
			result = "PEAKLEVEL";
			return result;
		}
		case COL_COLOUR:
		{
			result = "COLOUR";
			return result;
		}
		case COL_SAMPLEPTR:
		{
			result = "SAMPLEPTR";
			return result;
		}
		case COL_X_EBUR:
		{
			result = "X_EBUR";
			return result;
		}
		case COL_X_NOTES:
		{
			result = "X_NOTES";
			return result;
		}
		case COL_ALL:
		{
			return result = "ALL";
		}
		default:
		{
			gchar* _tmp2_ = NULL;
			gchar* a;
			const gchar* _tmp3_;
			_tmp2_ = g_strdup_printf ("UNKNOWN PROPERTY (%u)", prop_type);
			a = _tmp2_;
			_tmp3_ = a;
			memcpy (samplecat_model_unk, _tmp3_, (gsize) 31);
			_g_free0 (a);
			break;
		}
	}
	result = samplecat_model_unk;
	return result;
}


void
samplecat_model_move_files(GList* list, const gchar* dest_path)
{
	/* This is called _before_ the files are actually moved!
	 * file_util_move_simple() which is called afterwards 
	 * will free the list and dest_path.
	 */
	if (!list) return;
	if (!dest_path) return;

	GList* l = list;
	do {
		const gchar* src_path=l->data;

		gchar* bn = g_path_get_basename(src_path);
		gchar* full_path = g_strdup_printf("%s%c%s", dest_path, G_DIR_SEPARATOR, bn);
		g_free(bn);

		if (src_path) {
			dbg(0,"move %s -> %s | NEW: %s", src_path, dest_path, full_path);
			int id = -1;
			if(backend.file_exists(src_path, &id) && id > 0){
				backend.update_string(id, "filedir", dest_path);
				backend.update_string(id, "full_path", full_path);
			}

			Sample* s = sample_get_by_filename(src_path);
			if (s) {
				g_free(s->full_path);
				g_free(s->sample_dir);
				s->full_path  = strdup(full_path);
				s->sample_dir = strdup(dest_path);
				g_signal_emit_by_name(samplecat.model, "sample-changed", s, COL_FNAME, NULL);
			}
		}
		g_free(full_path);
		l = l->next;
	} while (l);
}


static void
g_cclosure_user_marshal_VOID__POINTER_INT_POINTER (GClosure* closure, GValue* return_value, guint n_param_values, const GValue* param_values, gpointer invocation_hint, gpointer marshal_data)
{
	typedef void (*GMarshalFunc_VOID__POINTER_INT_POINTER) (gpointer data1, gpointer arg_1, gint arg_2, gpointer arg_3, gpointer data2);
	register GMarshalFunc_VOID__POINTER_INT_POINTER callback;
	register GCClosure * cc;
	register gpointer data1;
	register gpointer data2;
	cc = (GCClosure *) closure;
	g_return_if_fail (n_param_values == 4);
	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = param_values->data[0].v_pointer;
	} else {
		data1 = param_values->data[0].v_pointer;
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__POINTER_INT_POINTER) (marshal_data ? marshal_data : cc->callback);
	callback (data1, g_value_get_pointer (param_values + 1), g_value_get_int (param_values + 2), g_value_get_pointer (param_values + 3), data2);
}


static GObject*
samplecat_model_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (samplecat_model_parent_class);
	GObject* obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	return obj;
}


static void
samplecat_model_class_init (SamplecatModelClass* klass)
{
	samplecat_model_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (SamplecatModelPrivate));
	G_OBJECT_CLASS (klass)->constructor = samplecat_model_constructor;
	G_OBJECT_CLASS (klass)->finalize = samplecat_model_finalize;
	g_signal_new ("dir_list_changed", SAMPLECAT_TYPE_MODEL, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("selection_changed", SAMPLECAT_TYPE_MODEL, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);
	g_signal_new ("sample_changed", SAMPLECAT_TYPE_MODEL, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_user_marshal_VOID__POINTER_INT_POINTER, G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_INT, G_TYPE_POINTER);
}


static void
samplecat_model_instance_init (SamplecatModel* self)
{
	self->priv = SAMPLECAT_MODEL_GET_PRIVATE (self);
	self->state = 0;
}


static void
samplecat_model_finalize (GObject* obj)
{
	SamplecatModel* self = G_TYPE_CHECK_INSTANCE_CAST (obj, SAMPLECAT_TYPE_MODEL, SamplecatModel);
	_g_list_free0 (self->backends);
	_g_list_free0 (self->filters_);
	_samplecat_idle_unref0 (self->priv->dir_idle);
	_samplecat_idle_unref0 (self->priv->sample_changed_idle);
	_g_list_free0 (self->modified);
	G_OBJECT_CLASS (samplecat_model_parent_class)->finalize (obj);
}


GType
samplecat_model_get_type (void)
{
	static volatile gsize samplecat_model_type_id__volatile = 0;
	if (g_once_init_enter (&samplecat_model_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (SamplecatModelClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) samplecat_model_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (SamplecatModel), 0, (GInstanceInitFunc) samplecat_model_instance_init, NULL };
		GType samplecat_model_type_id;
		samplecat_model_type_id = g_type_register_static (G_TYPE_OBJECT, "SamplecatModel", &g_define_type_info, 0);
		g_once_init_leave (&samplecat_model_type_id__volatile, samplecat_model_type_id);
	}
	return samplecat_model_type_id__volatile;
}



