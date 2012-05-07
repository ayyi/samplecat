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

#ifndef __sample_list_model_h__
#define __sample_list_model_h__

#include <clutter/clutter.h>
#include "sample.h"

#define SAMPLE_TYPE_LIST_MODEL (sample_list_model_get_type ())
#define SAMPLE_LIST_MODEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLE_TYPE_LIST_MODEL, SampleListModel))
#define SAMPLE_LIST_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLE_TYPE_LIST_MODEL, SampleListModelClass))
#define SAMPLE_IS_LIST_MODEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLE_TYPE_LIST_MODEL))
#define SAMPLE_IS_LIST_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLE_TYPE_LIST_MODEL))
#define SAMPLE_LIST_MODEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLE_TYPE_LIST_MODEL, SampleListModelClass))

typedef struct _SampleListModel SampleListModel;
typedef struct _SampleListModelClass SampleListModelClass;

struct _SampleListModel
{
	ClutterListModel parent;
};

struct _SampleListModelClass
{
	ClutterListModelClass parent_class;
};

enum {
	SAMPLE_LIST_MODEL_TYPE_SOURCE = 0,
	SAMPLE_LIST_MODEL_TYPE_MEDIA_BOX,
	SAMPLE_LIST_MODEL_TYPE_MEDIA
};

enum {
	SAMPLE_LIST_MODEL_COLUMN_ID = 0,
	SAMPLE_LIST_MODEL_COLUMN_NAME,
	SAMPLE_LIST_MODEL_COLUMN_LENGTH,
	SAMPLE_LIST_MODEL_COLUMN_TYPE,
	SAMPLE_LIST_MODEL_COLUMN_SAMPLERATE,
	SAMPLE_LIST_MODEL_COLUMN_INSTANCE,
	SAMPLE_LIST_MODEL_N_COLUMNS
};



GType           sample_list_model_get_type   () G_GNUC_CONST;
ClutterModel*   sample_list_model_new        (SamplecatModel*);
void            sample_list_model_add_item   (SampleListModel*, Sample*);
GObject*        sample_list_model_get_item   (SampleListModel*, const gchar* id);

#endif
