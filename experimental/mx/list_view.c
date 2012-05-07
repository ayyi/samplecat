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

#include "config.h"
#include "utils/ayyi_utils.h"
#include "list_item.h"
#include "list_view.h"
#include "marshal.h"

enum
{
  ACTIVATED,
  LAST_SIGNAL
};

static guint list_view_signals[LAST_SIGNAL] = { 0, };

static void          item_factory_init     (gpointer g_iface, gpointer iface_data);
static ClutterActor* list_view_create_item (MxItemFactory*);

G_DEFINE_TYPE_WITH_CODE (SamplecatMxListView, list_view, MX_TYPE_LIST_VIEW, G_IMPLEMENT_INTERFACE(MX_TYPE_ITEM_FACTORY, item_factory_init));


static void
item_factory_init (gpointer g_iface, gpointer iface_data)
{
	dbg(2, "...");

	MxItemFactoryIface* iface = (MxItemFactoryIface*)g_iface;

	iface->create = list_view_create_item;
}


static void
list_view_class_init (SamplecatMxListViewClass *klass)
{
	list_view_signals[ACTIVATED] =
		g_signal_new ("activated",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (SamplecatMxListViewClass, activated),
			      NULL, NULL,
			      _samplecat_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
}


static void
list_view_init (SamplecatMxListView *self)
{
	mx_list_view_set_factory   (MX_LIST_VIEW(self), MX_ITEM_FACTORY (self));

	mx_list_view_add_attribute (MX_LIST_VIEW(self), "id",         SAMPLE_LIST_MODEL_COLUMN_ID);
	mx_list_view_add_attribute (MX_LIST_VIEW(self), "sample_name",SAMPLE_LIST_MODEL_COLUMN_NAME);
	mx_list_view_add_attribute (MX_LIST_VIEW(self), "samplerate", SAMPLE_LIST_MODEL_COLUMN_SAMPLERATE);
	mx_list_view_add_attribute (MX_LIST_VIEW(self), "length",     SAMPLE_LIST_MODEL_COLUMN_LENGTH);
}


ClutterActor*
list_view_new (SampleListModel* model)
{
	return g_object_new (SAMPLECAT_TYPE_MX_LIST_VIEW, "model", model, NULL);
}


static ClutterActor *
list_view_create_item (MxItemFactory* factory)
{
	dbg(2, "");
	return g_object_new (SAMPLECAT_TYPE_MX_LIST_ITEM, "list-view", factory, NULL);
}

