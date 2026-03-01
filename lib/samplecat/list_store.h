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
 | Implements G_TYPE_LIST_MODEL interface
 |
 */
#pragma once

#include <glib-object.h>
#include <samplecat/sample.h>
#include <samplecat/sample_row.h>

G_BEGIN_DECLS

#define SAMPLECAT_TYPE_LIST_STORE            (samplecat_list_store_get_type ())
#define SAMPLECAT_LIST_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_LIST_STORE, SamplecatListStore))
#define SAMPLECAT_IS_LIST_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLECAT_TYPE_LIST_STORE))
#define SAMPLECAT_LIST_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_LIST_STORE, SamplecatListStoreClass))
#define SAMPLECAT_IS_LIST_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_LIST_STORE))
#define SAMPLECAT_LIST_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_LIST_STORE, SamplecatListStoreClass))

#define LIST_STORE_MAX_ROWS 1000

typedef struct _SamplecatListStore       SamplecatListStore;
typedef struct _SamplecatListStoreClass  SamplecatListStoreClass;

struct _SamplecatListStore {
	GObject      parent_instance;

	GListStore*  rows;
	gint         row_count;
	SampleRow*   playing;
};

struct _SamplecatListStoreClass {
	GObjectClass parent_class;
};

GType               samplecat_list_store_get_type           (void) G_GNUC_CONST;
SamplecatListStore* samplecat_list_store_new                (void);

/* Operations */
void                samplecat_list_store_add                (Sample*);
void                samplecat_list_store_remove_at          (guint position);
void                samplecat_list_store_do_search          ();
void                samplecat_list_store_set_row_ref        (Sample*, SampleRow*);

/* Accessors */
SampleRow*          samplecat_list_store_get_row            (guint position); /* returns new ref */
Sample*             samplecat_list_store_get_sample_by_index(guint position); /* sample_ref()'d */
guint               samplecat_list_store_find_sample_index  (Sample*);

G_END_DECLS
