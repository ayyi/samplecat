/*
 * Copyright (C) 2001 Mikael Hallendal <micke@imendio.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#pragma once

#include <glib.h>
#include <gtk/gtk.h>
//#include <gtk/gtkwidget.h>

#include "dh_link.h"

#define DH_BOOK_TREE_OAFIID "OAFIID:GNOME_DevHelp_DhBookTree"

#define DH_TYPE_BOOK_TREE            (dh_book_tree_get_type ())
#define DH_BOOK_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_BOOK_TREE, DhBookTree))
#define DH_BOOK_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_BOOK_TREE, DhBookTreeClass))
#define DH_IS_BOOK_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_BOOK_TREE))
#define DH_IS_BOOK_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), DH_TYPE_BOOK_TREE))

typedef struct _DhBookTree       DhBookTree;
typedef struct _DhBookTreeClass  DhBookTreeClass;
typedef struct _DhBookTreePriv   DhBookTreePriv;

struct _DhBookTree
{
	GtkTreeView     parent;

	DhBookTreePriv *priv;
};

struct _DhBookTreeClass
{
	GtkTreeViewClass parent_class;

	/* Signals */
	//void (*link_selected) (DhBookTree *book_tree, DhLink *link);
	void (*link_selected) (DhBookTree *book_tree, void *link);
};

GType         dh_book_tree_get_type  (void);
GtkWidget *   dh_book_tree_new       (GNode**);
void          dh_book_tree_reload    (DhBookTree*);
