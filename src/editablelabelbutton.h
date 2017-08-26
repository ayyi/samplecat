/*
 * Copyright (C) 2008-2013 Tim Orford. Part of the Ayyi project. http://ayyi.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __editablelabelbutton_h__
#define __editablelabelbutton_h__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define TYPE_EDITABLE_LABEL_BUTTON (editable_label_button_get_type ())
#define EDITABLE_LABEL_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_EDITABLE_LABEL_BUTTON, EditableLabelButton))
#define EDITABLE_LABEL_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_EDITABLE_LABEL_BUTTON, EditableLabelButtonClass))
#define IS_EDITABLE_LABEL_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_EDITABLE_LABEL_BUTTON))
#define IS_EDITABLE_LABEL_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_EDITABLE_LABEL_BUTTON))
#define EDITABLE_LABEL_BUTTON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_EDITABLE_LABEL_BUTTON, EditableLabelButtonClass))

typedef struct _EditableLabelButton EditableLabelButton;
typedef struct _EditableLabelButtonClass EditableLabelButtonClass;
typedef struct _EditableLabelButtonPrivate EditableLabelButtonPrivate;

struct _EditableLabelButton {
	GtkEventBox                 parent_instance;
	GtkLabel*                   label;
	GtkWidget*                  wrename;             //text entry widget used for editing. Normally hidden.
	EditableLabelButtonPrivate* priv;
};

struct _EditableLabelButtonClass {
	GtkEventBoxClass parent_class;
};


GType                editable_label_button_get_type   () G_GNUC_CONST;
EditableLabelButton* editable_label_button_new        (const char*);
EditableLabelButton* editable_label_button_construct  (GType);
void                 editable_label_button_start_edit (EditableLabelButton*);


G_END_DECLS

#endif
