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


#ifndef __LIST_STORE_H__
#define __LIST_STORE_H__

#include <glib.h>
#include <glib-object.h>
#include <sample.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define SAMPLECAT_TYPE_FILTER (samplecat_filter_get_type ())
#define SAMPLECAT_FILTER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_FILTER, SamplecatFilter))
#define SAMPLECAT_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_FILTER, SamplecatFilterClass))
#define SAMPLECAT_IS_FILTER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLECAT_TYPE_FILTER))
#define SAMPLECAT_IS_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_FILTER))
#define SAMPLECAT_FILTER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_FILTER, SamplecatFilterClass))

typedef struct _SamplecatFilter SamplecatFilter;
typedef struct _SamplecatFilterClass SamplecatFilterClass;
typedef struct _SamplecatFilterPrivate SamplecatFilterPrivate;

#define SAMPLECAT_TYPE_FILTERS (samplecat_filters_get_type ())
typedef struct _SamplecatFilters SamplecatFilters;

#define SAMPLECAT_TYPE_MODEL (samplecat_model_get_type ())
#define SAMPLECAT_MODEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_MODEL, SamplecatModel))
#define SAMPLECAT_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_MODEL, SamplecatModelClass))
#define SAMPLECAT_IS_MODEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLECAT_TYPE_MODEL))
#define SAMPLECAT_IS_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_MODEL))
#define SAMPLECAT_MODEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_MODEL, SamplecatModelClass))

typedef struct _SamplecatModel SamplecatModel;
typedef struct _SamplecatModelClass SamplecatModelClass;
typedef struct _SamplecatModelPrivate SamplecatModelPrivate;

#define SAMPLECAT_TYPE_LIST_STORE (samplecat_list_store_get_type ())
#define SAMPLECAT_LIST_STORE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_LIST_STORE, SamplecatListStore))
#define SAMPLECAT_LIST_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_LIST_STORE, SamplecatListStoreClass))
#define SAMPLECAT_IS_LIST_STORE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLECAT_TYPE_LIST_STORE))
#define SAMPLECAT_IS_LIST_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_LIST_STORE))
#define SAMPLECAT_LIST_STORE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_LIST_STORE, SamplecatListStoreClass))

#define LIST_STORE_MAX_ROWS 1000

typedef struct _SamplecatListStore SamplecatListStore;
typedef struct _SamplecatListStoreClass SamplecatListStoreClass;
typedef struct _SamplecatListStorePrivate SamplecatListStorePrivate;

struct _SamplecatListStore {
	GtkListStore               parent_instance;
	SamplecatListStorePrivate* priv;
	gint                       row_count;
	GtkTreeRowReference*       playing;
};

struct _SamplecatListStoreClass {
	GtkListStoreClass          parent_class;
};

enum
{
   COL_ICON = 0,
#ifdef USE_AYYI
   COL_AYYI_ICON,
#endif
   COL_IDX,
   COL_NAME,
   COL_FNAME,
   COL_KEYWORDS,
   COL_OVERVIEW,
   COL_LENGTH,
   COL_SAMPLERATE,
   COL_CHANNELS,
   COL_MIMETYPE,
   COL_PEAKLEVEL,

   //from here on items are the store but not the view.
   COL_COLOUR,
   COL_SAMPLEPTR,
   COL_LEN,
   NUM_COLS, ///< end of columns in the store

   // these are NOT in the store but in the sample-struct (COL_SAMPLEPTR)
   COL_X_EBUR,
   COL_X_NOTES,
   COL_ALL
};


GParamSpec*      samplecat_param_spec_filter                  (const gchar* name, const gchar* nick, const gchar* blurb, GType, GParamFlags);
void             samplecat_value_set_filter                   (GValue*, gpointer v_object);
void             samplecat_value_take_filter                  (GValue*, gpointer v_object);
gpointer         samplecat_value_get_filter                   (const GValue*);

GType            samplecat_filter_get_type                    (void) G_GNUC_CONST;
SamplecatFilter* samplecat_filter_new                         (gchar* _name);
SamplecatFilter* samplecat_filter_construct                   (GType, gchar* _name);
void             samplecat_filter_set_value                   (SamplecatFilter*, gchar* val);
gpointer         samplecat_filter_ref                         (gpointer instance);
void             samplecat_filter_unref                       (gpointer instance);

GType            samplecat_filters_get_type                   (void) G_GNUC_CONST;

GType            samplecat_model_get_type                     (void) G_GNUC_CONST;
SamplecatModel*  samplecat_model_new                          (void);
SamplecatModel*  samplecat_model_construct                    (GType);
void             samplecat_model_set_search_dir               (SamplecatModel*, gchar* dir);
void             samplecat_model_set_selection                (SamplecatModel*, Sample*);
void             samplecat_model_add_filter                   (SamplecatModel*, SamplecatFilter*);

GType               samplecat_list_store_get_type             (void) G_GNUC_CONST;
SamplecatListStore* samplecat_list_store_new                  (void);
SamplecatListStore* samplecat_list_store_construct            (GType);
void             samplecat_list_store_clear_                  (SamplecatListStore*);
void             samplecat_list_store_add                     (SamplecatListStore*, Sample*);
void             samplecat_list_store_on_sample_changed       (SamplecatListStore*, Sample*, gint prop, void* val);
void             samplecat_list_store_do_search               (SamplecatListStore*);

Sample*          samplecat_list_store_get_sample_by_iter      (GtkTreeIter*);
Sample*          samplecat_list_store_get_sample_by_row_index (int);
Sample*          samplecat_list_store_get_sample_by_row_ref   (GtkTreeRowReference*);


G_END_DECLS

#endif
