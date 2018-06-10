/*
  This file is part of the Ayyi Project. http://www.ayyi.org
  copyright (C) 2004-2017 Tim Orford <tim@orford.org>

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
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "typedefs.h"
#include "debug/debug.h"
#include "editablelabelbutton.h"


#define TYPE_EDITABLE_LABEL_BUTTON (editable_label_button_get_type ())
#define EDITABLE_LABEL_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_EDITABLE_LABEL_BUTTON, EditableLabelButton))
#define EDITABLE_LABEL_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_EDITABLE_LABEL_BUTTON, EditableLabelButtonClass))
#define IS_EDITABLE_LABEL_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_EDITABLE_LABEL_BUTTON))
#define IS_EDITABLE_LABEL_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_EDITABLE_LABEL_BUTTON))
#define EDITABLE_LABEL_BUTTON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_EDITABLE_LABEL_BUTTON, EditableLabelButtonClass))

static void editable_label_button_finalize  (GObject*);
static bool editable_label_button_stop_edit (GtkWidget*, GdkEventCrossing*, gpointer);

struct _EditableLabelButtonPrivate {
	GtkWidget* align;
};

static GtkWidget* w_rename = NULL;


static gpointer editable_label_button_parent_class = NULL;

GType editable_label_button_get_type (void) G_GNUC_CONST;
#define EDITABLE_LABEL_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_EDITABLE_LABEL_BUTTON, EditableLabelButtonPrivate))
enum  {
	EDITABLE_LABEL_BUTTON_DUMMY_PROPERTY
};


static void
accels_connect(GtkWidget* widget)
{
	// TODO
}


static void
accels_disconnect(GtkWidget* widget)
{
	// TODO
}


EditableLabelButton*
editable_label_button_construct (GType object_type)
{
	EditableLabelButton* self = (EditableLabelButton*) g_object_new (object_type, NULL);
	return self;
}


static bool
editable_label_on_buttonpress (GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
	if(event->type == GDK_BUTTON_PRESS){
		editable_label_button_start_edit((EditableLabelButton*)widget);
		return TRUE;
	}

	return FALSE;
}


EditableLabelButton*
editable_label_button_new (const char* name)
{
	EditableLabelButton* self = editable_label_button_construct (TYPE_EDITABLE_LABEL_BUTTON);

	GtkWidget* left_align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	self->priv->align = left_align;

	self->label = (GtkLabel*)gtk_label_new(name);
	gtk_misc_set_padding(GTK_MISC(self->label), 5, 0);

	gtk_container_add(GTK_CONTAINER(left_align), (GtkWidget*)self->label);
	gtk_container_add(GTK_CONTAINER(self), left_align);

	g_signal_connect(self, "button-press-event", G_CALLBACK(editable_label_on_buttonpress), NULL);

	return self;
}


static void
editable_label_button_class_init (EditableLabelButtonClass* klass)
{
	editable_label_button_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (EditableLabelButtonPrivate));
	G_OBJECT_CLASS (klass)->finalize = editable_label_button_finalize;
	g_signal_new ("edited", TYPE_EDITABLE_LABEL_BUTTON, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);

	w_rename = gtk_entry_new ();
	gtk_entry_set_max_length(GTK_ENTRY(w_rename), 64);
	g_object_ref(w_rename); // prevent automatic freeing.
	gtk_widget_show(w_rename);
}


static void
editable_label_button_instance_init (EditableLabelButton* self)
{
	self->priv = EDITABLE_LABEL_BUTTON_GET_PRIVATE (self);
	self->priv->align = NULL;
	self->label = NULL;
}


static void
editable_label_button_finalize (GObject* obj)
{
	G_OBJECT_CLASS (editable_label_button_parent_class)->finalize (obj);
}


GType
editable_label_button_get_type ()
{
	static volatile gsize editable_label_button_type_id__volatile = 0;
	if (g_once_init_enter (&editable_label_button_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (EditableLabelButtonClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) editable_label_button_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (EditableLabelButton), 0, (GInstanceInitFunc) editable_label_button_instance_init, NULL };
		GType editable_label_button_type_id;
		editable_label_button_type_id = g_type_register_static (GTK_TYPE_EVENT_BOX, "EditableLabelButton", &g_define_type_info, 0);
		g_once_init_leave (&editable_label_button_type_id__volatile, editable_label_button_type_id);
	}
	return editable_label_button_type_id__volatile;
}


struct _editor
{
	char orig_text[256]; //for comparison, to detect changes.
} editor;

static gulong focus_handler_id = 0;
static bool need_abort = false;


static bool
stop (gpointer widget)
{
	g_signal_emit_by_name (G_OBJECT(widget), "focus-out-event");
	return G_SOURCE_REMOVE;
}


/*
 *  Trap the RET and ESC keys and stop editing.
 */
static bool
track_entry_key_press (GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
	if(event->type == GDK_KEY_PRESS){
		switch(event->keyval){
			case GDK_Escape:
				need_abort = true;
			case 0xFF0D/*GDK_KP_Return*/:
				g_idle_add(stop, widget);
				return true;
		}
	}
	return false;
}


/*
 *  Initiate the inplace renaming of a label following a double-click.
 *   - swap the non-editable gtklabel with the editable gtkentry widget.
 *   - w_rename has had an extra ref added at creation time, so we dont have to do it here.
 */
void
editable_label_button_start_edit (EditableLabelButton* self)
{
	PF;
	EditableLabelButtonPrivate* _self = self->priv;

	static gulong handler2 = 0;

	accels_disconnect((GtkWidget*)self);

	//show the rename widget
	gtk_entry_set_text(GTK_ENTRY(w_rename), gtk_label_get_text(GTK_LABEL(self->label)));
	g_object_ref(self->label); // prevent the widget being destroyed when unparented.
	gtk_container_remove(GTK_CONTAINER(_self->align), (GtkWidget*)self->label);
	gtk_container_add   (GTK_CONTAINER(_self->align), w_rename);

	g_strlcpy(editor.orig_text, gtk_label_get_text(GTK_LABEL(self->label)), 256);

	// focus-out can be improved by properly focussing other widgets when clicked on.

	//add signal to toplevel window? it doesnt like this:
	//g_signal_connect ((gpointer)gtk_widget_get_toplevel(w_rename), "clicked", G_CALLBACK(track_label_edit_stop), GINT_TO_POINTER(tnum));

	//if(focus_handler_id) g_signal_handler_disconnect((gpointer)w_rename, focus_handler_id);

	// this signal wont quit until you click on another button, or other widget which takes kb focus.
	// "focus" refers to the keyboard. So, eg, TAB will cause it to quit.
#ifndef DEBUG_TRACKCONTROL
	focus_handler_id = g_signal_connect ((gpointer)w_rename, "focus-out-event", G_CALLBACK(editable_label_button_stop_edit), self);
#endif

	if(handler2) g_signal_handler_disconnect((gpointer)w_rename, handler2);
	handler2 = g_signal_connect(G_OBJECT(w_rename), "key-press-event", G_CALLBACK(track_entry_key_press), NULL); // trap the RET, ESC key.

	// this is neccesary as it allows us to start typing imediately. It also selects the whole string.
	gtk_widget_grab_focus(GTK_WIDGET(w_rename));
}


static gboolean
editable_label_button_stop_edit (GtkWidget* entry, GdkEventCrossing* event, gpointer user_data)
{
	EditableLabelButton* self = user_data;
	g_return_val_if_fail(self, FALSE);

	if(g_signal_handler_is_connected ((gpointer)w_rename, focus_handler_id)){
		g_signal_handler_disconnect((gpointer)w_rename, focus_handler_id);
	}
	else gwarn("focus handler not connected.");

	accels_connect((GtkWidget*)self);

	const char* new_text = gtk_entry_get_text(GTK_ENTRY(w_rename));
	if(!need_abort){
		gtk_label_set_text(GTK_LABEL(self->label), new_text); // change the text in the label
	}

	// swap back to the normal label
	gtk_container_remove(GTK_CONTAINER(self->priv->align), w_rename);

	gtk_container_add(GTK_CONTAINER(self->priv->align), (GtkWidget*)self->label);
	g_object_unref(self->label); // remove temporary ref added in edit_start.

	if(!need_abort){
		g_signal_emit_by_name(self, "edited", new_text);
	}

	need_abort = false;

	return false;
}


