/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#pragma once

#include <gtk/gtk.h>


G_BEGIN_DECLS

#define GTK_TYPE_SIMPLE_TABLE            (simple_table_get_type ())
#define GTK_SIMPLE_TABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_SIMPLE_TABLE, GtkSimpleTable))
#define GTK_SIMPLE_TABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_SIMPLE_TABLE, GtkSimpleTableClass))
#define GTK_IS_SIMPLE_TABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_SIMPLE_TABLE))
#define GTK_IS_SIMPLE_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_SIMPLE_TABLE))
#define GTK_SIMPLE_TABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_SIMPLE_TABLE, GtkSimpleTableClass))


typedef struct _GtkSimpleTable        GtkSimpleTable;
typedef struct _GtkSimpleTableClass   GtkSimpleTableClass;
typedef struct _GtkSimpleTableChild   GtkSimpleTableChild;
typedef struct _GtkSimpleTableRowCol  GtkSimpleTableRowCol;

/**
 * GtkAttachOptions:
 * @GTK_EXPAND: the widget should expand to take up any extra space in its
 * container that has been allocated.
 * @GTK_SHRINK: the widget should shrink as and when possible.
 * @GTK_FILL: the widget should fill the space allocated to it.
 *
 * Denotes the expansion properties that a widget will have when it (or its
 * parent) is resized.
 */
typedef enum
{
  GTK_EXPAND = 1 << 0,
  GTK_SHRINK = 1 << 1,
  GTK_FILL   = 1 << 2
} GtkAttachOptions;

struct _GtkSimpleTable
{
#ifdef GTK4_TODO
  GtkContainer container;
#else
  GtkWidget             container;
#endif

  GList*                children;
  GtkSimpleTableRowCol* rows;
  GtkSimpleTableRowCol* cols;
  guint16               nrows;
  guint16               ncols;
  guint16               column_spacing;
  guint16               row_spacing;
  guint                 homogeneous : 1;
};

struct _GtkSimpleTableClass
{
#ifdef GTK4_TODO
  GtkContainerClass parent_class;
#else
  GtkWidgetClass    parent_class;
#endif
};

struct _GtkSimpleTableChild
{
  GtkWidget *widget;
  guint16 left_attach;
  guint16 right_attach;
  guint16 top_attach;
  guint16 bottom_attach;
  guint16 xpadding;
  guint16 ypadding;
  guint xexpand : 1;
  guint yexpand : 1;
  guint xshrink : 1;
  guint yshrink : 1;
  guint xfill : 1;
  guint yfill : 1;
};

struct _GtkSimpleTableRowCol
{
  guint16 requisition;
  guint16 allocation;
  guint16 spacing;
  guint need_expand : 1;
  guint need_shrink : 1;
  guint expand : 1;
  guint shrink : 1;
  guint empty : 1;
};


GType      simple_table_get_type                (void) G_GNUC_CONST;
GtkWidget* simple_table_new                     (guint rows, guint columns, gboolean homogeneous);
void       simple_table_resize                  (GtkSimpleTable*, guint rows, guint columns);
void       simple_table_attach                  (GtkSimpleTable*, GtkWidget* child, guint left, guint right, guint top, guint bottom, GtkAttachOptions, GtkAttachOptions, guint xpadding, guint ypadding);
void       simple_table_attach_defaults         (GtkSimpleTable*, GtkWidget*, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach);
void       simple_table_set_row_spacing         (GtkSimpleTable*, guint row, guint spacing);
guint      simple_table_get_row_spacing         (GtkSimpleTable*, guint row);
void       simple_table_set_col_spacing         (GtkSimpleTable*, guint column, guint spacing);
guint      simple_table_get_col_spacing         (GtkSimpleTable*, guint column);
void       simple_table_set_row_spacings        (GtkSimpleTable*, guint spacing);
guint      simple_table_get_default_row_spacing (GtkSimpleTable*);
void       simple_table_set_col_spacings        (GtkSimpleTable*, guint spacing);
guint      simple_table_get_default_col_spacing (GtkSimpleTable*);
void       simple_table_get_size                (GtkSimpleTable*, guint* rows, guint* columns);
#if 0
void       simple_table_empty_column            (GtkSimpleTable*, guint column);
#endif

void       simple_table_add	                    (GtkWidget*, GtkWidget*);
void       simple_table_remove	                (GtkWidget*, GtkWidget*);
void       simple_table_forall	                (GtkWidget*, gboolean include_internals, GtkCallback, gpointer);

void       simple_table_print                   (GtkSimpleTable*);


G_END_DECLS
