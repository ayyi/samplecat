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
#include "debug/debug.h"
#include "list_item.h"
#include "utils/ayyi_utils.h"

enum {
	PROP_0,
	PROP_ID,
	PROP_SAMPLENAME,
	PROP_SAMPLERATE,
	PROP_LENGTH,
	PROP_LIST_VIEW
};

#define SAMPLECAT_MX_LIST_ITEM_GET_PRIVATE(obj)    \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), SAMPLECAT_TYPE_MX_LIST_ITEM, SamplecatMxListItemPrivate))

typedef struct _SamplecatMxListItemPrivate SamplecatMxListItemPrivate;
struct _SamplecatMxListItemPrivate {
	gchar*        id;
	gchar*        sample_name;
	int           samplerate;
	int           length;
	ClutterActor* list_view;

	MxLabel*      label0;
	MxLabel*      label1;
	MxLabel*      label2;
};

G_DEFINE_TYPE (SamplecatMxListItem, samplecat_mx_list_item, MX_TYPE_BOX_LAYOUT);

GList* selected_rows = NULL;


static void
set_id (SamplecatMxListItem* self, const gchar* id)
{
	SamplecatMxListItemPrivate* priv = SAMPLECAT_MX_LIST_ITEM_GET_PRIVATE (self);

	if (priv->id) {
		g_free (priv->id);
	}

	priv->id = g_strdup (id);
	g_object_notify (G_OBJECT (self), "id");
}


static const gchar*
get_id (SamplecatMxListItem* self)
{
	SamplecatMxListItemPrivate* priv = SAMPLECAT_MX_LIST_ITEM_GET_PRIVATE (self);

	return priv->id;
}


static void
set_sample_name (SamplecatMxListItem* self, const char* name)
{
	SamplecatMxListItemPrivate* priv = SAMPLECAT_MX_LIST_ITEM_GET_PRIVATE (self);

	priv->sample_name = (char*)name;
	g_object_notify (G_OBJECT (self), "sample_name");

	gchar* sr = g_strdup(priv->sample_name);
	mx_label_set_text(priv->label0, sr);
	g_free(sr);
}


static void
set_samplerate (SamplecatMxListItem* self, int samplerate)
{
	SamplecatMxListItemPrivate* priv = SAMPLECAT_MX_LIST_ITEM_GET_PRIVATE (self);

	priv->samplerate = samplerate;
	g_object_notify (G_OBJECT (self), "samplerate");

	gchar* sr = g_strdup_printf("%i", priv->samplerate);
	mx_label_set_text(priv->label1, sr);
	g_free(sr);
}


static void
set_length (SamplecatMxListItem* self, int length)
{
	SamplecatMxListItemPrivate* priv = SAMPLECAT_MX_LIST_ITEM_GET_PRIVATE (self);

	priv->length = length;
	g_object_notify (G_OBJECT (self), "length");

	gchar* sr = g_strdup_printf("%i", priv->length);
	mx_label_set_text(priv->label2, sr);
	g_free(sr);
}


static void
set_list_view (SamplecatMxListItem* self, ClutterActor* list_view)
{
	SamplecatMxListItemPrivate* priv = SAMPLECAT_MX_LIST_ITEM_GET_PRIVATE (self);

	if (priv->list_view) {
		priv->list_view = NULL;
	}

	priv->list_view = list_view ? g_object_ref (list_view) : NULL;
	
	g_object_notify (G_OBJECT (self), "list-view");
}


static void
samplecat_mx_list_item_set_property (GObject* gobject, guint prop_id, const GValue* value, GParamSpec* pspec)
{
	SamplecatMxListItem* self = SAMPLECAT_MX_LIST_ITEM (gobject);

	switch (prop_id) {
	case PROP_ID:
		set_id (self, g_value_get_string (value));
		break;
	case PROP_SAMPLENAME:
		set_sample_name (self, g_value_get_string (value));
		break;
	case PROP_SAMPLERATE:
		set_samplerate (self, g_value_get_int (value));
		break;
	case PROP_LENGTH:
		set_length (self, g_value_get_int (value));
		break;
	case PROP_LIST_VIEW:
		set_list_view (self, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
		break;
	}
}


static void
samplecat_mx_list_item_get_property (GObject* gobject, guint prop_id, GValue* value, GParamSpec* pspec)
{
	SamplecatMxListItem* self = SAMPLECAT_MX_LIST_ITEM (gobject);
	SamplecatMxListItemPrivate* priv = SAMPLECAT_MX_LIST_ITEM_GET_PRIVATE (self);

	switch (prop_id) {
	case PROP_ID:
		g_value_set_string (value, get_id (self));
		break;
	case PROP_SAMPLENAME:
		g_value_set_string (value, priv->sample_name);
		break;
	case PROP_SAMPLERATE:
		g_value_set_int (value, priv->samplerate);
		break;
	case PROP_LENGTH:
		g_value_set_int (value, priv->length);
		break;
	case PROP_LIST_VIEW:
		g_value_set_object (value, priv->list_view);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
		break;
	}
}


static void
samplecat_mx_list_item_dispose (GObject *self)
{
	SamplecatMxListItemPrivate* priv = SAMPLECAT_MX_LIST_ITEM_GET_PRIVATE (self);

	if (priv->list_view) {
		g_object_unref (priv->list_view);
		priv->list_view = NULL;
	}

	G_OBJECT_CLASS (samplecat_mx_list_item_parent_class)->dispose (G_OBJECT (self));
}


static void
samplecat_mx_list_item_finalize (GObject *self)
{
	SamplecatMxListItemPrivate* priv = SAMPLECAT_MX_LIST_ITEM_GET_PRIVATE (self);

	g_free (priv->id);

	G_OBJECT_CLASS (samplecat_mx_list_item_parent_class)->finalize (G_OBJECT (self));
}


static void
samplecat_mx_list_item_class_init (SamplecatMxListItemClass* klass)
{
	GParamSpec* pspec;
	GObjectClass* gobject_class = (GObjectClass*) klass;

	gobject_class->dispose = samplecat_mx_list_item_dispose;
	gobject_class->finalize = samplecat_mx_list_item_finalize;
	gobject_class->set_property = samplecat_mx_list_item_set_property;
	gobject_class->get_property = samplecat_mx_list_item_get_property;

	pspec = g_param_spec_string ("id",
				     "Id",
				     "Id of the media plugin",
				     NULL, G_PARAM_READABLE | G_PARAM_WRITABLE);
	g_object_class_install_property (gobject_class, PROP_ID, pspec);

	pspec = g_param_spec_string ("sample_name",
				     "Sample Name",
				     "The name of the sample file",
				     NULL, G_PARAM_READABLE | G_PARAM_WRITABLE);
	g_object_class_install_property (gobject_class, PROP_SAMPLENAME, pspec);

	pspec = g_param_spec_int ("samplerate",
				     "Samplerate",
				     "Samplerate in Hz",
				     0, 192000, 44100,
				     G_PARAM_READABLE | G_PARAM_WRITABLE);
	g_object_class_install_property (gobject_class, PROP_SAMPLERATE, pspec);

	pspec = g_param_spec_int ("length",
				     "Length",
				     "Length in ms probably",
				     0, 1000000, 1000,
				     G_PARAM_READABLE | G_PARAM_WRITABLE);
	g_object_class_install_property (gobject_class, PROP_LENGTH, pspec);

	pspec = g_param_spec_object ("list-view",
				     "List view",
				     "List view the item belongs to",
				     MX_TYPE_LIST_VIEW,
				     G_PARAM_READABLE | G_PARAM_WRITABLE);
	g_object_class_install_property (gobject_class, PROP_LIST_VIEW, pspec);

	g_type_class_add_private (gobject_class, sizeof(SamplecatMxListItemPrivate));
}


static void
samplecat_mx_list_item_init (SamplecatMxListItem* self)
{
	SamplecatMxListItemPrivate* priv = SAMPLECAT_MX_LIST_ITEM_GET_PRIVATE (self);

	mx_box_layout_set_spacing((MxBoxLayout*)self, 10);
	clutter_actor_set_name((ClutterActor*)self, "row");
	dbg(0, "name=%s", clutter_actor_get_name((ClutterActor*)self));

#if 0
	void on_clicked (MxButton* button, SamplecatMxListItem* self)
	{
		SamplecatMxListItemPrivate* priv = SAMPLECAT_MX_LIST_ITEM_GET_PRIVATE (self);

		if (priv->list_view) {
			g_signal_emit_by_name (G_OBJECT (priv->list_view), "activated", priv->id);
		}
	}

	ClutterActor* button = mx_button_new_with_label("Hello");
	mx_box_layout_add_actor((MxBoxLayout*)self, (ClutterActor*)button, 0);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (on_clicked), self);
#else
	priv->label0 = (MxLabel*)mx_label_new();
	clutter_actor_set_width((ClutterActor*)priv->label0, 200.0);
	mx_box_layout_add_actor((MxBoxLayout*)self, (ClutterActor*)priv->label0, 0);
	gboolean on_button_press(ClutterActor* actor, ClutterEvent* event, gpointer user_data)
	{
		//dbg(0, "button-press");
		return TRUE;
	}
	gboolean on_button_release(ClutterActor* actor, ClutterEvent* event, gpointer user_data)
	{
		dbg(0, "button-release");
		/*
		clutter_actor_set_name(actor, "row0");

		guint n_props = 0;
		GParamSpec** ps = mx_stylable_list_properties(MX_STYLABLE(actor), &n_props);
		if(ps){
			int n; for(n=0;n<n_props;n++){
				GParamSpec* p = ps[n];
				dbg(0, "  %s", p->name);
			}
			g_free(ps);
		}

		ClutterColor* bg_color;
		mx_stylable_get(MX_STYLABLE(actor), "bg-color", &bg_color, NULL);
		*/

		GList* l = selected_rows;
		if(l){
			for(;l;l=l->next){
				mx_stylable_set_style_pseudo_class(MX_STYLABLE(l->data), "");
			}
			g_list_free(selected_rows);
		}

		mx_stylable_set_style_pseudo_class(MX_STYLABLE(actor), "selected");
		selected_rows = g_list_prepend(NULL, actor);

		mx_stylable_style_changed(MX_STYLABLE(actor), MX_STYLE_CHANGED_FORCE);
		return TRUE;
	}
	/*
	void on_allocation_changed(ClutterActor* actor, ClutterActorBox* box, ClutterAllocationFlags flags, gpointer user_data)
	{
		dbg(0, "allocation-changed");
	}
	g_signal_connect (self, "allocation-changed", G_CALLBACK(on_allocation_changed), NULL);
	*/
	//clutter_actor_set_reactive((ClutterActor*)priv->label0, TRUE);
	clutter_actor_set_reactive((ClutterActor*)self, TRUE);
	//g_signal_connect (priv->label0, "button-release-event", G_CALLBACK(on_button_release), NULL);
	g_signal_connect (self, "button-release-event", G_CALLBACK(on_button_release), NULL);
	g_signal_connect (priv->label0, "button-press-event", G_CALLBACK(on_button_press), NULL);
#endif

	priv->label1 = (MxLabel*)mx_label_new();
	mx_box_layout_add_actor((MxBoxLayout*)self, (ClutterActor*)priv->label1, 1);

	priv->label2 = (MxLabel*)mx_label_new();
	mx_box_layout_add_actor((MxBoxLayout*)self, (ClutterActor*)priv->label2, 2);
}

