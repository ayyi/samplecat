/* GTK - The GIMP Toolkit
 * gtkcellrendererseptext.h: Cell renderer for text or a separator
 * Copyright (C) 2003, Ximian, Inc.
 *
 * Author: Federico Mena-Quintero <federico@ximian.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GTK_CELL_RENDERER_HYPER_TEXT_H__
#define __GTK_CELL_RENDERER_HYPER_TEXT_H__

#include <gtk/gtkcellrenderertext.h>

G_BEGIN_DECLS

#define GTK_TYPE_CELL_RENDERER_HYPER_TEXT            (_gtk_cell_renderer_hyper_text_get_type ())
#define GTK_CELL_RENDERER_HYPER_TEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_CELL_RENDERER_HYPER_TEXT, GtkCellRendererHyperText))
#define GTK_CELL_RENDERER_HYPER_TEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_CELL_RENDERER_HYPER_TEXT, GtkCellRendererHyperTextClass))
#define GTK_IS_CELL_RENDERER_HYPER_TEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_CELL_RENDERER_HYPER_TEXT))
#define GTK_IS_CELL_RENDERER_HYPER_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_CELL_RENDERER_HYPER_TEXT))
#define GTK_CELL_RENDERER_HYPER_TEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_CELL_RENDERER_HYPER_TEXT, GtkCellRendererHyperTextClass))

typedef struct _GtkCellRendererHyperText GtkCellRendererHyperText;
typedef struct _GtkCellRendererHyperTextClass GtkCellRendererHyperTextClass;

struct _GtkCellRendererHyperText
{
  GtkCellRendererText renderer_text;

  //private stuff from parent:
  //GtkCellRendererTextPrivate *priv;
  guint single_paragraph : 1;
  guint language_set : 1;
  guint markup_set : 1;
  PangoLanguage *language;
};

struct _GtkCellRendererHyperTextClass
{
  GtkCellRendererTextClass parent_class;
};

GType _gtk_cell_renderer_hyper_text_get_type(void) G_GNUC_CONST;

GtkCellRenderer *gtk_cell_renderer_hyper_text_new(void);

G_END_DECLS

#endif
