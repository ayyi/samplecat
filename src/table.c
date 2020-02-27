/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2014 Tim Orford <tim@orford.org>                       |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
* |                                                                      |
* | GtkTable but with homogenous row-height and differing size columns   |
* |                                                                      |
* +----------------------------------------------------------------------+
*
*/
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

#include "config.h"
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "typedefs.h"
#include "table.h"

#define GTK_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB

enum
{
  PROP_0,
  PROP_N_ROWS,
  PROP_N_COLUMNS,
  PROP_COLUMN_SPACING,
  PROP_ROW_SPACING,
  PROP_HOMOGENEOUS
};

enum
{
  CHILD_PROP_0,
  CHILD_PROP_LEFT_ATTACH,
  CHILD_PROP_RIGHT_ATTACH,
  CHILD_PROP_TOP_ATTACH,
  CHILD_PROP_BOTTOM_ATTACH,
  CHILD_PROP_X_OPTIONS,
  CHILD_PROP_Y_OPTIONS,
  CHILD_PROP_X_PADDING,
  CHILD_PROP_Y_PADDING
};
  

static void simple_table_finalize	    (GObject	    *object);
static void simple_table_size_request  (GtkWidget	    *widget,
				     GtkRequisition *requisition);
static void simple_table_size_allocate (GtkWidget	    *widget,
				     GtkAllocation  *allocation);
static void simple_table_add	    (GtkContainer   *container,
				     GtkWidget	    *widget);
static void simple_table_remove	    (GtkContainer   *container,
				     GtkWidget	    *widget);
static void simple_table_forall	    (GtkContainer   *container,
				     gboolean	     include_internals,
				     GtkCallback     callback,
				     gpointer	     callback_data);
static void simple_table_get_property       (GObject*, guint prop_id, GValue*, GParamSpec*);
static void simple_table_set_property       (GObject*, guint prop_id, const GValue*, GParamSpec*);
static void simple_table_set_child_property (GtkContainer*, GtkWidget* child, guint property_id, const GValue*, GParamSpec*);
static void simple_table_get_child_property (GtkContainer*, GtkWidget* child, guint property_id, GValue*, GParamSpec*);
static GType simple_table_child_type        (GtkContainer*);


static void simple_table_size_request_init  (GtkSimpleTable*);
static void simple_table_size_request_pass1 (GtkSimpleTable*);
static void simple_table_size_request_pass2 (GtkSimpleTable*);
static void simple_table_size_request_pass3 (GtkSimpleTable*);

static void simple_table_size_allocate_init  (GtkSimpleTable*);
static void simple_table_size_allocate_pass1 (GtkSimpleTable*);
static void simple_table_size_allocate_pass2 (GtkSimpleTable*);


G_DEFINE_TYPE (GtkSimpleTable, simple_table, GTK_TYPE_CONTAINER)

static void
simple_table_class_init (GtkSimpleTableClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);
  
  gobject_class->finalize = simple_table_finalize;

  gobject_class->get_property = simple_table_get_property;
  gobject_class->set_property = simple_table_set_property;
  
  widget_class->size_request = simple_table_size_request;
  widget_class->size_allocate = simple_table_size_allocate;
  
  container_class->add = simple_table_add;
  container_class->remove = simple_table_remove;
  container_class->forall = simple_table_forall;
  container_class->child_type = simple_table_child_type;
  container_class->set_child_property = simple_table_set_child_property;
  container_class->get_child_property = simple_table_get_child_property;
  

  g_object_class_install_property (gobject_class,
                                   PROP_N_ROWS,
                                   g_param_spec_uint ("n-rows",
						     "Rows",
						     "The number of rows in the table",
						     1,
						     65535,
						     1,
						     GTK_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_N_COLUMNS,
                                   g_param_spec_uint ("n-columns",
						     "Columns",
						     "The number of columns in the table",
						     1,
						     65535,
						     1,
						     GTK_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_ROW_SPACING,
                                   g_param_spec_uint ("row-spacing",
						     "Row spacing",
						     "The amount of space between two consecutive rows",
						     0,
						     65535,
						     0,
						     GTK_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_COLUMN_SPACING,
                                   g_param_spec_uint ("column-spacing",
						     "Column spacing",
						     "The amount of space between two consecutive columns",
						     0,
						     65535,
						     0,
						     GTK_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_HOMOGENEOUS,
                                   g_param_spec_boolean ("homogeneous",
							 "Homogeneous",
							 "If TRUE, the table cells are all the same width/height",
							 false,
							 GTK_PARAM_READWRITE));

  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_LEFT_ATTACH,
					      g_param_spec_uint ("left-attach", 
								 "Left attachment", 
								 "The column number to attach the left side of the child to",
								 0, 65535, 0,
								 GTK_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_RIGHT_ATTACH,
					      g_param_spec_uint ("right-attach", 
								 "Right attachment", 
								 "The column number to attach the right side of a child widget to",
								 1, 65535, 1,
								 GTK_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_TOP_ATTACH,
					      g_param_spec_uint ("top-attach", 
								 "Top attachment", 
								 "The row number to attach the top of a child widget to",
								 0, 65535, 0,
								 GTK_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_BOTTOM_ATTACH,
					      g_param_spec_uint ("bottom-attach",
								 "Bottom attachment", 
								 "The row number to attach the bottom of the child to",
								 1, 65535, 1,
								 GTK_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_X_OPTIONS,
					      g_param_spec_flags ("x-options", 
								  "Horizontal options", 
								  "Options specifying the horizontal behaviour of the child",
								  GTK_TYPE_ATTACH_OPTIONS, GTK_EXPAND | GTK_FILL,
								  GTK_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_Y_OPTIONS,
					      g_param_spec_flags ("y-options", 
								  "Vertical options", 
								  "Options specifying the vertical behaviour of the child",
								  GTK_TYPE_ATTACH_OPTIONS, GTK_EXPAND | GTK_FILL,
								  GTK_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_X_PADDING,
					      g_param_spec_uint ("x-padding", 
								 "Horizontal padding", 
								 "Extra space to put between the child and its left and right neighbors, in pixels",
								 0, 65535, 0,
								 GTK_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_Y_PADDING,
					      g_param_spec_uint ("y-padding", 
								 "Vertical padding", 
								 "Extra space to put between the child and its upper and lower neighbors, in pixels",
								 0, 65535, 0,
								 GTK_PARAM_READWRITE));
}

static GType
simple_table_child_type (GtkContainer   *container)
{
  return GTK_TYPE_WIDGET;
}

static void
simple_table_get_property (GObject      *object,
			guint         prop_id,
			GValue       *value,
			GParamSpec   *pspec)
{
  GtkSimpleTable *table;

  table = GTK_SIMPLE_TABLE (object);

  switch (prop_id)
    {
    case PROP_N_ROWS:
      g_value_set_uint (value, table->nrows);
      break;
    case PROP_N_COLUMNS:
      g_value_set_uint (value, table->ncols);
      break;
    case PROP_ROW_SPACING:
      g_value_set_uint (value, table->row_spacing);
      break;
    case PROP_COLUMN_SPACING:
      g_value_set_uint (value, table->column_spacing);
      break;
    case PROP_HOMOGENEOUS:
      g_value_set_boolean (value, table->homogeneous);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
simple_table_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  GtkSimpleTable *table;

  table = GTK_SIMPLE_TABLE (object);

  switch (prop_id)
    {
    case PROP_N_ROWS:
      simple_table_resize (table, g_value_get_uint (value), table->ncols);
      break;
    case PROP_N_COLUMNS:
      simple_table_resize (table, table->nrows, g_value_get_uint (value));
      break;
    case PROP_ROW_SPACING:
      simple_table_set_row_spacings (table, g_value_get_uint (value));
      break;
    case PROP_COLUMN_SPACING:
      simple_table_set_col_spacings (table, g_value_get_uint (value));
      break;
    case PROP_HOMOGENEOUS:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
simple_table_set_child_property (GtkContainer    *container,
			      GtkWidget       *child,
			      guint            property_id,
			      const GValue    *value,
			      GParamSpec      *pspec)
{
  GtkSimpleTable *table = GTK_SIMPLE_TABLE (container);
  GtkSimpleTableChild *table_child;
  GList *list;

  table_child = NULL;
  for (list = table->children; list; list = list->next)
    {
      table_child = list->data;

      if (table_child->widget == child)
	break;
    }
  if (!list)
    {
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      table_child->left_attach = g_value_get_uint (value);
      if (table_child->right_attach <= table_child->left_attach)
	table_child->right_attach = table_child->left_attach + 1;
      if (table_child->right_attach >= table->ncols)
	simple_table_resize (table, table->nrows, table_child->right_attach);
      break;
    case CHILD_PROP_RIGHT_ATTACH:
      table_child->right_attach = g_value_get_uint (value);
      if (table_child->right_attach <= table_child->left_attach)
	table_child->left_attach = table_child->right_attach - 1;
      if (table_child->right_attach >= table->ncols)
	simple_table_resize (table, table->nrows, table_child->right_attach);
      break;
    case CHILD_PROP_TOP_ATTACH:
      table_child->top_attach = g_value_get_uint (value);
      if (table_child->bottom_attach <= table_child->top_attach)
	table_child->bottom_attach = table_child->top_attach + 1;
      if (table_child->bottom_attach >= table->nrows)
	simple_table_resize (table, table_child->bottom_attach, table->ncols);
      break;
    case CHILD_PROP_BOTTOM_ATTACH:
      table_child->bottom_attach = g_value_get_uint (value);
      if (table_child->bottom_attach <= table_child->top_attach)
	table_child->top_attach = table_child->bottom_attach - 1;
      if (table_child->bottom_attach >= table->nrows)
	simple_table_resize (table, table_child->bottom_attach, table->ncols);
      break;
    case CHILD_PROP_X_OPTIONS:
      table_child->xexpand = (g_value_get_flags (value) & GTK_EXPAND) != 0;
      table_child->xshrink = (g_value_get_flags (value) & GTK_SHRINK) != 0;
      table_child->xfill = (g_value_get_flags (value) & GTK_FILL) != 0;
      break;
    case CHILD_PROP_Y_OPTIONS:
      table_child->yexpand = (g_value_get_flags (value) & GTK_EXPAND) != 0;
      table_child->yshrink = (g_value_get_flags (value) & GTK_SHRINK) != 0;
      table_child->yfill = (g_value_get_flags (value) & GTK_FILL) != 0;
      break;
    case CHILD_PROP_X_PADDING:
      table_child->xpadding = g_value_get_uint (value);
      break;
    case CHILD_PROP_Y_PADDING:
      table_child->ypadding = g_value_get_uint (value);
      break;
    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
  if (GTK_WIDGET_VISIBLE (child) && GTK_WIDGET_VISIBLE (table))
    gtk_widget_queue_resize (child);
}

static void
simple_table_get_child_property (GtkContainer    *container,
			      GtkWidget       *child,
			      guint            property_id,
			      GValue          *value,
			      GParamSpec      *pspec)
{
  GtkSimpleTable *table = GTK_SIMPLE_TABLE (container);
  GtkSimpleTableChild *table_child;
  GList *list;

  table_child = NULL;
  for (list = table->children; list; list = list->next)
    {
      table_child = list->data;

      if (table_child->widget == child)
	break;
    }
  if (!list)
    {
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      g_value_set_uint (value, table_child->left_attach);
      break;
    case CHILD_PROP_RIGHT_ATTACH:
      g_value_set_uint (value, table_child->right_attach);
      break;
    case CHILD_PROP_TOP_ATTACH:
      g_value_set_uint (value, table_child->top_attach);
      break;
    case CHILD_PROP_BOTTOM_ATTACH:
      g_value_set_uint (value, table_child->bottom_attach);
      break;
    case CHILD_PROP_X_OPTIONS:
      g_value_set_flags (value, (table_child->xexpand * GTK_EXPAND |
				 table_child->xshrink * GTK_SHRINK |
				 table_child->xfill * GTK_FILL));
      break;
    case CHILD_PROP_Y_OPTIONS:
      g_value_set_flags (value, (table_child->yexpand * GTK_EXPAND |
				 table_child->yshrink * GTK_SHRINK |
				 table_child->yfill * GTK_FILL));
      break;
    case CHILD_PROP_X_PADDING:
      g_value_set_uint (value, table_child->xpadding);
      break;
    case CHILD_PROP_Y_PADDING:
      g_value_set_uint (value, table_child->ypadding);
      break;
    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
simple_table_init (GtkSimpleTable *table)
{
  GTK_WIDGET_SET_FLAGS (table, GTK_NO_WINDOW);
  gtk_widget_set_redraw_on_allocate (GTK_WIDGET (table), false);
  
  table->children = NULL;
  table->rows = NULL;
  table->cols = NULL;
  table->nrows = 0;
  table->ncols = 0;
  table->column_spacing = 0;
  table->row_spacing = 0;
  table->homogeneous = false;

  simple_table_resize (table, 1, 1);
}

GtkWidget*
simple_table_new (guint	rows,
	       guint	columns,
	       gboolean homogeneous)
{
  GtkSimpleTable *table;

  if (rows == 0)
    rows = 1;
  if (columns == 0)
    columns = 1;
  
  table = g_object_new (GTK_TYPE_SIMPLE_TABLE, NULL);
  
  table->homogeneous = (homogeneous ? TRUE : false);

  simple_table_resize (table, rows, columns);
  
  return GTK_WIDGET (table);
}

void
simple_table_resize (GtkSimpleTable* table, guint n_rows, guint n_cols)
{
	g_return_if_fail (GTK_IS_SIMPLE_TABLE (table));
	g_return_if_fail (n_rows > 0 && n_rows <= 65535);
	g_return_if_fail (n_cols > 0 && n_cols <= 65535);

	n_rows = MAX (n_rows, 1);
	n_cols = MAX (n_cols, 1);

  if (n_rows != table->nrows ||
      n_cols != table->ncols)
    {
      GList *list;
      
      for (list = table->children; list; list = list->next)
	{
	  GtkSimpleTableChild *child;
	  
	  child = list->data;
	  
	  n_rows = MAX (n_rows, child->bottom_attach);
	  n_cols = MAX (n_cols, child->right_attach);
	}
      
      if (n_rows != table->nrows)
	{
	  guint i;

	  i = table->nrows;
	  table->nrows = n_rows;
	  table->rows = g_realloc (table->rows, table->nrows * sizeof (GtkSimpleTableRowCol));
	  
	  for (; i < table->nrows; i++)
	    {
	      table->rows[i].requisition = 0;
	      table->rows[i].allocation = 0;
	      table->rows[i].spacing = table->row_spacing;
	      table->rows[i].need_expand = 0;
	      table->rows[i].need_shrink = 0;
	      table->rows[i].expand = 0;
	      table->rows[i].shrink = 0;
	    }

	  g_object_notify (G_OBJECT (table), "n-rows");
	}

      if (n_cols != table->ncols)
	{
	  guint i;

	  i = table->ncols;
	  table->ncols = n_cols;
	  table->cols = g_realloc (table->cols, table->ncols * sizeof (GtkSimpleTableRowCol));
	  
	  for (; i < table->ncols; i++)
	    {
	      table->cols[i].requisition = 0;
	      table->cols[i].allocation = 0;
	      table->cols[i].spacing = table->column_spacing;
	      table->cols[i].need_expand = 0;
	      table->cols[i].need_shrink = 0;
	      table->cols[i].expand = 0;
	      table->cols[i].shrink = 0;
	    }

	  g_object_notify (G_OBJECT (table), "n-columns");
	}
    }
}


void
simple_table_attach (GtkSimpleTable* table, GtkWidget* child, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, GtkAttachOptions xoptions, GtkAttachOptions yoptions, guint xpadding, guint ypadding)
{
	GtkSimpleTableChild *table_child;

	g_return_if_fail (GTK_IS_SIMPLE_TABLE (table));
	g_return_if_fail (GTK_IS_WIDGET (child));
	g_return_if_fail (child->parent == NULL);

	/* g_return_if_fail (left_attach >= 0); */
	g_return_if_fail (left_attach < right_attach);
	/* g_return_if_fail (top_attach >= 0); */
	g_return_if_fail (top_attach < bottom_attach);

	if (right_attach >= table->ncols)
		simple_table_resize (table, table->nrows, right_attach);

	if (bottom_attach >= table->nrows)
		simple_table_resize (table, bottom_attach, table->ncols);

	table_child = g_new (GtkSimpleTableChild, 1);
	table_child->widget = child;
	table_child->left_attach = left_attach;
	table_child->right_attach = right_attach;
	table_child->top_attach = top_attach;
	table_child->bottom_attach = bottom_attach;
	table_child->xexpand = (xoptions & GTK_EXPAND) != 0;
	table_child->xshrink = (xoptions & GTK_SHRINK) != 0;
	table_child->xfill = (xoptions & GTK_FILL) != 0;
	table_child->xpadding = xpadding;
	table_child->yexpand = (yoptions & GTK_EXPAND) != 0;
	table_child->yshrink = (yoptions & GTK_SHRINK) != 0;
	table_child->yfill = (yoptions & GTK_FILL) != 0;
	table_child->ypadding = ypadding;

	table->children = g_list_prepend (table->children, table_child);

	gtk_widget_set_parent (child, GTK_WIDGET (table));
}


void
simple_table_attach_defaults (GtkSimpleTable  *table,
			   GtkWidget *widget,
			   guint      left_attach,
			   guint      right_attach,
			   guint      top_attach,
			   guint      bottom_attach)
{
	simple_table_attach (table, widget, left_attach, right_attach, top_attach, bottom_attach, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
}


void
simple_table_set_row_spacing (GtkSimpleTable *table,
			   guint     row,
			   guint     spacing)
{
  g_return_if_fail (GTK_IS_SIMPLE_TABLE (table));
  g_return_if_fail (row < table->nrows);
  
  if (table->rows[row].spacing != spacing)
    {
      table->rows[row].spacing = spacing;
      
      if (GTK_WIDGET_VISIBLE (table))
	gtk_widget_queue_resize (GTK_WIDGET (table));
    }
}

/**
 * simple_table_get_row_spacing:
 * @table: a #GtkSimpleTable
 * @row: a row in the table, 0 indicates the first row
 *
 * Gets the amount of space between row @row, and
 * row @row + 1. See simple_table_set_row_spacing().
 *
 * Return value: the row spacing
 **/
guint
simple_table_get_row_spacing (GtkSimpleTable *table,
			   guint     row)
{
  g_return_val_if_fail (GTK_IS_SIMPLE_TABLE (table), 0);
  g_return_val_if_fail (row < table->nrows - 1, 0);
 
  return table->rows[row].spacing;
}

void
simple_table_set_col_spacing (GtkSimpleTable *table,
			   guint     column,
			   guint     spacing)
{
  g_return_if_fail (GTK_IS_SIMPLE_TABLE (table));
  g_return_if_fail (column < table->ncols);
  
  if (table->cols[column].spacing != spacing)
    {
      table->cols[column].spacing = spacing;
      
      if (GTK_WIDGET_VISIBLE (table))
	gtk_widget_queue_resize (GTK_WIDGET (table));
    }
}

/**
 * simple_table_get_col_spacing:
 * @table: a #GtkSimpleTable
 * @column: a column in the table, 0 indicates the first column
 *
 * Gets the amount of space between column @col, and
 * column @col + 1. See simple_table_set_col_spacing().
 *
 * Return value: the column spacing
 **/
guint
simple_table_get_col_spacing (GtkSimpleTable *table,
			   guint     column)
{
  g_return_val_if_fail (GTK_IS_SIMPLE_TABLE (table), 0);
  g_return_val_if_fail (column < table->ncols, 0);

  return table->cols[column].spacing;
}

void
simple_table_set_row_spacings (GtkSimpleTable *table,
			    guint     spacing)
{
  guint row;
  
  g_return_if_fail (GTK_IS_SIMPLE_TABLE (table));
  
  table->row_spacing = spacing;
  for (row = 0; row < table->nrows; row++)
    table->rows[row].spacing = spacing;
  
  if (GTK_WIDGET_VISIBLE (table))
    gtk_widget_queue_resize (GTK_WIDGET (table));

  g_object_notify (G_OBJECT (table), "row-spacing");
}

/**
 * simple_table_get_default_row_spacing:
 * @table: a #GtkSimpleTable
 *
 * Gets the default row spacing for the table. This is
 * the spacing that will be used for newly added rows.
 * (See simple_table_set_row_spacings())
 *
 * Return value: the default row spacing
 **/
guint
simple_table_get_default_row_spacing (GtkSimpleTable *table)
{
  g_return_val_if_fail (GTK_IS_SIMPLE_TABLE (table), 0);

  return table->row_spacing;
}

void
simple_table_set_col_spacings (GtkSimpleTable *table,
			    guint     spacing)
{
  guint col;
  
  g_return_if_fail (GTK_IS_SIMPLE_TABLE (table));
  
  table->column_spacing = spacing;
  for (col = 0; col < table->ncols; col++)
    table->cols[col].spacing = spacing;
  
  if (GTK_WIDGET_VISIBLE (table))
    gtk_widget_queue_resize (GTK_WIDGET (table));

  g_object_notify (G_OBJECT (table), "column-spacing");
}

/**
 * simple_table_get_default_col_spacing:
 * @table: a #GtkSimpleTable
 *
 * Gets the default column spacing for the table. This is
 * the spacing that will be used for newly added columns.
 * (See simple_table_set_col_spacings())
 *
 * Return value: the default column spacing
 **/
guint
simple_table_get_default_col_spacing (GtkSimpleTable *table)
{
  g_return_val_if_fail (GTK_IS_SIMPLE_TABLE (table), 0);

  return table->column_spacing;
}

/**
 * gtk_table_get_size:
 * @table: a #GtkTable
 * @rows: (out) (allow-none): return location for the number of
 *   rows, or %NULL
 * @columns: (out) (allow-none): return location for the number
 *   of columns, or %NULL
 *
 * Returns the number of rows and columns in the table.
 *
 * Since: 2.22
 **/
void
simple_table_get_size (GtkSimpleTable *table,
                    guint    *rows,
                    guint    *columns)
{
  g_return_if_fail (GTK_IS_SIMPLE_TABLE (table));

  if (rows)
    *rows = table->nrows;

  if (columns)
    *columns = table->ncols;
}

#if 0
void
simple_table_empty_column (GtkSimpleTable* table, guint column)
{
  g_return_if_fail (GTK_IS_SIMPLE_TABLE (table));

  GList* children = g_list_copy(table->children);
  GList* l = children;
  for (;l;l=l->next){
    GtkSimpleTableChild* child = l->data;
    if(child->left_attach == column) gtk_widget_destroy(child->widget);
  }
  g_list_free(children);
}
#endif

static void
simple_table_finalize (GObject *object)
{
  GtkSimpleTable *table = GTK_SIMPLE_TABLE (object);

  g_free (table->rows);
  g_free (table->cols);
  
  G_OBJECT_CLASS (simple_table_parent_class)->finalize (object);
}

static void
simple_table_size_request (GtkWidget      *widget,
			GtkRequisition *requisition)
{
  GtkSimpleTable *table = GTK_SIMPLE_TABLE (widget);
  gint row, col;

  requisition->width = 0;
  requisition->height = 0;
  
  simple_table_size_request_init (table);
  simple_table_size_request_pass1 (table);
  simple_table_size_request_pass2 (table);
  simple_table_size_request_pass3 (table);
  simple_table_size_request_pass2 (table);
  
  for (col = 0; col < table->ncols; col++)
    requisition->width += table->cols[col].requisition;
  for (col = 0; col + 1 < table->ncols; col++)
    requisition->width += table->cols[col].spacing;
  
  for (row = 0; row < table->nrows; row++)
    requisition->height += table->rows[row].requisition;
  for (row = 0; row + 1 < table->nrows; row++)
    requisition->height += table->rows[row].spacing;
  
  requisition->width += GTK_CONTAINER (table)->border_width * 2;
  requisition->height += GTK_CONTAINER (table)->border_width * 2;
}

static void
simple_table_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  GtkSimpleTable *table = GTK_SIMPLE_TABLE (widget);

  widget->allocation = *allocation;

  simple_table_size_allocate_init (table);
  simple_table_size_allocate_pass1 (table);
  simple_table_size_allocate_pass2 (table);
}

static void
simple_table_add (GtkContainer *container, GtkWidget *widget)
{
  simple_table_attach_defaults (GTK_SIMPLE_TABLE (container), widget, 0, 1, 0, 1);
}

static void
simple_table_remove (GtkContainer *container, GtkWidget *widget)
{
  GtkSimpleTable *table = GTK_SIMPLE_TABLE (container);
  GtkSimpleTableChild *child;
  GList *children;

  children = table->children;
  
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (child->widget == widget)
	{
	  gboolean was_visible = GTK_WIDGET_VISIBLE (widget);
	  
	  gtk_widget_unparent (widget);
	  
	  table->children = g_list_remove (table->children, child);
	  g_free (child);
	  
	  if (was_visible && GTK_WIDGET_VISIBLE (container))
	    gtk_widget_queue_resize (GTK_WIDGET (container));
	  break;
	}
    }
}

static void
simple_table_forall (GtkContainer *container,
		  gboolean	include_internals,
		  GtkCallback	callback,
		  gpointer	callback_data)
{
  GtkSimpleTable *table = GTK_SIMPLE_TABLE (container);
  GtkSimpleTableChild *child;
  GList *children;

  children = table->children;
  
  while (children)
    {
      child = children->data;
      children = children->next;
      
      (* callback) (child->widget, callback_data);
    }
}

static void
simple_table_size_request_init (GtkSimpleTable *table)
{
  GtkSimpleTableChild *child;
  GList *children;
  gint row, col;
  
  for (row = 0; row < table->nrows; row++)
    {
      table->rows[row].requisition = 0;
      table->rows[row].expand = false;
    }
  for (col = 0; col < table->ncols; col++)
    {
      table->cols[col].requisition = 0;
      table->cols[col].expand = false;
    }
  
  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (GTK_WIDGET_VISIBLE (child->widget))
	gtk_widget_size_request (child->widget, NULL);

      if (child->left_attach == (child->right_attach - 1) && child->xexpand)
	table->cols[child->left_attach].expand = TRUE;
      
      if (child->top_attach == (child->bottom_attach - 1) && child->yexpand)
	table->rows[child->top_attach].expand = TRUE;
    }
}

static void
simple_table_size_request_pass1 (GtkSimpleTable *table)
{
  GtkSimpleTableChild *child;
  GList *children;
  gint width;
  gint height;
  
  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  GtkRequisition child_requisition;
	  gtk_widget_get_child_requisition (child->widget, &child_requisition);

	  /* Child spans a single column.
	   */
	  if (child->left_attach == (child->right_attach - 1))
	    {
	      width = child_requisition.width + child->xpadding * 2;
	      table->cols[child->left_attach].requisition = MAX (table->cols[child->left_attach].requisition, width);
	    }
	  
	  /* Child spans a single row.
	   */
	  if (child->top_attach == (child->bottom_attach - 1))
	    {
	      height = child_requisition.height + child->ypadding * 2;
	      table->rows[child->top_attach].requisition = MAX (table->rows[child->top_attach].requisition, height);
	    }
	}
    }
}

static void
simple_table_size_request_pass2 (GtkSimpleTable *table)
{
	gint max_width;
	gint max_height;
	gint row, col;

	if (table->homogeneous) {
		max_width = 0;
		max_height = 0;

		for (col = 0; col < table->ncols; col++)
			max_width = MAX (max_width, table->cols[col].requisition);
		for (row = 0; row < table->nrows; row++)
			max_height = MAX (max_height, table->rows[row].requisition);

		/*
		 *    This is the only major change from GtkTable
		 */
																										// ********** cols evenly divided !!
		//for (col = 0; col < table->ncols; col++)
		//	table->cols[col].requisition = max_width;
		for (row = 0; row < table->nrows; row++)
			table->rows[row].requisition = max_height;

		/*
		for (col = 0; col < table->ncols; col++)
			dbg(0, "%i: req=%i", col, table->cols[col].requisition);
		*/
	}
}

static void
simple_table_size_request_pass3 (GtkSimpleTable *table)
{
	GtkSimpleTableChild *child;
	GList *children;
	gint width, height;
	gint row, col;
	gint extra;

	children = table->children;
	while (children) {
		child = children->data;
		children = children->next;

		if (GTK_WIDGET_VISIBLE (child->widget)) {
			// Child spans multiple columns.
			if (child->left_attach != (child->right_attach - 1)) {
				GtkRequisition child_requisition;

				gtk_widget_get_child_requisition (child->widget, &child_requisition);

				/* Check and see if there is already enough space
				 *  for the child.
				 */
				width = 0;
				for (col = child->left_attach; col < child->right_attach; col++) {
					width += table->cols[col].requisition;
					if ((col + 1) < child->right_attach)
						width += table->cols[col].spacing;
				}

				/* If we need to request more space for this child to fill
				 *  its requisition, then divide up the needed space amongst the
				 *  columns it spans, favoring expandable columns if any.
				 */
				if (width < child_requisition.width + child->xpadding * 2) {
					gint n_expand = 0;
					gboolean force_expand = false;

					width = child_requisition.width + child->xpadding * 2 - width;

					for (col = child->left_attach; col < child->right_attach; col++)
						if (table->cols[col].expand)
							n_expand++;

					if (n_expand == 0) {
						n_expand = (child->right_attach - child->left_attach);
						force_expand = TRUE;
					}

					for (col = child->left_attach; col < child->right_attach; col++)
						if (force_expand || table->cols[col].expand) {
							extra = width / n_expand;
							table->cols[col].requisition += extra;
							width -= extra;
							n_expand--;
						}
				}
			}

			// Child spans multiple rows.
			if (child->top_attach != (child->bottom_attach - 1)) {
				GtkRequisition child_requisition;

				gtk_widget_get_child_requisition (child->widget, &child_requisition);

				/* Check and see if there is already enough space
				*  for the child.
				*/
				height = 0;
				for (row = child->top_attach; row < child->bottom_attach; row++) {
					height += table->rows[row].requisition;
					if ((row + 1) < child->bottom_attach)
						height += table->rows[row].spacing;
				}

				/* If we need to request more space for this child to fill
				*  its requisition, then divide up the needed space amongst the
				*  rows it spans, favoring expandable rows if any.
				*/
				if (height < child_requisition.height + child->ypadding * 2) {
					gint n_expand = 0;
					gboolean force_expand = false;

					height = child_requisition.height + child->ypadding * 2 - height;

					for (row = child->top_attach; row < child->bottom_attach; row++) {
						if (table->rows[row].expand)
							n_expand++;
					}

					if (n_expand == 0) {
						n_expand = (child->bottom_attach - child->top_attach);
						force_expand = TRUE;
					}

				for (row = child->top_attach; row < child->bottom_attach; row++)
					if (force_expand || table->rows[row].expand) {
						extra = height / n_expand;
						table->rows[row].requisition += extra;
						height -= extra;
						n_expand--;
					}
				}
			}
		}
	}
}

static void
simple_table_size_allocate_init (GtkSimpleTable *table)
{
	GtkSimpleTableChild *child;
	gint row, col;
	gint has_expand;
	gint has_shrink;

	/* Initialize the rows and cols.
	 *  By default, rows and cols do not expand and do shrink.
	 *  Those values are modified by the children that occupy
	 *  the rows and cols.
	 */
	for (col = 0; col < table->ncols; col++) {
		table->cols[col].allocation = table->cols[col].requisition;
		table->cols[col].need_expand = false;
		table->cols[col].need_shrink = TRUE;
		table->cols[col].expand = false;
		table->cols[col].shrink = TRUE;
		table->cols[col].empty = TRUE;
	}

	for (row = 0; row < table->nrows; row++) {
		table->rows[row].allocation = table->rows[row].requisition;
		table->rows[row].need_expand = false;
		table->rows[row].need_shrink = TRUE;
		table->rows[row].expand = false;
		table->rows[row].shrink = TRUE;
		table->rows[row].empty = TRUE;
	}
  
	/* Loop over all the children and adjust the row and col values
	 *  based on whether the children want to be allowed to expand
	 *  or shrink. This loop handles children that occupy a single
	 *  row or column.
	 */
	GList* children = table->children;
	while (children) {
		child = children->data;
		children = children->next;
      
		if (GTK_WIDGET_VISIBLE (child->widget)) {
			if (child->left_attach == (child->right_attach - 1)) {
				if (child->xexpand)
					table->cols[child->left_attach].expand = TRUE;

				if (!child->xshrink)
					table->cols[child->left_attach].shrink = false;

				table->cols[child->left_attach].empty = false;
			}

			if (child->top_attach == (child->bottom_attach - 1)) {
				if (child->yexpand)
					table->rows[child->top_attach].expand = TRUE;

				if (!child->yshrink)
					table->rows[child->top_attach].shrink = false;

				table->rows[child->top_attach].empty = false;
			}
		}
	}

	/* Loop over all the children again and this time handle children
	 *  which span multiple rows or columns.
	 */
	children = table->children;
	while (children) {
		child = children->data;
		children = children->next;

		if (GTK_WIDGET_VISIBLE (child->widget)) {
			if (child->left_attach != (child->right_attach - 1)) {
				for (col = child->left_attach; col < child->right_attach; col++)
					table->cols[col].empty = false;

				if (child->xexpand) {
					has_expand = false;
					for (col = child->left_attach; col < child->right_attach; col++)
						if (table->cols[col].expand) {
							has_expand = true;
							break;
						}

					if (!has_expand)
						for (col = child->left_attach; col < child->right_attach; col++)
							table->cols[col].need_expand = true;
				}

				if (!child->xshrink) {
					has_shrink = true;
					for (col = child->left_attach; col < child->right_attach; col++)
						if (!table->cols[col].shrink) {
							has_shrink = false;
							break;
						}

					if (has_shrink)
						for (col = child->left_attach; col < child->right_attach; col++)
							table->cols[col].need_shrink = false;
				}
			}

			if (child->top_attach != (child->bottom_attach - 1)) {
				for (row = child->top_attach; row < child->bottom_attach; row++)
					table->rows[row].empty = false;

				if (child->yexpand) {
					has_expand = false;
					for (row = child->top_attach; row < child->bottom_attach; row++)
						if (table->rows[row].expand) {
							has_expand = TRUE;
							break;
						}

					if (!has_expand)
						for (row = child->top_attach; row < child->bottom_attach; row++)
							table->rows[row].need_expand = TRUE;
				}

				if (!child->yshrink) {
					has_shrink = TRUE;
					for (row = child->top_attach; row < child->bottom_attach; row++)
						if (!table->rows[row].shrink) {
							has_shrink = false;
							break;
						}

					if (has_shrink)
						for (row = child->top_attach; row < child->bottom_attach; row++)
							table->rows[row].need_shrink = false;
				}
			}
		}
	}

	/* Loop over the columns and set the expand and shrink values
	 *  if the column can be expanded or shrunk.
	 */
	for (col = 0; col < table->ncols; col++) {
		if (table->cols[col].empty) {
			table->cols[col].expand = false;
			table->cols[col].shrink = false;
		} else {
			if (table->cols[col].need_expand)
				table->cols[col].expand = TRUE;
			if (!table->cols[col].need_shrink)
				table->cols[col].shrink = false;
		}
	}

	/* Loop over the rows and set the expand and shrink values
	 *  if the row can be expanded or shrunk.
	 */
	for (row = 0; row < table->nrows; row++) {
		if (table->rows[row].empty)	{
			table->rows[row].expand = false;
			table->rows[row].shrink = false;
		} else {
			if (table->rows[row].need_expand)
				table->rows[row].expand = TRUE;
			if (!table->rows[row].need_shrink)
				table->rows[row].shrink = false;
		}
	}

	/*
	for (col = 0; col < table->ncols; col++) {
		dbg(0, "%i: need_expand=%i need_shrink=%i", col, table->cols[col].need_expand, table->cols[col].need_shrink);
	}
	*/
}


static void
simple_table_size_allocate_pass1 (GtkSimpleTable *table)
{
	gint real_width;
	gint width;
	gint col;
	gint nexpand;
	gint extra;

	/* If we were allocated more space than we requested
	 *  then we have to expand any expandable columns
	 *  to fill in the extra space.
	 */

	real_width = GTK_WIDGET (table)->allocation.width - GTK_CONTAINER (table)->border_width * 2;

	if (!table->children)
		nexpand = 1;
	else {
		nexpand = 0;
		for (col = 0; col < table->ncols; col++)
			if (table->cols[col].expand) {
				nexpand += 1;
				break;
			}
	}
	if (nexpand) {
																		dbg(0, "expanding!");
		width = real_width;
		for (col = 0; col + 1 < table->ncols; col++)
			width -= table->cols[col].spacing;

		for (col = 0; col < table->ncols; col++) {
			extra = width / (table->ncols - col);
			table->cols[col].allocation = MAX (1, extra);
			width -= extra;
		}
	}
}


static void
simple_table_size_allocate_pass2 (GtkSimpleTable *table)
{
	GtkSimpleTableChild* child;
	gint max_width;
	gint max_height;
	gint x, y;
	gint row, col;
	GtkAllocation allocation;
	GtkWidget *widget = GTK_WIDGET (table);

	int c = 0;
	GList* children = table->children;
	while (children) {
		child = children->data;
		children = children->next;

		if (GTK_WIDGET_VISIBLE (child->widget)) {
			GtkRequisition child_requisition;
			gtk_widget_get_child_requisition (child->widget, &child_requisition);

			x = GTK_WIDGET (table)->allocation.x + GTK_CONTAINER (table)->border_width;
			y = GTK_WIDGET (table)->allocation.y + GTK_CONTAINER (table)->border_width;
			max_width = 0;
			max_height = 0;

			for (col = 0; col < child->left_attach; col++) {
				x += table->cols[col].allocation;
				x += table->cols[col].spacing;
			}

			for (col = child->left_attach; col < child->right_attach; col++) {
				max_width += table->cols[col].allocation;
				if ((col + 1) < child->right_attach)
					max_width += table->cols[col].spacing;
			}

			for (row = 0; row < child->top_attach; row++) {
				y += table->rows[row].allocation;
				y += table->rows[row].spacing;
			}

			for (row = child->top_attach; row < child->bottom_attach; row++) {
				max_height += table->rows[row].allocation;
				if ((row + 1) < child->bottom_attach)
					max_height += table->rows[row].spacing;
			}

			if (child->xfill) {
				allocation.width = MAX (1, max_width - (gint)child->xpadding * 2);
				allocation.x = x + (max_width - allocation.width) / 2;
			} else {
				allocation.width = child_requisition.width;
				allocation.x = x + (max_width - allocation.width) / 2;
			}

			if (child->yfill) {
				allocation.height = MAX (1, max_height - (gint)child->ypadding * 2);
				allocation.y = y + (max_height - allocation.height) / 2;
			} else {
				allocation.height = child_requisition.height;
				allocation.y = y + (max_height - allocation.height) / 2;
			}

			if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
				allocation.x = widget->allocation.x + widget->allocation.width - (allocation.x - widget->allocation.x) - allocation.width;

			gtk_widget_size_allocate (child->widget, &allocation);
			//dbg(0, "%i: %i-%i requisition=%i allocation.width=%i", child->top_attach, child->left_attach, child->right_attach, child_requisition.width, allocation.width);
		}
		c++;
	}
}


void
simple_table_print(GtkSimpleTable* table)
{
	PF0;

	GtkSimpleTableChild* rows[table->nrows][table->ncols];
	memset(rows, 0, table->nrows * table->ncols * sizeof(GtkSimpleTableChild*));

	GList* l = table->children;
	for (;l;l=l->next){
		GtkSimpleTableChild* child = l->data;
		rows[child->top_attach][child->left_attach] = child;
	}
	int x, y;
	for(y=0;y<table->nrows;y++){
		printf("%2i: ", y);
		for(x=0;x<table->ncols;x++){
			GtkSimpleTableChild* child = rows[y][x];
			printf("%19s", child ? G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(child->widget)) : "");
		}
		printf("\n");
	}
}

#define __simple_table_c__
