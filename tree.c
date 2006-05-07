/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2003      CodeFactory AB
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

//#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreestore.h>

//#include "dh-marshal.h"
#include "tree.h"
#include "dh-link.h"
#include "rox_global.h"
#include "type.h"
#include "pixmaps.h"

#define d(x)

#define DATA_DIR "~/"

typedef struct {
        GdkPixbuf *pixbuf_opened;
        GdkPixbuf *pixbuf_closed;
        GdkPixbuf *pixbuf_helpdoc;
} DhBookTreePixbufs;

struct _DhBookTreePriv {
	GtkTreeStore      *store;

	DhBookTreePixbufs *pixbufs;
 	GNode             *link_tree;
};

static void book_tree_class_init           (DhBookTreeClass      *klass);
static void book_tree_init                 (DhBookTree           *tree);

static void book_tree_finalize             (GObject              *object);

static void book_tree_add_columns          (DhBookTree           *tree);
static void book_tree_setup_selection      (DhBookTree           *tree);
static void book_tree_populate_tree        (DhBookTree           *tree);
static void book_tree_insert_node          (DhBookTree           *tree,
					    GNode                *node,
					    GtkTreeIter          *parent_iter);
static void book_tree_create_pixbufs       (DhBookTree           *tree);
static void book_tree_selection_changed_cb (GtkTreeSelection     *selection,
					    DhBookTree           *tree);

enum {
        LINK_SELECTED,
        LAST_SIGNAL
};

enum {
	COL_OPEN_PIXBUF,
	COL_CLOSED_PIXBUF,
	COL_TITLE,
	COL_LINK,
	N_COLUMNS
};

static GtkTreeViewClass *parent_class = NULL;
static gint              signals[LAST_SIGNAL] = { 0 };

GType
dh_book_tree_get_type (void)
{
        static GType type = 0;

        if (!type) {
                static const GTypeInfo info = {
                        sizeof (DhBookTreeClass),
			NULL,
			NULL,
                        (GClassInitFunc) book_tree_class_init,
			NULL,
			NULL,
                        sizeof (DhBookTree),
			0,
                        (GInstanceInitFunc) book_tree_init,
                };
		
		type = g_type_register_static (GTK_TYPE_TREE_VIEW,
					       "DhBookTree",
					       &info, 0);
        }

        return type;
}

static void
book_tree_class_init (DhBookTreeClass *klass)
{
	GObjectClass   *object_class;
	
	object_class = (GObjectClass *) klass;
	parent_class = g_type_class_peek_parent (klass);
	
	object_class->finalize = book_tree_finalize;
	
	signals[LINK_SELECTED] = g_signal_new ("link_selected",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (DhBookTreeClass, link_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER, //dh_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);
}

static void
book_tree_init (DhBookTree *tree)
{
	DhBookTreePriv *priv;
        
	priv        = g_new0 (DhBookTreePriv, 1);
	priv->store = gtk_tree_store_new (N_COLUMNS, 
					  GDK_TYPE_PIXBUF,
					  GDK_TYPE_PIXBUF,
					  G_TYPE_STRING,
					  G_TYPE_POINTER);

	tree->priv = priv;

	gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (priv->store));

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
	
	book_tree_create_pixbufs (tree);

	book_tree_add_columns (tree);

	book_tree_setup_selection (tree);
}

static void
book_tree_finalize (GObject *object)
{
	DhBookTree     *tree;
	DhBookTreePriv *priv;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (DH_IS_BOOK_TREE (object));
	
	tree = DH_BOOK_TREE (object);
	priv  = tree->priv;

	if (priv) {
		g_object_unref (priv->store);

		if (priv->pixbufs->pixbuf_opened) g_object_unref (priv->pixbufs->pixbuf_opened);
		if (priv->pixbufs->pixbuf_closed) g_object_unref (priv->pixbufs->pixbuf_closed);
		if (priv->pixbufs->pixbuf_helpdoc) g_object_unref (priv->pixbufs->pixbuf_helpdoc);
		g_free (priv->pixbufs);
			
		g_free (priv);
		
		tree->priv = NULL;
	}

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		G_OBJECT_CLASS (parent_class)->finalize (object);
	}
}

static void
book_tree_add_columns (DhBookTree *tree)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	column = gtk_tree_view_column_new ();
	
	renderer = GTK_CELL_RENDERER (gtk_cell_renderer_pixbuf_new ());
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes
		(column, renderer,
		 "pixbuf", COL_OPEN_PIXBUF,
		 "pixbuf-expander-open", COL_OPEN_PIXBUF,
		 "pixbuf-expander-closed", COL_CLOSED_PIXBUF,
		 NULL);
	
	renderer = GTK_CELL_RENDERER (gtk_cell_renderer_text_new ());
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", COL_TITLE,
					     NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
}

static void
book_tree_setup_selection (DhBookTree *tree)
{
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

	g_signal_connect (selection, "changed",
			  G_CALLBACK (book_tree_selection_changed_cb),
			  tree);
}

static void
book_tree_populate_tree (DhBookTree *tree)
{
	DhBookTreePriv *priv;
	GNode          *node;
	
	g_return_if_fail (tree != NULL);
	g_return_if_fail (DH_IS_BOOK_TREE (tree));

	priv = tree->priv;

	/* Use the link tree ... */
/* 	books = dh_bookshelf_get_books (priv->bookshelf); */

	for (node = g_node_first_child (priv->link_tree);
	     node;
	     node = g_node_next_sibling (node)) {
		book_tree_insert_node (tree, node, NULL);
	}
}

static void
book_tree_insert_node (DhBookTree  *tree, 
		       GNode       *node,
		       GtkTreeIter *parent_iter)

{
	DhBookTreePriv      *priv;
	GtkTreeIter          iter;
	DhLink              *link;
	GNode               *child;
        
	g_return_if_fail (DH_IS_BOOK_TREE (tree));
	g_return_if_fail (node != NULL);

	priv = tree->priv;
	link = (DhLink *) node->data;
	
	gtk_tree_store_append (priv->store, &iter, parent_iter);

	if (link->type == DH_LINK_TYPE_BOOK) {
		//printf("book_tree_insert_node(): DH_LINK_TYPE_BOOK: %s\n", link->name);
		gtk_tree_store_set (priv->store, &iter, 
				    COL_OPEN_PIXBUF,   priv->pixbufs->pixbuf_opened,
				    COL_CLOSED_PIXBUF, priv->pixbufs->pixbuf_closed,
				    COL_TITLE,         link->name,
				    COL_LINK,          link,
				    -1);
	} else {
		//printf("book_tree_insert_node(): not book. %s %p\n", link->name, link);
		gtk_tree_store_set (priv->store, &iter, 
				    COL_OPEN_PIXBUF,   priv->pixbufs->pixbuf_helpdoc,
				    COL_CLOSED_PIXBUF, priv->pixbufs->pixbuf_helpdoc,
				    COL_TITLE,         link->name,
				    COL_LINK,          link,
				    -1);
	}
	
	for (child = g_node_first_child (node);
	     child;
	     child = g_node_next_sibling (child)) {
		book_tree_insert_node (tree, child, &iter);
	}
}

static void
book_tree_create_pixbufs (DhBookTree *tree)
{
 	DhBookTreePixbufs *pixbufs;
	
	g_return_if_fail (DH_IS_BOOK_TREE (tree));
	
	pixbufs = g_new0 (DhBookTreePixbufs, 1);

    GdkPixbuf* iconbuf = NULL;
    MIME_type* mime_type = mime_type_lookup("inode/directory");
    type_to_icon(mime_type);
    if ( mime_type->image == NULL ) printf("db_get_dirs(): no icon.\n");
    iconbuf = mime_type->image->sm_pixbuf;

	pixbufs->pixbuf_closed = iconbuf;//gdk_pixbuf_new_from_file (DATA_DIR "/devhelp/images/book_closed.png", NULL);
	pixbufs->pixbuf_opened = gdk_pixbuf_new_from_file (DATA_DIR "/devhelp/images/book_open.png", NULL);
	pixbufs->pixbuf_helpdoc = iconbuf;//gdk_pixbuf_new_from_file (DATA_DIR "/devhelp/images/helpdoc.png", NULL);

	tree->priv->pixbufs = pixbufs;
}

static void
book_tree_selection_changed_cb (GtkTreeSelection *selection, DhBookTree *tree)
{
	GtkTreeIter  iter;
	DhLink      *link;
	
	g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
	g_return_if_fail (DH_IS_BOOK_TREE (tree));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (tree->priv->store), 
				    &iter, COL_LINK, &link, -1);

		d(g_print ("emiting '%s'\n", link->uri));

		g_signal_emit (tree, signals[LINK_SELECTED], 0, link);
	}
}

GtkWidget *
dh_book_tree_new (GNode *books)
{
	DhBookTree *tree;

	tree = g_object_new (DH_TYPE_BOOK_TREE, NULL);

	tree->priv->link_tree = books;

	book_tree_populate_tree (tree);
	
	return GTK_WIDGET (tree);
}

void
dh_book_tree_show_uri (DhBookTree *tree, const gchar *uri)
{
#if 0
	GtkTreeSelection    *selection;
	GtkTreeRowReference *row;
	GtkTreePath         *path;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

	/* FIXME: Hmm .. */
	row = g_hash_table_lookup (tree->priv->node_rows, book_node);
	g_return_if_fail (row != NULL);

	path = gtk_tree_row_reference_get_path (row);
	g_return_if_fail (path != NULL);
	
	g_signal_handlers_block_by_func
		(selection, 
		 book_tree_selection_changed_cb,
		 tree);

	gtk_tree_selection_select_path (selection, path);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (tree), path, NULL, 0);	

	g_signal_handlers_unblock_by_func
		(selection, 
		 book_tree_selection_changed_cb,
		 tree);

	gtk_tree_path_free (path);
#endif
}
