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

#ifndef __model_h__
#define __model_h__

#include <glib.h>
#include <glib-object.h>
#include <samplecat/observable.h>
#include <samplecat/sample.h>
#include <samplecat/db/db.h>

G_BEGIN_DECLS

#define SAMPLECAT_TYPE_MODEL (samplecat_model_get_type ())
#define SAMPLECAT_MODEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_MODEL, SamplecatModel))
#define SAMPLECAT_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_MODEL, SamplecatModelClass))
#define SAMPLECAT_IS_MODEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLECAT_TYPE_MODEL))
#define SAMPLECAT_IS_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_MODEL))
#define SAMPLECAT_MODEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_MODEL, SamplecatModelClass))

typedef struct _SamplecatModel SamplecatModel;
typedef struct _SamplecatModelClass SamplecatModelClass;
typedef struct _SamplecatModelPrivate SamplecatModelPrivate;

typedef enum {
    FILTER_SEARCH,
    FILTER_DIR,
    FILTER_CATEGORY,
    N_FILTERS
} FilterTypes;

struct _SamplecatModel {
    GObject                parent_instance;
    SamplecatModelPrivate* priv;
    gint                   state;
    gchar*                 cache_dir;
    GList*                 backends;
    SamplecatBackend       backend;

    union {
        Observable* filters3[N_FILTERS];
        struct {
            Observable* search;
            Observable* dir;
            Observable* category;
        }                  filters2;
    };

    GNode*                 dir_tree;
    Sample*                selection;
    GList*                 modified;
};

struct _SamplecatModelClass {
    GObjectClass           parent_class;
};


GType             samplecat_model_get_type       () G_GNUC_CONST;
SamplecatModel*   samplecat_model_new            ();
SamplecatModel*   samplecat_model_construct      (GType);
gboolean          samplecat_model_add            (SamplecatModel*);
gboolean          samplecat_model_remove         (SamplecatModel*, gint id);
void              samplecat_model_set_search_dir (SamplecatModel*, gchar* dir);
void              samplecat_model_set_selection  (SamplecatModel*, Sample*);
void              samplecat_model_refresh_sample (SamplecatModel*, Sample*, gboolean force_update);
bool              samplecat_model_update_sample  (SamplecatModel*, Sample*, gint prop, void* val);
gchar*            samplecat_model_print_col_name (guint prop_type);
void              samplecat_model_move_files     (GList*, const gchar* dest_path);

#define samplecat_model_add_backend(A) samplecat.model->backends = g_list_append(samplecat.model->backends, A)

G_END_DECLS

#endif
