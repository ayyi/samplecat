/* GTK - The GIMP Toolkit
 * Copyright (C) 2021 Red Hat, Inc.
 *
 * Authors:
 * - Matthias Clasen <mclasen@redhat.com>
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>

#define GDL_TYPE_GTK_PANED_HANDLE             (gdl_gtk_paned_handle_get_type ())
#define GDL_GTK_PANED_HANDLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDL_TYPE_GTK_PANED_HANDLE, GdlGtkPanedHandle))
#define GDL_GTK_PANED_HANDLE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GDL_TYPE_GTK_PANED_HANDLE, GdlGtkPanedHandleClass))
#define GDL_IS_GTK_PANED_HANDLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDL_TYPE_GTK_PANED_HANDLE))
#define GDL_IS_GTK_PANED_HANDLE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GDL_TYPE_GTK_PANED_HANDLE))
#define GDL_GTK_PANED_HANDLE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GDL_TYPE_GTK_PANED_HANDLE, GdlGtkPanedHandleClass))

typedef struct _GdlGtkPanedHandle             GdlGtkPanedHandle;
typedef struct _GdlGtkPanedHandleClass        GdlGtkPanedHandleClass;

struct _GdlGtkPanedHandle
{
  GtkWidget parent_instance;
};

struct _GdlGtkPanedHandleClass
{
  GtkWidgetClass parent_class;
};

G_DEFINE_TYPE (GdlGtkPanedHandle, gdl_gtk_paned_handle, GTK_TYPE_WIDGET);

static void
gdl_gtk_paned_handle_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
#ifdef GTK4_TODO
  GtkCssStyle* style = gtk_css_node_get_style (gtk_widget_get_css_node (widget));

  int width = gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);

  if (width > 0 && height > 0)
    gtk_css_style_snapshot_icon (style, snapshot, width, height);
#endif
}

#define HANDLE_EXTRA_SIZE 6

#ifdef GTK4_TODO
static gboolean
gdl_gtk_paned_handle_contains (GtkWidget *widget, double x, double y)
{
	GtkCssBoxes boxes;
	gtk_css_boxes_init (&boxes, widget);

	graphene_rect_t area;
	graphene_rect_init_from_rect (&area, gtk_css_boxes_get_border_rect (&boxes));

	GtkWidget* paned = gtk_widget_get_parent (widget);
	if (!gtk_paned_get_wide_handle (GDL_GTK_PANED (paned)))
		graphene_rect_inset (&area, - HANDLE_EXTRA_SIZE, - HANDLE_EXTRA_SIZE);

	return graphene_rect_contains_point (&area, &GRAPHENE_POINT_INIT (x, y));
}
#endif

static void
gdl_gtk_paned_handle_finalize (GObject *object)
{
  GdlGtkPanedHandle *self = GDL_GTK_PANED_HANDLE (object);

  GtkWidget *widget = gtk_widget_get_first_child (GTK_WIDGET (self));
  while (widget)
    {
      GtkWidget *next = gtk_widget_get_next_sibling (widget);

      gtk_widget_unparent (widget);

      widget = next;
    }

  G_OBJECT_CLASS (gdl_gtk_paned_handle_parent_class)->finalize (object);
}

static void
gdl_gtk_paned_handle_class_init (GdlGtkPanedHandleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = gdl_gtk_paned_handle_finalize;

  widget_class->snapshot = gdl_gtk_paned_handle_snapshot;
#ifdef GTK4_TODO
  widget_class->contains = gdl_gtk_paned_handle_contains;
#endif

  gtk_widget_class_set_css_name (widget_class, "separator");
}

static void
gdl_gtk_paned_handle_init (GdlGtkPanedHandle *self)
{
}

GtkWidget *
gdl_gtk_paned_handle_new (void)
{
  return g_object_new (GDL_TYPE_GTK_PANED_HANDLE, NULL);
}
