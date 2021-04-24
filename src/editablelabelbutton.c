/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2004-2021 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <glib.h>
#include <glib-object.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include <gdk/gdkkeysyms.h>
#include "typedefs.h"
#include "debug/debug.h"
#include "editablelabelbutton.h"

static void editable_label_button_finalize  (GObject*);
static bool editable_label_button_stop_edit (GtkWidget*, GdkEventCrossing*, gpointer);

struct _EditableLabelButtonPrivate {
	GtkWidget* align;
};

static GtkWidget* w_rename = NULL;

GType editable_label_button_get_type (void) G_GNUC_CONST;
G_DEFINE_TYPE_WITH_PRIVATE (EditableLabelButton, editable_label_button, GTK_TYPE_EVENT_BOX)
enum  {
	EDITABLE_LABEL_BUTTON_DUMMY_PROPERTY
};


static void
accels_connect (GtkWidget* widget)
{
	// TODO
}


static void
accels_disconnect (GtkWidget* widget)
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
	G_OBJECT_CLASS (klass)->finalize = editable_label_button_finalize;
	g_signal_new ("edited", TYPE_EDITABLE_LABEL_BUTTON, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);

	w_rename = gtk_entry_new ();
	gtk_entry_set_max_length(GTK_ENTRY(w_rename), 64);
	g_object_ref(w_rename); // prevent automatic freeing.
	gtk_widget_show(w_rename);
}


static void
editable_label_button_init (EditableLabelButton* self)
{
	self->priv = editable_label_button_get_instance_private(self);
	self->priv->align = NULL;
	self->label = NULL;
}


static void
editable_label_button_finalize (GObject* obj)
{
	G_OBJECT_CLASS (editable_label_button_parent_class)->finalize (obj);
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

	// Show the rename widget
	gtk_entry_set_text(GTK_ENTRY(w_rename), gtk_label_get_text(GTK_LABEL(self->label)));
	g_object_ref(self->label); // prevent the widget being destroyed when unparented.
	gtk_container_remove(GTK_CONTAINER(_self->align), (GtkWidget*)self->label);
	gtk_container_add   (GTK_CONTAINER(_self->align), w_rename);

	g_strlcpy(editor.orig_text, gtk_label_get_text(GTK_LABEL(self->label)), 256);

	// focus-out can be improved by properly focussing other widgets when clicked on.

	//if(focus_handler_id) g_signal_handler_disconnect((gpointer)w_rename, focus_handler_id);

	// this signal wont quit until you click on another button, or other widget which takes kb focus.
	// "focus" refers to the keyboard. So, eg, TAB will cause it to quit.
	focus_handler_id = g_signal_connect ((gpointer)w_rename, "focus-out-event", G_CALLBACK(editable_label_button_stop_edit), self);

	if(handler2) g_signal_handler_disconnect((gpointer)w_rename, handler2);
	handler2 = g_signal_connect(G_OBJECT(w_rename), "key-press-event", G_CALLBACK(track_entry_key_press), NULL); // trap the RET, ESC key.

	// Allow us to start typing imediately and select the whole string.
	gtk_widget_grab_focus(GTK_WIDGET(w_rename));
}


static bool
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
