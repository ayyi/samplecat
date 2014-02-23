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

#ifndef __simple_table_h__
#define __simple_table_h__


#include <gtk/gtkcontainer.h>


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

struct _GtkSimpleTable
{
  GtkContainer container;

  GList *GSEAL (children);
  GtkSimpleTableRowCol *GSEAL (rows);
  GtkSimpleTableRowCol *GSEAL (cols);
  guint16 GSEAL (nrows);
  guint16 GSEAL (ncols);
  guint16 GSEAL (column_spacing);
  guint16 GSEAL (row_spacing);
  guint GSEAL (homogeneous) : 1;
};

struct _GtkSimpleTableClass
{
  GtkContainerClass parent_class;
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


GType	   simple_table_get_type	    (void) G_GNUC_CONST;
GtkWidget* simple_table_new	            (guint rows, guint columns, gboolean homogeneous);
void	   simple_table_resize	        (GtkSimpleTable*, guint rows, guint columns);
void	   simple_table_attach	        (GtkSimpleTable*, GtkWidget* child, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, GtkAttachOptions xoptions, GtkAttachOptions yoptions, guint xpadding, guint ypadding);
void	   simple_table_attach_defaults (GtkSimpleTable*, GtkWidget* widget, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach);
void	   simple_table_set_row_spacing  (GtkSimpleTable	       *table,
				       guint		row,
				       guint		spacing);
guint      simple_table_get_row_spacing  (GtkSimpleTable        *table,
				       guint            row);
void	   simple_table_set_col_spacing  (GtkSimpleTable	       *table,
				       guint		column,
				       guint		spacing);
guint      simple_table_get_col_spacing  (GtkSimpleTable        *table,
				       guint            column);
void	   simple_table_set_row_spacings (GtkSimpleTable	       *table,
				       guint		spacing);
guint      simple_table_get_default_row_spacing (GtkSimpleTable        *table);
void	   simple_table_set_col_spacings (GtkSimpleTable	       *table,
				       guint		spacing);
guint      simple_table_get_default_col_spacing (GtkSimpleTable        *table);
void	   simple_table_set_homogeneous  (GtkSimpleTable	       *table,
				       gboolean		homogeneous);
gboolean   simple_table_get_homogeneous  (GtkSimpleTable        *table);
void       simple_table_get_size         (GtkSimpleTable*, guint* rows, guint* columns);


G_END_DECLS

#endif /* __GTK_SIMPLE_TABLE_H__ */
