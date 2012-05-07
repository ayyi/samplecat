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

#ifndef __mx_list_view_h__
#define __mx_list_view_h__

#include <mx/mx.h>
#include <list_model.h>

#define SAMPLECAT_TYPE_MX_LIST_VIEW (list_view_get_type ())
#define SAMPLECAT_MX_LIST_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_MX_LIST_VIEW, SamplecatMxListView))
#define SAMPLECAT_MX_LIST_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_MX_LIST_VIEW, SamplecatMxListViewClass))
#define SAMPLECAT_IS_MX_LIST_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLECAT_TYPE_MX_LIST_VIEW))
#define SAMPLECAT_IS_MX_LIST_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_MX_LIST_VIEW))
#define SAMPLECAT_MX_LIST_VIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_MX_LIST_VIEW, SamplecatMxListViewClass))

typedef struct _SamplecatMxListView SamplecatMxListView;
typedef struct _SamplecatMxListViewClass SamplecatMxListViewClass;

struct _SamplecatMxListView
{
	MxListView parent;
};

struct _SamplecatMxListViewClass
{
	MxListViewClass parent_class;

	/* signals */
	void     (* activated)    (SamplecatMxListView*, const gchar* id);
};

GType         list_view_get_type () G_GNUC_CONST;
ClutterActor* list_view_new      (SampleListModel*);

#endif
