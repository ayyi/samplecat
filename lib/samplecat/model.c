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
#include <glib.h>
#include <glib-object.h>
#include <time.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gobject/gvaluecollector.h>
#include <debug/debug.h>
#include <samplecat.h>
#include <db/db.h>

#define backend samplecat.model->backend
#define _g_free0(var) (var = (g_free (var), NULL))
#define _samplecat_idle_unref0(var) ((var == NULL) ? NULL : (var = (samplecat_idle_unref (var), NULL)))
#define _sample_unref0(var) ((var == NULL) ? NULL : (var = (sample_unref (var), NULL)))

#define SAMPLECAT_TYPE_IDLE (samplecat_idle_get_type ())
#define SAMPLECAT_IDLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_IDLE, SamplecatIdle))
#define SAMPLECAT_IDLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_IDLE, SamplecatIdleClass))
#define SAMPLECAT_IS_IDLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLECAT_TYPE_IDLE))
#define SAMPLECAT_IS_IDLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_IDLE))
#define SAMPLECAT_IDLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_IDLE, SamplecatIdleClass))

typedef struct _SamplecatIdle SamplecatIdle;
typedef struct _SamplecatIdleClass SamplecatIdleClass;
typedef struct _SamplecatIdlePrivate SamplecatIdlePrivate;

struct _SamplecatIdle {
	GTypeInstance          parent_instance;
	volatile int           ref_count;
	SamplecatIdlePrivate*  priv;
};

struct _SamplecatIdleClass {
	GTypeClass             parent_class;
	void                   (*finalize) (SamplecatIdle*);
};

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

static gpointer samplecat_idle_parent_class = NULL;
static gchar    samplecat_model_unk[32];
static gchar    samplecat_model_unk[32] = {0};

#define _G_TYPE_INSTANCE_GET_PRIVATE(instance, g_type, c_type) \
	((c_type*) g_type_instance_get_private ((GTypeInstance*) (instance), (g_type)))
#define SAMPLECAT_IDLE_GET_PRIVATE(o) (_G_TYPE_INSTANCE_GET_PRIVATE ((o), SAMPLECAT_TYPE_IDLE, SamplecatIdlePrivate))
enum  {
	SAMPLECAT_IDLE_DUMMY_PROPERTY
};
static void     samplecat_idle_finalize   (SamplecatIdle* obj);

G_DEFINE_TYPE_WITH_PRIVATE (SamplecatModel, samplecat_model, G_TYPE_OBJECT)
enum  {
	SAMPLECAT_MODEL_DUMMY_PROPERTY
};
static gboolean samplecat_model_queue_selection_changed (gpointer);
static void     g_cclosure_user_marshal_VOID__POINTER_INT_POINTER (GClosure*, GValue* return_value, guint n_param_values, const GValue* param_values, gpointer invocation_hint, gpointer marshal_data);
static GObject* samplecat_model_constructor (GType type, guint n_construct_properties, GObjectConstructParam*);
static void     samplecat_model_finalize (GObject*);


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
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	g_type_class_add_private (klass, sizeof (SamplecatIdlePrivate));
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
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
	SamplecatIdle* self = G_TYPE_CHECK_INSTANCE_CAST (obj, SAMPLECAT_TYPE_IDLE, SamplecatIdle);

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

static gboolean
__sample_changed_idle (gpointer _self)
{
	SamplecatModel* self = _self;

	GList* l = self->modified;
	for (;l;l=l->next) {
		SamplecatSampleChange* change = l->data;
		g_signal_emit_by_name (self, "sample-changed", change->sample, change->prop, change->val);
		sample_unref(change->sample);
		g_free (change);
	}
	g_list_free0 (self->modified);

	return G_SOURCE_REMOVE;
}

const char* names[] = {"search", "directory", "category"};

SamplecatModel*
samplecat_model_construct (GType object_type)
{
	SamplecatModel* self = (SamplecatModel*) g_object_new (object_type, NULL);
	self->state = 1;
	self->cache_dir = g_build_filename (g_get_home_dir(), ".config", PACKAGE, "cache", NULL, NULL);

	self->categories[0] = "drums";
	self->categories[1] = "perc";
	self->categories[2] = "bass";
	self->categories[3] = "keys";
	self->categories[4] = "synth";
	self->categories[5] = "strings";
	self->categories[6] = "brass";
	self->categories[7] = "fx";
	self->categories[8] = "impulse";
	self->categories[9] = "breaks";

	// note that category value must be NULL if not set - empty string is invalid
	for(int i = 0; i < N_FILTERS; i++){
		if(true || i == FILTER_CATEGORY){
			self->filters3[i] = named_observable_new(names[i]);
		}else{
			observable_string_set(self->filters3[i] = named_observable_new(names[i]), g_strdup(""));
		}
	}

	_samplecat_idle_unref0 (self->priv->dir_idle);
	self->priv->dir_idle = samplecat_idle_new (___lambda__gsource_func, self);

#if 0 // updating the directory list is not currently done until a consumer needs it
	samplecat_idle_queue (self->priv->dir_idle);
#endif

	self->priv->sample_changed_idle = samplecat_idle_new(__sample_changed_idle, self);

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
	g_return_if_fail (self);

	observable_string_set (self->filters2.dir, dir);
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
		self->priv->selection_change_timeout = g_timeout_add_full (G_PRIORITY_DEFAULT, (guint) 250, samplecat_model_queue_selection_changed, g_object_ref (self), g_object_unref);
	}
}


static gboolean
samplecat_model_queue_selection_changed (gpointer _self)
{
	SamplecatModel* self = _self;
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


bool
samplecat_model_update_sample (SamplecatModel* self, Sample* sample, gint prop, void* val)
{
	g_return_val_if_fail (self, FALSE);

	bool ok = FALSE;

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
			if((ok = backend.update_string (sample->id, "notes", (gchar*)val))){
				gchar* str = g_strdup ((const gchar*)val);
				_g_free0 (sample->notes);
				sample->notes = str;
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
			gchar* metadata = sample_get_metadata_str (sample);
			ok = TRUE;

			ok &= backend.update_int (sample->id, "channels", sample->channels);
			ok &= backend.update_int (sample->id, "sample_rate", sample->sample_rate);
			ok &= backend.update_int (sample->id, "length", (guint) sample->length);
			ok &= backend.update_int (sample->id, "frames", (guint) sample->frames);
			ok &= backend.update_int (sample->id, "bit_rate", (guint) sample->bit_rate);
			ok &= backend.update_int (sample->id, "bit_depth", (guint) sample->bit_depth);

			if(sample->peaklevel) ok &= backend.update_float (sample->id, "peaklevel", sample->peaklevel);

			if(sample->ebur && sample->ebur[0]) ok = backend.update_string (sample->id, "ebur", sample->ebur);

			if (sample->overview) {
				guint len = 0U;
				guint8* blob = pixbuf_to_blob (sample->overview, &len);
				ok = backend.update_blob (sample->id, "pixbuf", blob, len);
			}
			ok &= backend.update_string (sample->id, "meta_data", metadata);
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
			gchar* a = g_strdup_printf ("UNKNOWN PROPERTY (%u)", prop_type);
			memcpy (samplecat_model_unk, a, (gsize) 31);
			_g_free0 (a);
			break;
		}
	}
	result = samplecat_model_unk;
	return result;
}


void
samplecat_model_move_files (GList* list, const gchar* dest_path)
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

	G_OBJECT_CLASS (klass)->constructor = samplecat_model_constructor;
	G_OBJECT_CLASS (klass)->finalize = samplecat_model_finalize;

	g_signal_new ("dir_list_changed", SAMPLECAT_TYPE_MODEL, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("selection_changed", SAMPLECAT_TYPE_MODEL, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);
	g_signal_new ("sample_changed", SAMPLECAT_TYPE_MODEL, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_user_marshal_VOID__POINTER_INT_POINTER, G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_INT, G_TYPE_POINTER);
}


static void
samplecat_model_init (SamplecatModel* self)
{
	self->priv = samplecat_model_get_instance_private(self);
	self->state = 0;
}


static void
samplecat_model_finalize (GObject* obj)
{
	SamplecatModel* self = G_TYPE_CHECK_INSTANCE_CAST (obj, SAMPLECAT_TYPE_MODEL, SamplecatModel);
	g_list_free0 (self->backends);
	for(int i = 0; i < N_FILTERS; i++){
		g_free(self->filters3[i]->value.c);
		observable_free(self->filters3[i]);
	}
	_samplecat_idle_unref0 (self->priv->dir_idle);
	_samplecat_idle_unref0 (self->priv->sample_changed_idle);
	g_list_free0 (self->modified);
	G_OBJECT_CLASS (samplecat_model_parent_class)->finalize (obj);
}
