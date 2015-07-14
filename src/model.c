/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2015 Tim Orford <tim@orford.org>                  |
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
#include <sample.h>
#include <config.h>
#include <db/db.h>
#include <samplecat/support.h>
#include <time.h>
#include <worker.h>
#include <debug/debug.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <support.h>
#include <stdlib.h>
#include <list_store.h>
#include <gobject/gvaluecollector.h>
#include <application.h>


#define SAMPLECAT_TYPE_FILTER (samplecat_filter_get_type ())
#define SAMPLECAT_FILTER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_FILTER, SamplecatFilter))
#define SAMPLECAT_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_FILTER, SamplecatFilterClass))
#define SAMPLECAT_IS_FILTER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLECAT_TYPE_FILTER))
#define SAMPLECAT_IS_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_FILTER))
#define SAMPLECAT_FILTER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_FILTER, SamplecatFilterClass))

typedef struct _SamplecatFilter SamplecatFilter;
typedef struct _SamplecatFilterClass SamplecatFilterClass;
typedef struct _SamplecatFilterPrivate SamplecatFilterPrivate;
#define _g_free0(var) (var = (g_free (var), NULL))
typedef struct _SamplecatParamSpecFilter SamplecatParamSpecFilter;

#define SAMPLECAT_TYPE_FILTERS (samplecat_filters_get_type ())
typedef struct _SamplecatFilters SamplecatFilters;

#define SAMPLECAT_TYPE_IDLE (samplecat_idle_get_type ())
#define SAMPLECAT_IDLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_IDLE, SamplecatIdle))
#define SAMPLECAT_IDLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_IDLE, SamplecatIdleClass))
#define SAMPLECAT_IS_IDLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLECAT_TYPE_IDLE))
#define SAMPLECAT_IS_IDLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_IDLE))
#define SAMPLECAT_IDLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_IDLE, SamplecatIdleClass))

typedef struct _SamplecatIdle SamplecatIdle;
typedef struct _SamplecatIdleClass SamplecatIdleClass;
typedef struct _SamplecatIdlePrivate SamplecatIdlePrivate;
typedef struct _SamplecatParamSpecIdle SamplecatParamSpecIdle;

typedef struct _SamplecatSampleChange SamplecatSampleChange;

#define SAMPLECAT_TYPE_MODEL (samplecat_model_get_type ())
#define SAMPLECAT_MODEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_MODEL, SamplecatModel))
#define SAMPLECAT_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_MODEL, SamplecatModelClass))
#define SAMPLECAT_IS_MODEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLECAT_TYPE_MODEL))
#define SAMPLECAT_IS_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_MODEL))
#define SAMPLECAT_MODEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_MODEL, SamplecatModelClass))

typedef struct _SamplecatModel SamplecatModel;
typedef struct _SamplecatModelClass SamplecatModelClass;
typedef struct _SamplecatModelPrivate SamplecatModelPrivate;
#define _g_list_free0(var) ((var == NULL) ? NULL : (var = (g_list_free (var), NULL)))
#define _samplecat_idle_unref0(var) ((var == NULL) ? NULL : (var = (samplecat_idle_unref (var), NULL)))
#define _sample_unref0(var) ((var == NULL) ? NULL : (var = (sample_unref (var), NULL)))

struct _SamplecatFilter {
	GTypeInstance parent_instance;
	volatile int ref_count;
	SamplecatFilterPrivate * priv;
	gchar* name;
	gchar* value;
};

struct _SamplecatFilterClass {
	GTypeClass parent_class;
	void (*finalize) (SamplecatFilter *self);
};

struct _SamplecatParamSpecFilter {
	GParamSpec parent_instance;
};

struct _SamplecatFilters {
	SamplecatFilter* search;
	SamplecatFilter* dir;
	SamplecatFilter* category;
};

struct _SamplecatIdle {
	GTypeInstance parent_instance;
	volatile int ref_count;
	SamplecatIdlePrivate * priv;
};

struct _SamplecatIdleClass {
	GTypeClass parent_class;
	void (*finalize) (SamplecatIdle *self);
};

struct _SamplecatIdlePrivate {
	guint id;
	GSourceFunc fn;
	gpointer fn_target;
	GDestroyNotify fn_target_destroy_notify;
};

struct _SamplecatParamSpecIdle {
	GParamSpec parent_instance;
};

struct _SamplecatSampleChange {
	Sample* sample;
	gint prop;
	void* val;
};

struct _SamplecatModel {
	GObject parent_instance;
	SamplecatModelPrivate * priv;
	gint state;
	gchar* cache_dir;
	GList* backends;
	SamplecatFilters filters;
	GList* filters_;
	Sample* selection;
	GList* modified;
};

struct _SamplecatModelClass {
	GObjectClass parent_class;
};

struct _SamplecatModelPrivate {
	SamplecatIdle* idle;
	SamplecatIdle* sample_changed_idle;
	guint selection_change_timeout;
};


static gpointer samplecat_filter_parent_class = NULL;
static gpointer samplecat_idle_parent_class = NULL;
static gpointer samplecat_model_parent_class = NULL;
static gchar samplecat_model_unk[32];
static gchar samplecat_model_unk[32] = {0};

gpointer samplecat_filter_ref (gpointer instance);
void samplecat_filter_unref (gpointer instance);
GParamSpec* samplecat_param_spec_filter (const gchar* name, const gchar* nick, const gchar* blurb, GType object_type, GParamFlags flags);
void samplecat_value_set_filter (GValue* value, gpointer v_object);
void samplecat_value_take_filter (GValue* value, gpointer v_object);
gpointer samplecat_value_get_filter (const GValue* value);
GType samplecat_filter_get_type (void) G_GNUC_CONST;
enum  {
	SAMPLECAT_FILTER_DUMMY_PROPERTY
};
SamplecatFilter* samplecat_filter_new (gchar* _name);
SamplecatFilter* samplecat_filter_construct (GType object_type, gchar* _name);
void samplecat_filter_set_value (SamplecatFilter* self, gchar* val);
static void samplecat_filter_finalize (SamplecatFilter* obj);
GType samplecat_filters_get_type (void) G_GNUC_CONST;
SamplecatFilters* samplecat_filters_dup (const SamplecatFilters* self);
void samplecat_filters_free (SamplecatFilters* self);
gpointer samplecat_idle_ref (gpointer instance);
void samplecat_idle_unref (gpointer instance);
GParamSpec* samplecat_param_spec_idle (const gchar* name, const gchar* nick, const gchar* blurb, GType object_type, GParamFlags flags);
void samplecat_value_set_idle (GValue* value, gpointer v_object);
void samplecat_value_take_idle (GValue* value, gpointer v_object);
gpointer samplecat_value_get_idle (const GValue* value);
GType samplecat_idle_get_type (void) G_GNUC_CONST;
#define SAMPLECAT_IDLE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SAMPLECAT_TYPE_IDLE, SamplecatIdlePrivate))
enum  {
	SAMPLECAT_IDLE_DUMMY_PROPERTY
};
SamplecatIdle* samplecat_idle_new (GSourceFunc _fn, void* _fn_target);
SamplecatIdle* samplecat_idle_construct (GType object_type, GSourceFunc _fn, void* _fn_target);
void samplecat_idle_queue (SamplecatIdle* self);
static gboolean ___lambda2_ (SamplecatIdle* self);
static gboolean ____lambda2__gsource_func (gpointer self);
static void samplecat_idle_finalize (SamplecatIdle* obj);
GType samplecat_model_get_type (void) G_GNUC_CONST;
#define SAMPLECAT_MODEL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SAMPLECAT_TYPE_MODEL, SamplecatModelPrivate))
enum  {
	SAMPLECAT_MODEL_DUMMY_PROPERTY
};
SamplecatModel* samplecat_model_new (void);
SamplecatModel* samplecat_model_construct (GType object_type);
static gboolean __lambda3_ (SamplecatModel* self);
static gboolean ___lambda3__gsource_func (gpointer self);
gboolean samplecat_model_add (SamplecatModel* self);
gboolean samplecat_model_remove (SamplecatModel* self, gint id);
void samplecat_model_set_search_dir (SamplecatModel* self, gchar* dir);
void samplecat_model_set_selection (SamplecatModel* self, Sample* sample);
static gboolean samplecat_model_queue_selection_changed (SamplecatModel* self);
static gboolean _samplecat_model_queue_selection_changed_gsource_func (gpointer self);
void samplecat_model_add_filter (SamplecatModel* self, SamplecatFilter* filter);
void samplecat_model_refresh_sample (SamplecatModel* self, Sample* sample, gboolean force_update);
gboolean samplecat_model_update_sample (SamplecatModel* self, Sample* sample, gint prop, void* val);
gchar* samplecat_model_print_col_name (guint prop_type);
static void g_cclosure_user_marshal_VOID__POINTER_INT_POINTER (GClosure * closure, GValue * return_value, guint n_param_values, const GValue * param_values, gpointer invocation_hint, gpointer marshal_data);
static GObject * samplecat_model_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void samplecat_model_finalize (GObject* obj);


SamplecatFilter* samplecat_filter_construct (GType object_type, gchar* _name) {
	SamplecatFilter* self = NULL;
	gchar* _tmp0_;
	self = (SamplecatFilter*) g_type_create_instance (object_type);
	_tmp0_ = _name;
	self->name = _tmp0_;
	return self;
}


SamplecatFilter* samplecat_filter_new (gchar* _name) {
	return samplecat_filter_construct (SAMPLECAT_TYPE_FILTER, _name);
}


void samplecat_filter_set_value (SamplecatFilter* self, gchar* val) {
	gchar* _tmp0_;
	gchar* _tmp1_;
	g_return_if_fail (self != NULL);
	_tmp0_ = val;
	_tmp1_ = g_strdup ((const gchar*) _tmp0_);
	_g_free0 (self->value);
	self->value = _tmp1_;
	g_signal_emit_by_name (self, "changed");
}


static void samplecat_value_filter_init (GValue* value) {
	value->data[0].v_pointer = NULL;
}


static void samplecat_value_filter_free_value (GValue* value) {
	if (value->data[0].v_pointer) {
		samplecat_filter_unref (value->data[0].v_pointer);
	}
}


static void samplecat_value_filter_copy_value (const GValue* src_value, GValue* dest_value) {
	if (src_value->data[0].v_pointer) {
		dest_value->data[0].v_pointer = samplecat_filter_ref (src_value->data[0].v_pointer);
	} else {
		dest_value->data[0].v_pointer = NULL;
	}
}


static gpointer samplecat_value_filter_peek_pointer (const GValue* value) {
	return value->data[0].v_pointer;
}


static gchar* samplecat_value_filter_collect_value (GValue* value, guint n_collect_values, GTypeCValue* collect_values, guint collect_flags) {
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


static gchar* samplecat_value_filter_lcopy_value (const GValue* value, guint n_collect_values, GTypeCValue* collect_values, guint collect_flags) {
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


GParamSpec* samplecat_param_spec_filter (const gchar* name, const gchar* nick, const gchar* blurb, GType object_type, GParamFlags flags) {
	SamplecatParamSpecFilter* spec;
	g_return_val_if_fail (g_type_is_a (object_type, SAMPLECAT_TYPE_FILTER), NULL);
	spec = g_param_spec_internal (G_TYPE_PARAM_OBJECT, name, nick, blurb, flags);
	G_PARAM_SPEC (spec)->value_type = object_type;
	return G_PARAM_SPEC (spec);
}


gpointer samplecat_value_get_filter (const GValue* value) {
	g_return_val_if_fail (G_TYPE_CHECK_VALUE_TYPE (value, SAMPLECAT_TYPE_FILTER), NULL);
	return value->data[0].v_pointer;
}


void samplecat_value_set_filter (GValue* value, gpointer v_object) {
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


void samplecat_value_take_filter (GValue* value, gpointer v_object) {
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


static void samplecat_filter_class_init (SamplecatFilterClass * klass) {
	samplecat_filter_parent_class = g_type_class_peek_parent (klass);
	SAMPLECAT_FILTER_CLASS (klass)->finalize = samplecat_filter_finalize;
	g_signal_new ("changed", SAMPLECAT_TYPE_FILTER, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}


static void samplecat_filter_instance_init (SamplecatFilter * self) {
	self->ref_count = 1;
}


static void samplecat_filter_finalize (SamplecatFilter* obj) {
	SamplecatFilter * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (obj, SAMPLECAT_TYPE_FILTER, SamplecatFilter);
	_g_free0 (self->value);
}


GType samplecat_filter_get_type (void) {
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


gpointer samplecat_filter_ref (gpointer instance) {
	SamplecatFilter* self;
	self = instance;
	g_atomic_int_inc (&self->ref_count);
	return instance;
}


void samplecat_filter_unref (gpointer instance) {
	SamplecatFilter* self;
	self = instance;
	if (g_atomic_int_dec_and_test (&self->ref_count)) {
		SAMPLECAT_FILTER_GET_CLASS (self)->finalize (self);
		g_type_free_instance ((GTypeInstance *) self);
	}
}


SamplecatFilters* samplecat_filters_dup (const SamplecatFilters* self) {
	SamplecatFilters* dup;
	dup = g_new0 (SamplecatFilters, 1);
	memcpy (dup, self, sizeof (SamplecatFilters));
	return dup;
}


void samplecat_filters_free (SamplecatFilters* self) {
	g_free (self);
}


GType samplecat_filters_get_type (void) {
	static volatile gsize samplecat_filters_type_id__volatile = 0;
	if (g_once_init_enter (&samplecat_filters_type_id__volatile)) {
		GType samplecat_filters_type_id;
		samplecat_filters_type_id = g_boxed_type_register_static ("SamplecatFilters", (GBoxedCopyFunc) samplecat_filters_dup, (GBoxedFreeFunc) samplecat_filters_free);
		g_once_init_leave (&samplecat_filters_type_id__volatile, samplecat_filters_type_id);
	}
	return samplecat_filters_type_id__volatile;
}


SamplecatIdle* samplecat_idle_construct (GType object_type, GSourceFunc _fn, void* _fn_target) {
	SamplecatIdle* self = NULL;
	GSourceFunc _tmp0_;
	void* _tmp0__target;
	self = (SamplecatIdle*) g_type_create_instance (object_type);
	_tmp0_ = _fn;
	_tmp0__target = _fn_target;
	(self->priv->fn_target_destroy_notify == NULL) ? NULL : (self->priv->fn_target_destroy_notify (self->priv->fn_target), NULL);
	self->priv->fn = NULL;
	self->priv->fn_target = NULL;
	self->priv->fn_target_destroy_notify = NULL;
	self->priv->fn = _tmp0_;
	self->priv->fn_target = _tmp0__target;
	self->priv->fn_target_destroy_notify = NULL;
	return self;
}


SamplecatIdle* samplecat_idle_new (GSourceFunc _fn, void* _fn_target) {
	return samplecat_idle_construct (SAMPLECAT_TYPE_IDLE, _fn, _fn_target);
}


static gboolean ___lambda2_ (SamplecatIdle* self) {
	gboolean result = FALSE;
	GSourceFunc _tmp0_;
	void* _tmp0__target;
	_tmp0_ = self->priv->fn;
	_tmp0__target = self->priv->fn_target;
	_tmp0_ (_tmp0__target);
	self->priv->id = (guint) 0;
	result = FALSE;
	return result;
}


static gboolean ____lambda2__gsource_func (gpointer self) {
	gboolean result;
	result = ___lambda2_ (self);
	return result;
}


void samplecat_idle_queue (SamplecatIdle* self) {
	guint _tmp0_;
	g_return_if_fail (self != NULL);
	_tmp0_ = self->priv->id;
	if (!((gboolean) _tmp0_)) {
		guint _tmp1_ = 0U;
		_tmp1_ = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, ____lambda2__gsource_func, samplecat_idle_ref (self), samplecat_idle_unref);
		self->priv->id = _tmp1_;
	}
}


static void samplecat_value_idle_init (GValue* value) {
	value->data[0].v_pointer = NULL;
}


static void samplecat_value_idle_free_value (GValue* value) {
	if (value->data[0].v_pointer) {
		samplecat_idle_unref (value->data[0].v_pointer);
	}
}


static void samplecat_value_idle_copy_value (const GValue* src_value, GValue* dest_value) {
	if (src_value->data[0].v_pointer) {
		dest_value->data[0].v_pointer = samplecat_idle_ref (src_value->data[0].v_pointer);
	} else {
		dest_value->data[0].v_pointer = NULL;
	}
}


static gpointer samplecat_value_idle_peek_pointer (const GValue* value) {
	return value->data[0].v_pointer;
}


static gchar* samplecat_value_idle_collect_value (GValue* value, guint n_collect_values, GTypeCValue* collect_values, guint collect_flags) {
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


static gchar* samplecat_value_idle_lcopy_value (const GValue* value, guint n_collect_values, GTypeCValue* collect_values, guint collect_flags) {
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


GParamSpec* samplecat_param_spec_idle (const gchar* name, const gchar* nick, const gchar* blurb, GType object_type, GParamFlags flags) {
	SamplecatParamSpecIdle* spec;
	g_return_val_if_fail (g_type_is_a (object_type, SAMPLECAT_TYPE_IDLE), NULL);
	spec = g_param_spec_internal (G_TYPE_PARAM_OBJECT, name, nick, blurb, flags);
	G_PARAM_SPEC (spec)->value_type = object_type;
	return G_PARAM_SPEC (spec);
}


gpointer samplecat_value_get_idle (const GValue* value) {
	g_return_val_if_fail (G_TYPE_CHECK_VALUE_TYPE (value, SAMPLECAT_TYPE_IDLE), NULL);
	return value->data[0].v_pointer;
}


void samplecat_value_set_idle (GValue* value, gpointer v_object) {
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


void samplecat_value_take_idle (GValue* value, gpointer v_object) {
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


static void samplecat_idle_class_init (SamplecatIdleClass * klass) {
	samplecat_idle_parent_class = g_type_class_peek_parent (klass);
	SAMPLECAT_IDLE_CLASS (klass)->finalize = samplecat_idle_finalize;
	g_type_class_add_private (klass, sizeof (SamplecatIdlePrivate));
}


static void samplecat_idle_instance_init (SamplecatIdle * self) {
	self->priv = SAMPLECAT_IDLE_GET_PRIVATE (self);
	self->priv->id = (guint) 0;
	self->ref_count = 1;
}


static void samplecat_idle_finalize (SamplecatIdle* obj) {
	SamplecatIdle * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (obj, SAMPLECAT_TYPE_IDLE, SamplecatIdle);
	(self->priv->fn_target_destroy_notify == NULL) ? NULL : (self->priv->fn_target_destroy_notify (self->priv->fn_target), NULL);
	self->priv->fn = NULL;
	self->priv->fn_target = NULL;
	self->priv->fn_target_destroy_notify = NULL;
}


GType samplecat_idle_get_type (void) {
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


gpointer samplecat_idle_ref (gpointer instance) {
	SamplecatIdle* self;
	self = instance;
	g_atomic_int_inc (&self->ref_count);
	return instance;
}


void samplecat_idle_unref (gpointer instance) {
	SamplecatIdle* self;
	self = instance;
	if (g_atomic_int_dec_and_test (&self->ref_count)) {
		SAMPLECAT_IDLE_GET_CLASS (self)->finalize (self);
		g_type_free_instance ((GTypeInstance *) self);
	}
}


static gboolean __lambda3_ (SamplecatModel* self) {
	gboolean result = FALSE;
	g_signal_emit_by_name (self, "dir-list-changed");
	result = FALSE;
	return result;
}


static gboolean ___lambda3__gsource_func (gpointer self) {
	gboolean result;
	result = __lambda3_ (self);
	return result;
}


static gboolean __lambda4_ (gpointer _self) {
	SamplecatModel* self = _self;

	GList* _tmp0_ = self->modified;
	{
		GList* change_collection = NULL;
		GList* change_it = NULL;
		change_collection = _tmp0_;
		for (change_it = change_collection; change_it != NULL; change_it = change_it->next) {
			SamplecatSampleChange* change = NULL;
			change = change_it->data;
			{
				g_signal_emit_by_name (self, "sample-changed", change->sample, change->prop, change->val);
				sample_unref(change->sample);
				g_free (change);
			}
		}
	}
	_g_list_free0 (self->modified);

	return FALSE;
}


SamplecatModel* samplecat_model_construct (GType object_type) {
	SamplecatModel * self = NULL;
	const gchar* _tmp0_ = NULL;
	gchar* _tmp1_ = NULL;
	SamplecatIdle* _tmp2_;
	self = (SamplecatModel*) g_object_new (object_type, NULL);
	self->state = 1;
	_tmp0_ = g_get_home_dir ();
	_tmp1_ = g_build_filename (_tmp0_, ".config", PACKAGE, "cache", NULL, NULL);
	self->cache_dir = _tmp1_;

	_tmp2_ = samplecat_idle_new (___lambda3__gsource_func, self);
	_samplecat_idle_unref0 (self->priv->idle);
	self->priv->idle = _tmp2_;

	self->priv->sample_changed_idle = samplecat_idle_new (__lambda4_, self);

	return self;
}


SamplecatModel* samplecat_model_new (void) {
	return samplecat_model_construct (SAMPLECAT_TYPE_MODEL);
}


gboolean samplecat_model_add (SamplecatModel* self) {
	gboolean result = FALSE;
	SamplecatIdle* _tmp0_;
	g_return_val_if_fail (self != NULL, FALSE);
	_tmp0_ = self->priv->idle;
	samplecat_idle_queue (_tmp0_);
	result = TRUE;
	return result;
}


gboolean samplecat_model_remove (SamplecatModel* self, gint id) {
	gboolean result = FALSE;
	SamplecatIdle* _tmp0_;
	gint _tmp1_;
	gboolean _tmp2_ = FALSE;
	g_return_val_if_fail (self != NULL, FALSE);
	_tmp0_ = self->priv->idle;
	samplecat_idle_queue (_tmp0_);
	_tmp1_ = id;
	_tmp2_ = backend.remove (_tmp1_);
	result = _tmp2_;
	return result;
}


void samplecat_model_set_search_dir (SamplecatModel* self, gchar* dir) {
	SamplecatFilters _tmp0_;
	SamplecatFilter* _tmp1_;
	gchar* _tmp2_;
	g_return_if_fail (self != NULL);
	_tmp0_ = self->filters;
	_tmp1_ = _tmp0_.dir;
	_tmp2_ = dir;
	samplecat_filter_set_value (_tmp1_, _tmp2_);
}


static gboolean _samplecat_model_queue_selection_changed_gsource_func (gpointer self) {
	gboolean result;
	result = samplecat_model_queue_selection_changed (self);
	return result;
}


void samplecat_model_set_selection (SamplecatModel* self, Sample* sample) {
	Sample* _tmp0_;
	Sample* _tmp1_;
	g_return_if_fail (self != NULL);
	_tmp0_ = sample;
	_tmp1_ = self->selection;
	if (_tmp0_ != _tmp1_) {
		Sample* _tmp2_;
		Sample* _tmp4_;
		Sample* _tmp5_;
		guint _tmp6_;
		guint _tmp8_ = 0U;
		_tmp2_ = self->selection;
		if ((gboolean) _tmp2_) {
			Sample* _tmp3_;
			_tmp3_ = self->selection;
			sample_unref (_tmp3_);
		}
		_tmp4_ = sample;
		sample = NULL;
		self->selection = _tmp4_;
		_tmp5_ = self->selection;
		sample_ref (_tmp5_);
		_tmp6_ = self->priv->selection_change_timeout;
		if ((gboolean) _tmp6_) {
			guint _tmp7_;
			_tmp7_ = self->priv->selection_change_timeout;
			g_source_remove (_tmp7_);
		}
		_tmp8_ = g_timeout_add_full (G_PRIORITY_DEFAULT, (guint) 250, _samplecat_model_queue_selection_changed_gsource_func, g_object_ref (self), g_object_unref);
		self->priv->selection_change_timeout = _tmp8_;
	}
}


static gboolean samplecat_model_queue_selection_changed (SamplecatModel* self) {
	gboolean result = FALSE;
	Sample* _tmp0_;
	g_return_val_if_fail (self != NULL, FALSE);
	self->priv->selection_change_timeout = (guint) 0;
	_tmp0_ = self->selection;
	g_signal_emit_by_name (self, "selection-changed", _tmp0_);
	result = FALSE;
	return result;
}


static void samplecat_model_queue_sample_changed (SamplecatModel* self, Sample* sample, gint prop, void* val) {
	GList* _tmp0_;
	Sample* _tmp8_;
	SamplecatSampleChange* _tmp9_;
	gint _tmp10_;
	SamplecatSampleChange* _tmp11_;
	void* _tmp12_;
	SamplecatSampleChange* _tmp13_;
	g_return_if_fail (self != NULL);
	_tmp0_ = self->modified;
	{
		GList* change_collection = NULL;
		GList* change_it = NULL;
		change_collection = _tmp0_;
		for (change_it = change_collection; change_it != NULL; change_it = change_it->next) {
			SamplecatSampleChange* change = NULL;
			change = change_it->data;
			{
				SamplecatSampleChange* _tmp1_;
				Sample* _tmp2_;
				Sample* _tmp3_;
				_tmp1_ = change;
				_tmp2_ = (*_tmp1_).sample;
				_tmp3_ = sample;
				if (_tmp2_ == _tmp3_) {
					SamplecatSampleChange* _tmp4_;
					SamplecatSampleChange* _tmp5_;
					_tmp4_ = change;
					(*_tmp4_).prop = -1;
					_tmp5_ = change;
					(*_tmp5_).val = NULL;
					return;
				}
			}
		}
	}
	SamplecatSampleChange* c = g_new(SamplecatSampleChange, 1);
	c->sample = _tmp8_ = sample_ref(sample);
	_tmp9_ = c;
	_tmp10_ = prop;
	(*_tmp9_).prop = _tmp10_;
	_tmp11_ = c;
	_tmp12_ = val;
	(*_tmp11_).val = _tmp12_;
	_tmp13_ = c;
	self->modified = g_list_append (self->modified, _tmp13_);
	samplecat_idle_queue (self->priv->sample_changed_idle);
}


void samplecat_model_add_filter (SamplecatModel* self, SamplecatFilter* filter) {
	SamplecatFilter* _tmp0_;
	g_return_if_fail (self != NULL);
	_tmp0_ = filter;
	self->filters_ = g_list_prepend (self->filters_, _tmp0_);
}


void samplecat_model_refresh_sample (SamplecatModel* self, Sample* sample, gboolean force_update) {
	Sample* _tmp0_;
	gboolean _tmp1_;
	gboolean online;
	Sample* _tmp2_;
	gchar* _tmp3_;
	time_t _tmp4_ = 0;
	time_t mtime;
	time_t _tmp5_;
	Sample* _tmp28_;
	gboolean _tmp29_;
	gboolean _tmp30_;
	g_return_if_fail (self != NULL);
	_tmp0_ = sample;
	_tmp1_ = _tmp0_->online;
	online = _tmp1_;
	_tmp2_ = sample;
	_tmp3_ = _tmp2_->full_path;
	_tmp4_ = file_mtime (_tmp3_);
	mtime = _tmp4_;
	_tmp5_ = mtime;
	if (_tmp5_ > ((time_t) 0)) {
		gboolean _tmp6_ = FALSE;
		Sample* _tmp7_;
		time_t _tmp8_;
		time_t _tmp9_;
		gboolean _tmp11_;
		Sample* _tmp24_;
		time_t _tmp25_;
		Sample* _tmp26_;
		_tmp7_ = sample;
		_tmp8_ = _tmp7_->mtime;
		_tmp9_ = mtime;
		if (_tmp8_ < _tmp9_) {
			_tmp6_ = TRUE;
		} else {
			gboolean _tmp10_;
			_tmp10_ = force_update;
			_tmp6_ = _tmp10_;
		}
		_tmp11_ = _tmp6_;
		if (_tmp11_) {
			Sample* _tmp12_;
			gchar* _tmp13_;
			Sample* _tmp14_;
			Sample* test;
			Sample* _tmp15_;
			Sample* _tmp16_;
			gboolean _tmp17_ = FALSE;
			_tmp12_ = sample;
			_tmp13_ = _tmp12_->full_path;
			_tmp14_ = sample_new_from_filename (_tmp13_, FALSE);
			test = _tmp14_;
			_tmp15_ = test;
			if (!((gboolean) _tmp15_)) {
				_sample_unref0 (test);
				return;
			}
			_tmp16_ = sample;
			_tmp17_ = sample_get_file_info (_tmp16_);
			if (_tmp17_) {
				Sample* _tmp18_;
				time_t _tmp19_;
				Sample* _tmp20_;
				Sample* _tmp21_;
				Sample* _tmp22_;
				Sample* _tmp23_;
				_tmp18_ = sample;
				_tmp19_ = mtime;
				_tmp18_->mtime = _tmp19_;
				_tmp20_ = sample;
				g_signal_emit_by_name (self, "sample-changed", _tmp20_, -1, NULL);
				_tmp21_ = sample;
				request_peaklevel (_tmp21_);
				_tmp22_ = sample;
				request_overview (_tmp22_);
				_tmp23_ = sample;
				request_ebur128 (_tmp23_);
			} else {
				dbg (0, "full update - reading file info failed!");
			}
			_sample_unref0 (test);
		}
		_tmp24_ = sample;
		_tmp25_ = mtime;
		_tmp24_->mtime = _tmp25_;
		_tmp26_ = sample;
		_tmp26_->online = TRUE;
	} else {
		Sample* _tmp27_;
		_tmp27_ = sample;
		_tmp27_->online = FALSE;
	}
	_tmp28_ = sample;
	_tmp29_ = _tmp28_->online;
	_tmp30_ = online;
	if (_tmp29_ != _tmp30_) {
		Sample* _tmp31_;
		_tmp31_ = sample;
		samplecat_model_update_sample (self, _tmp31_, (gint) COL_ICON, NULL);
	}
}


gboolean samplecat_model_update_sample (SamplecatModel* self, Sample* sample, gint prop, void* val) {
	gboolean result = FALSE;
	gboolean ok;
	gint _tmp0_;
	gboolean _tmp105_;
	g_return_val_if_fail (self != NULL, FALSE);
	ok = FALSE;
	_tmp0_ = prop;
	switch (_tmp0_) {
		case COL_ICON:
		{
			gint _tmp1_ = 0;
			Sample* _tmp4_;
			gint _tmp5_;
			gint _tmp6_;
			gboolean _tmp7_ = FALSE;
			if (sample->online) {
				_tmp1_ = 1;
			} else {
				_tmp1_ = 0;
			}
			_tmp4_ = sample;
			_tmp5_ = _tmp4_->id;
			_tmp6_ = _tmp1_;
			_tmp7_ = backend.update_int (_tmp5_, "online", (guint) _tmp6_);
			if (_tmp7_) {
				Sample* _tmp8_;
				gint _tmp9_;
				Sample* _tmp10_;
				time_t _tmp11_;
				gboolean _tmp12_ = FALSE;
				_tmp8_ = sample;
				_tmp9_ = _tmp8_->id;
				_tmp10_ = sample;
				_tmp11_ = _tmp10_->mtime;
				_tmp12_ = backend.update_int (_tmp9_, "mtime", (guint) _tmp11_);
				ok = _tmp12_;
			}
			break;
		}
		case COL_KEYWORDS:
		{
			Sample* _tmp13_;
			gint _tmp14_;
			void* _tmp15_;
			gboolean _tmp16_ = FALSE;
			gboolean _tmp17_;
			_tmp13_ = sample;
			_tmp14_ = _tmp13_->id;
			_tmp15_ = val;
			_tmp16_ = backend.update_string (_tmp14_, "keywords", (gchar*) _tmp15_);
			ok = _tmp16_;
			_tmp17_ = ok;
			if (_tmp17_) {
				Sample* _tmp18_;
				void* _tmp19_;
				gchar* _tmp20_;
				_tmp18_ = sample;
				_tmp19_ = val;
				_tmp20_ = g_strdup ((const gchar*) _tmp19_);
				_g_free0 (_tmp18_->keywords);
				_tmp18_->keywords = _tmp20_;
			}
			break;
		}
		case COL_OVERVIEW:
		{
			Sample* _tmp21_;
			GdkPixbuf* _tmp22_;
			_tmp21_ = sample;
			_tmp22_ = _tmp21_->overview;
			if ((gboolean) _tmp22_) {
				guint len = 0U;
				Sample* _tmp23_;
				GdkPixbuf* _tmp24_;
				guint _tmp25_ = 0U;
				guint8* _tmp26_ = NULL;
				guint8* blob;
				Sample* _tmp27_;
				gint _tmp28_;
				guint8* _tmp29_;
				guint _tmp30_;
				gboolean _tmp31_ = FALSE;
				_tmp23_ = sample;
				_tmp24_ = _tmp23_->overview;
				_tmp26_ = pixbuf_to_blob (_tmp24_, &_tmp25_);
				len = _tmp25_;
				blob = _tmp26_;
				_tmp27_ = sample;
				_tmp28_ = _tmp27_->id;
				_tmp29_ = blob;
				_tmp30_ = len;
				_tmp31_ = backend.update_blob (_tmp28_, "pixbuf", _tmp29_, _tmp30_);
				ok = _tmp31_;
			}
			break;
		}
		case COL_COLOUR:
		{
			void* _tmp32_;
			guint colour_index;
			Sample* _tmp33_;
			gint _tmp34_;
			guint _tmp35_;
			gboolean _tmp36_ = FALSE;
			gboolean _tmp37_;
			_tmp32_ = val;
			colour_index = *((guint*) _tmp32_);
			_tmp33_ = sample;
			_tmp34_ = _tmp33_->id;
			_tmp35_ = colour_index;
			_tmp36_ = backend.update_int (_tmp34_, "colour", _tmp35_);
			ok = _tmp36_;
			_tmp37_ = ok;
			if (_tmp37_) {
				Sample* _tmp38_;
				guint _tmp39_;
				_tmp38_ = sample;
				_tmp39_ = colour_index;
				_tmp38_->colour_index = (gint) _tmp39_;
			}
			break;
		}
		case COL_PEAKLEVEL:
		{
			Sample* _tmp40_;
			gint _tmp41_;
			Sample* _tmp42_;
			gfloat _tmp43_;
			gboolean _tmp44_ = FALSE;
			gboolean _tmp45_;
			_tmp40_ = sample;
			_tmp41_ = _tmp40_->id;
			_tmp42_ = sample;
			_tmp43_ = _tmp42_->peaklevel;
			_tmp44_ = backend.update_float (_tmp41_, "peaklevel", _tmp43_);
			ok = _tmp44_;
			_tmp45_ = ok;
			if (_tmp45_) {
			}
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
			Sample* _tmp54_;
			gint _tmp55_;
			Sample* _tmp56_;
			gchar* _tmp57_;
			gboolean _tmp58_ = FALSE;
			_tmp54_ = sample;
			_tmp55_ = _tmp54_->id;
			_tmp56_ = sample;
			_tmp57_ = _tmp56_->ebur;
			_tmp58_ = backend.update_string (_tmp55_, "ebur", _tmp57_);
			ok = _tmp58_;
			break;
		}
		case COL_ALL:
		case -1: // deprecated
		{
			Sample* _tmp59_;
			gchar* _tmp60_ = NULL;
			gchar* metadata;
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
			Sample* _tmp76_;
			gint64 _tmp77_;
			gboolean _tmp78_ = FALSE;
			gboolean _tmp79_;
			Sample* _tmp80_;
			gint _tmp81_;
			Sample* _tmp82_;
			gint64 _tmp83_;
			gboolean _tmp84_ = FALSE;
			gboolean _tmp85_;
			Sample* _tmp86_;
			gint _tmp87_;
			Sample* _tmp88_;
			gint _tmp89_;
			gboolean _tmp90_ = FALSE;
			gboolean _tmp91_;
			Sample* _tmp92_;
			Sample* _tmp94_;
			gboolean _tmp96_ = FALSE;
			gboolean _tmp97_;
			Sample* _tmp98_;
			gboolean _tmp101_ = FALSE;
			gchar* _tmp102_;
			_tmp59_ = sample;
			_tmp60_ = sample_get_metadata_str (_tmp59_);
			metadata = _tmp60_;
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
			_tmp76_ = sample;
			_tmp77_ = _tmp76_->length;
			_tmp78_ = backend.update_int (_tmp75_, "length", (guint) _tmp77_);
			ok = _tmp73_ & _tmp78_;
			_tmp79_ = ok;
			_tmp80_ = sample;
			_tmp81_ = _tmp80_->id;
			_tmp82_ = sample;
			_tmp83_ = _tmp82_->frames;
			_tmp84_ = backend.update_int (_tmp81_, "frames", (guint) _tmp83_);
			ok = _tmp79_ & _tmp84_;
			_tmp85_ = ok;
			_tmp86_ = sample;
			_tmp87_ = _tmp86_->id;
			_tmp88_ = sample;
			_tmp89_ = _tmp88_->bit_rate;
			_tmp90_ = backend.update_int (_tmp87_, "bit_rate", (guint) _tmp89_);
			ok = _tmp85_ & _tmp90_;
			_tmp91_ = ok;
			_tmp92_ = sample;
			_tmp94_ = sample;
			_tmp96_ = backend.update_int (_tmp92_->id, "bit_depth", (guint) _tmp94_->bit_depth);
			ok = _tmp91_ & _tmp96_;
			_tmp97_ = ok;
			_tmp98_ = sample;
			_tmp101_ = backend.update_string (_tmp98_->id, "meta_data", metadata);
			ok = _tmp97_ & _tmp101_;
			_tmp102_ = metadata;
			if ((gboolean) _tmp102_) {
				GDestroyNotify _tmp103_;
				gchar* _tmp104_;
				_tmp103_ = g_free;
				_tmp104_ = metadata;
				_tmp103_ (_tmp104_);
			}
			break;
		}
		default:
		{
			warnprintf2 ("model.update_sample", "unhandled property", NULL);
			break;
		}
	}
	_tmp105_ = ok;
	if (_tmp105_) {
		Sample* _tmp106_;
		gint _tmp107_;
		void* _tmp108_;
		_tmp106_ = sample;
		_tmp107_ = prop;
		_tmp108_ = val;
		samplecat_model_queue_sample_changed (self, _tmp106_, _tmp107_, _tmp108_);
	} else {
		warnprintf2 ("model.update_sample", "database update failed for %s", samplecat_model_print_col_name ((guint)prop), NULL);
	}
	result = ok;
	return result;
}


gchar* samplecat_model_print_col_name (guint prop_type) {
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
				g_signal_emit_by_name(app->model, "sample-changed", s, COL_FNAME, NULL);
			}
		}
		g_free(full_path);
		l = l->next;
	} while (l);
}


static void g_cclosure_user_marshal_VOID__POINTER_INT_POINTER (GClosure * closure, GValue * return_value, guint n_param_values, const GValue * param_values, gpointer invocation_hint, gpointer marshal_data) {
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


static GObject * samplecat_model_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GObjectClass * parent_class;
	parent_class = G_OBJECT_CLASS (samplecat_model_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	return obj;
}


static void samplecat_model_class_init (SamplecatModelClass * klass) {
	samplecat_model_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (SamplecatModelPrivate));
	G_OBJECT_CLASS (klass)->constructor = samplecat_model_constructor;
	G_OBJECT_CLASS (klass)->finalize = samplecat_model_finalize;
	g_signal_new ("dir_list_changed", SAMPLECAT_TYPE_MODEL, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("selection_changed", SAMPLECAT_TYPE_MODEL, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);
	g_signal_new ("sample_changed", SAMPLECAT_TYPE_MODEL, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_user_marshal_VOID__POINTER_INT_POINTER, G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_INT, G_TYPE_POINTER);
}


static void samplecat_model_instance_init (SamplecatModel * self) {
	self->priv = SAMPLECAT_MODEL_GET_PRIVATE (self);
	self->state = 0;
}


static void samplecat_model_finalize (GObject* obj) {
	SamplecatModel * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (obj, SAMPLECAT_TYPE_MODEL, SamplecatModel);
	_g_list_free0 (self->backends);
	_g_list_free0 (self->filters_);
	_samplecat_idle_unref0 (self->priv->idle);
	_samplecat_idle_unref0 (self->priv->sample_changed_idle);
	_g_list_free0 (self->modified);
	G_OBJECT_CLASS (samplecat_model_parent_class)->finalize (obj);
}


GType samplecat_model_get_type (void) {
	static volatile gsize samplecat_model_type_id__volatile = 0;
	if (g_once_init_enter (&samplecat_model_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (SamplecatModelClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) samplecat_model_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (SamplecatModel), 0, (GInstanceInitFunc) samplecat_model_instance_init, NULL };
		GType samplecat_model_type_id;
		samplecat_model_type_id = g_type_register_static (G_TYPE_OBJECT, "SamplecatModel", &g_define_type_info, 0);
		g_once_init_leave (&samplecat_model_type_id__volatile, samplecat_model_type_id);
	}
	return samplecat_model_type_id__volatile;
}



