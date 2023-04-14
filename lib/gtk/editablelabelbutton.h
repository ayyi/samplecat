/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2008-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_EDITABLE_LABEL_BUTTON            (editable_label_button_get_type ())
#define EDITABLE_LABEL_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_EDITABLE_LABEL_BUTTON, EditableLabelButton))
#define EDITABLE_LABEL_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_EDITABLE_LABEL_BUTTON, EditableLabelButtonClass))
#define IS_EDITABLE_LABEL_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_EDITABLE_LABEL_BUTTON))
#define IS_EDITABLE_LABEL_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_EDITABLE_LABEL_BUTTON))
#define EDITABLE_LABEL_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_EDITABLE_LABEL_BUTTON, EditableLabelButtonClass))

typedef struct _EditableLabelButton EditableLabelButton;
typedef struct _EditableLabelButtonClass EditableLabelButtonClass;
typedef struct _EditableLabelButtonPrivate EditableLabelButtonPrivate;

struct _EditableLabelButton {
#ifdef GTK4_TODO
	GtkEventBox                 parent_instance;
#else
	GtkWidget                   parent;
#endif
	GtkLabel*                   label;
	EditableLabelButtonPrivate* priv;
};

struct _EditableLabelButtonClass {
#ifdef GTK4_TODO
	GtkEventBoxClass parent_class;
#else
	GtkWidgetClass   parent_class;
#endif
};


GType                editable_label_button_get_type   () G_GNUC_CONST;
EditableLabelButton* editable_label_button_new        (const char*);
EditableLabelButton* editable_label_button_construct  (GType);
void                 editable_label_button_start_edit (EditableLabelButton*);


G_END_DECLS
