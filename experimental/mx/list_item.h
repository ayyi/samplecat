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

#ifndef __samplecat_mx_list_item_h__
#define __samplecat_mx_list_item_h__

#include <mx/mx.h>

#define SAMPLECAT_TYPE_MX_LIST_ITEM (samplecat_mx_list_item_get_type ())
#define SAMPLECAT_MX_LIST_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_MX_LIST_ITEM, SamplecatMxListItem))
#define SAMPLECAT_MX_LIST_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_MX_LIST_ITEM, SamplecatMxListItemClass))
#define SAMPLECAT_IS_MX_LIST_ITEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTP_TYPE_PLUGINS_LIST_ITEM))
#define SAMPLECAT_IS_MX_LIST_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_MX_LIST_ITEM))
#define SAMPLECAT_MX_LIST_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_MX_LIST_ITEM, SamplecatMxListItemClass))

typedef struct _SamplecatMxListItem SamplecatMxListItem;
typedef struct _SamplecatMxListItemClass SamplecatMxListItemClass;

struct _SamplecatMxListItem
{
	MxBoxLayout parent;
};

struct _SamplecatMxListItemClass
{
	MxBoxLayoutClass parent_class;
};

GType samplecat_mx_list_item_get_type (void) G_GNUC_CONST;

#endif
