/* model.c generated by valac 0.12.1, the Vala compiler
 * generated from model.vala, do not modify */


#include <glib.h>
#include <glib-object.h>
#include <string.h>
#include <config.h>
#include <application.h>
#include <sample.h>
#include "model.h"


#define SAMPLECAT_TYPE_FILTERS (samplecat_filters_get_type ())

#define SAMPLECAT_TYPE_MODEL (samplecat_model_get_type ())
#define SAMPLECAT_MODEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_MODEL, SamplecatModel))
#define SAMPLECAT_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_MODEL, SamplecatModelClass))
#define SAMPLECAT_IS_MODEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLECAT_TYPE_MODEL))
#define SAMPLECAT_IS_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_MODEL))
#define SAMPLECAT_MODEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_MODEL, SamplecatModelClass))


struct x_SamplecatFilters {
	gchar phrase[256];
	gchar* dir;
	gchar* category;
};

struct x_SamplecatModel {
	GObject parent_instance;
	SamplecatModelPrivate * priv;
	gint state;
	gchar* cache_dir;
	SamplecatFilters filters;
};

struct x_SamplecatModelClass {
	GObjectClass parent_class;
};


static gpointer samplecat_model_parent_class = NULL;

GType samplecat_filters_get_type (void) G_GNUC_CONST;
SamplecatFilters* samplecat_filters_dup (const SamplecatFilters* self);
void samplecat_filters_free (SamplecatFilters* self);
GType samplecat_model_get_type (void) G_GNUC_CONST;
enum  {
	SAMPLECAT_MODEL_DUMMY_PROPERTY
};
SamplecatModel* samplecat_model_new (void);
SamplecatModel* samplecat_model_construct (GType object_type);
void samplecat_model_set_search_dir (SamplecatModel* self, gchar* dir);
static void g_cclosure_user_marshal_VOID__POINTER_INT_POINTER (GClosure * closure, GValue * return_value, guint n_param_values, const GValue * param_values, gpointer invocation_hint, gpointer marshal_data);
static GObject * samplecat_model_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void samplecat_model_finalize (GObject* obj);


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


SamplecatModel* samplecat_model_construct (GType object_type) {
	SamplecatModel * self = NULL;
	const gchar* _tmp0_ = NULL;
	gchar* _tmp1_ = NULL;
	self = (SamplecatModel*) g_object_new (object_type, NULL);
	self->state = 1;
	_tmp0_ = g_get_home_dir ();
	_tmp1_ = g_build_filename (_tmp0_, ".config", PACKAGE, "cache", NULL, NULL);
	self->cache_dir = _tmp1_;
	return self;
}


SamplecatModel* samplecat_model_new (void) {
	return samplecat_model_construct (SAMPLECAT_TYPE_MODEL);
}


void samplecat_model_set_search_dir (SamplecatModel* self, gchar* dir) {
	g_return_if_fail (self != NULL);
	(*(*app).model).filters.dir = dir;
}


static void g_cclosure_user_marshal_VOID__POINTER_INT_POINTER (GClosure * closure, GValue * return_value, guint n_param_values, const GValue * param_values, gpointer invocation_hint, gpointer marshal_data) {
	typedef void (*GMarshalFunc_VOID__POINTER_INT_POINTER) (gpointer data1, gpointer arg_1, gint arg_2, gpointer arg_3, gpointer data2);
	register GMarshalFunc_VOID__POINTER_INT_POINTER callback;
	register GCClosure * cc;
	register gpointer data1, data2;
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
	SamplecatModel * self;
	parent_class = G_OBJECT_CLASS (samplecat_model_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = SAMPLECAT_MODEL (obj);
	return obj;
}


static void samplecat_model_class_init (SamplecatModelClass * klass) {
	samplecat_model_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->constructor = samplecat_model_constructor;
	G_OBJECT_CLASS (klass)->finalize = samplecat_model_finalize;
	g_signal_new ("sample_changed", SAMPLECAT_TYPE_MODEL, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_user_marshal_VOID__POINTER_INT_POINTER, G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_INT, G_TYPE_POINTER);
}


static void samplecat_model_instance_init (SamplecatModel * self) {
	self->state = 0;
}


static void samplecat_model_finalize (GObject* obj) {
	SamplecatModel * self;
	self = SAMPLECAT_MODEL (obj);
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



