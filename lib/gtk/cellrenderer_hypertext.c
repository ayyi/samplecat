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

#include "config.h"
#include <string.h>
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "cellrenderer_hypertext.h"

static void gtk_cell_renderer_hypertext_get_preferred_width  (GtkCellRenderer*, GtkWidget *widget, int *minimal_size, int *natural_size);
static void gtk_cell_renderer_hypertext_get_preferred_height (GtkCellRenderer*, GtkWidget *widget, int *minimal_size, int *natural_size);
static void gtk_cell_renderer_hypertext_snapshot             (GtkCellRenderer*, GtkSnapshot *snapshot, GtkWidget *widget, const GdkRectangle *background_area, const GdkRectangle *cell_area, GtkCellRendererState flags);

static GtkCellRendererTextClass *parent_class;

static void
gtk_cell_renderer_hypertext_class_init (GtkCellRendererHyperTextClass *class)
{
  GtkCellRendererClass* cell_class = GTK_CELL_RENDERER_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  cell_class->get_preferred_width = gtk_cell_renderer_hypertext_get_preferred_width;
  cell_class->get_preferred_height = gtk_cell_renderer_hypertext_get_preferred_height;
  cell_class->snapshot = gtk_cell_renderer_hypertext_snapshot;
}

GType
_gtk_cell_renderer_hypertext_get_type (void)
{
  static GType cell_type = 0;

  if (!cell_type)
    {
      static const GTypeInfo cell_info =
      {
        sizeof (GtkCellRendererHyperTextClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
        (GClassInitFunc) gtk_cell_renderer_hypertext_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (GtkCellRendererHyperText),
	0,		/* n_preallocs */
        NULL,		/* instance_init */
	NULL,		/* value_table */
      };

      cell_type = g_type_register_static(GTK_TYPE_CELL_RENDERER_TEXT, "GtkCellRendererHyperText", &cell_info, 0);
    }

  return cell_type;
}

#ifdef GTK4_TODO
static void
gtk_cell_renderer_hypertext_get_size(GtkCellRenderer *cell,
				GtkWidget       *widget,
				GdkRectangle    *cell_area,
				gint            *x_offset,
				gint            *y_offset,
				gint            *width,
				gint            *height)
{
  GtkCellRendererHyperText *st = GTK_CELL_RENDERER_HYPERTEXT (cell);
  const char *text = st->renderer_text.text;

  if (!text)
    {
      if (width)
	*width = cell->xpad * 2 + 1;
      
      if (height)
	*height = cell->ypad * 2 + 1;

      if (x_offset) *x_offset = 0;
      if (y_offset) *y_offset = 0;
    }
  else
    {
      GTK_CELL_RENDERER_CLASS (parent_class)->get_size (cell, widget, cell_area, x_offset, y_offset, width, height);
    }
}
#endif


static void
gtk_cell_renderer_hypertext_get_preferred_width (GtkCellRenderer* cell, GtkWidget *widget, int *minimal_size, int *natural_size)
{
	return GTK_CELL_RENDERER_CLASS (parent_class)->get_preferred_width (cell, widget, minimal_size, natural_size);
}


static void
gtk_cell_renderer_hypertext_get_preferred_height (GtkCellRenderer* cell, GtkWidget *widget, int *minimal_size, int *natural_size)
{
}


#ifdef GTK4_TODO
static void
add_attr (PangoAttrList  *attr_list, PangoAttribute *attr)
{
  attr->start_index = 0;
  attr->end_index = G_MAXINT;

  pango_attr_list_insert(attr_list, attr);
}


static PangoLayout*
get_layout (GtkCellRendererHyperText *cellhypertext, GtkWidget *widget, gboolean will_render, GtkCellRendererState flags)
{
  //this is a static function from the parent class - i couldnt work out how to call it directly.

  //normally is called when mouse enters a new row.

  /*
  PangoLayout* layout = ( * GTK_CELL_RENDERER_TEXT_CLASS(parent_class)->get_layout)(celltext, widget, will_render, flags);
  */

  GtkCellRendererText *celltext = &cellhypertext->renderer_text;
  PangoAttrList *attr_list;
  //GtkCellRendererTextPrivate* priv = GTK_CELL_RENDERER_TEXT_GET_PRIVATE(parent->celltext);

  PangoLayout* layout = gtk_widget_create_pango_layout(widget, celltext->text);

  if(celltext->extra_attrs) attr_list = pango_attr_list_copy(celltext->extra_attrs);
  else                      attr_list = pango_attr_list_new();

  //pango_layout_set_single_paragraph_mode (layout, priv->single_paragraph);
  pango_layout_set_single_paragraph_mode (layout, cellhypertext->single_paragraph);

  if(will_render){
      // Add options that affect appearance but not size
      
      // note that background doesn't go here, since it affects background_area not the PangoLayout area
      
      if(celltext->foreground_set){
          PangoColor color = celltext->foreground;
          
          add_attr(attr_list, pango_attr_foreground_new(color.red, color.green, color.blue));
      }

      //if(celltext->strikethrough_set) add_attr(attr_list, pango_attr_strikethrough_new (celltext->strikethrough));
  }

  add_attr(attr_list, pango_attr_font_desc_new(celltext->font));

  if (celltext->scale_set && celltext->font_scale != 1.0)
    add_attr (attr_list, pango_attr_scale_new (celltext->font_scale));

  PangoUnderline uline;
  if (celltext->underline_set) uline = celltext->underline_style;
  else                         uline = PANGO_UNDERLINE_NONE;

  if(cellhypertext->language_set) add_attr(attr_list, pango_attr_language_new (cellhypertext->language));
  
  if((flags & GTK_CELL_RENDERER_PRELIT) == GTK_CELL_RENDERER_PRELIT) { //the flags args determines whether we are Prelit.
    switch (uline){
        case PANGO_UNDERLINE_NONE:
          uline = PANGO_UNDERLINE_SINGLE;
          break;

        case PANGO_UNDERLINE_SINGLE:
          uline = PANGO_UNDERLINE_DOUBLE;
          break;

        default:
          break;
    }
  }

  if (uline != PANGO_UNDERLINE_NONE) add_attr(attr_list, pango_attr_underline_new(celltext->underline_style));

  if (celltext->rise_set) add_attr(attr_list, pango_attr_rise_new (celltext->rise));
  
  pango_layout_set_attributes (layout, attr_list);
  pango_layout_set_width (layout, -1);

  pango_attr_list_unref (attr_list);
  
  return layout;
}
#endif


static void
gtk_cell_renderer_hypertext_snapshot (GtkCellRenderer* cell, GtkSnapshot *snapshot, GtkWidget *widget, const GdkRectangle *background_area, const GdkRectangle *cell_area, GtkCellRendererState flags)
{
#ifdef GTK4_TODO
  //flags is an enum, eg GTK_CELL_RENDERER_SELECTED, GTK_CELL_RENDERER_PRELIT.

  GtkCellRendererText *celltext = (GtkCellRendererText *) cell;
  GtkCellRendererHyperText *hypercell = (GtkCellRendererHyperText *)cell;
  GtkStateType state;
  gint x_offset;
  gint y_offset;
  //static gint prev_row_num = 0;

  //correct for scrollwindow:
  /*
  gint tx;
  gint ty;
  gtk_tree_view_widget_to_tree_coords(app->view, cell_area->x, cell_area->y, &tx, &ty);
  */

  if (!celltext->text){/* errprintf("cell_renderer_hypertext_render(): text NULL\n");*/ return; }

  //get_layout is what determines the colour/style of the text:
  //(it renders the text freshly each time!)
  //PangoLayout *layout = get_layout(hypercell, widget, TRUE, flags);
  PangoLayout *layout = get_layout(hypercell, widget, TRUE, 0);
  if(!layout){ dbg(0, "layout NULL"); return; }

  gtk_cell_renderer_hypertext_get_size(cell, widget, cell_area, &x_offset, &y_offset, NULL, NULL);

  /*
  if((app->mouse_x > cell_area->x) && (app->mouse_x < (cell_area->x + cell_area->width)) && (app->mouse_y > cell_area->y) && (app->mouse_y < (cell_area->y + cell_area->height))){
    //pango_layout_set_markup(layout, "<b>important</b>", -1);
  }
  */



  if ((flags & GTK_CELL_RENDERER_SELECTED) == GTK_CELL_RENDERER_SELECTED) {
      if (GTK_WIDGET_HAS_FOCUS (widget))                      state = GTK_STATE_SELECTED;
      else                                                    state = GTK_STATE_ACTIVE;
  } else if ((flags & GTK_CELL_RENDERER_PRELIT) == GTK_CELL_RENDERER_PRELIT /*&&
	   GTK_WIDGET_STATE (widget) == GTK_STATE_PRELIGHT*/) {
		//
                                                              state = GTK_STATE_PRELIGHT;
                                                 //               state = GTK_STATE_NORMAL;

			/*
        gint row_num = treecell_get_row(app->library->widget, cell_area);
		if ( row_num != prev_row_num ) {
			//printf("gtk_cell_renderer_hypertext_render() new row (%i)! cell=%p %s\n", row_num, celltext->text, celltext->text);

			//FIXME only do this if x is inside the cell.
			gchar path_string[256];
			snprintf(path_string, 256, "%i", prev_row_num);
			GtkTreeIter iter;
			if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(app->store), &iter, path_string)){
			  snprintf(path_string, 256, "<u>o</u>ld %i", prev_row_num);
			  //4 = COL_KEYWORDS
			  gtk_list_store_set(app->store, &iter, 4, path_string, -1); //FIXME its the _attributes_ we need to reset.

			  //this will affect ALL rows!?
			  g_object_set(cell, "attributes", NULL, NULL);
			}
			else warnprintf("gtk_cell_renderer_hypertext_render(): failed to get iter. string=%s\n", path_string);
		}
        prev_row_num = row_num;
			*/

		if(app->mouse_x < (cell_area->x + cell_area->width)){ 

			if(strlen(celltext->text)){
				g_strstrip(celltext->text);
				const gchar* str = celltext->text;//"<b>pre</b> light";

				gchar** split = g_strsplit(str, " ", 100);
				//printf("gtk_cell_renderer_hypertext_render(): split: [%s] %p %p %p %s\n", str, split[0], split[1], split[2], split[0]);
				int word_index = 0;
				gchar* joined = NULL;
				if(split[word_index]){
					char* item = split[word_index];
					split[word_index] = g_strdup_printf("<u>%s</u>", item);
					g_free(item);
					joined = g_strjoinv(" ", split);
				}
				g_strfreev(split);

				gchar* text = NULL;
				GError* error = NULL;
				PangoAttrList* attrs = NULL;

				if (joined && !pango_parse_markup(joined, -1, 0, &attrs, &text, NULL, &error)){
					g_warning("Failed to set cell text from markup due to error parsing markup: %s", error->message);
					g_error_free(error);
					return;
				}
				if (joined) g_free(joined);

				if (celltext->text) g_free (celltext->text);

				if (celltext->extra_attrs) pango_attr_list_unref (celltext->extra_attrs);

				//setting text here doesnt seem to work (text is set but not displayed), but setting markup does.
				//printf("  render(): prelight! setting text: %s\n", text);
				celltext->text = text;
				celltext->extra_attrs = attrs;
				hypercell->markup_set = TRUE;
			}
		}

  } else {
      if (GTK_WIDGET_STATE (widget) == GTK_STATE_INSENSITIVE) state = GTK_STATE_INSENSITIVE;
  }

  /*
  if ((flags & GTK_CELL_RENDERER_PRELIT) != GTK_CELL_RENDERER_PRELIT){
    //celltext->extra_attrs = NULL;
    //hypercell->markup_set = FALSE;
  }
  */

  if (celltext->background_set && state != GTK_STATE_SELECTED) {
      GdkColor color;
      GdkGC *gc;
      
      //color.red = 0xffff;
      color.red = celltext->background.red;
      color.green = celltext->background.green;
      color.blue = celltext->background.blue;

      gc = gdk_gc_new (window);

      gdk_gc_set_rgb_fg_color (gc, &color);

      if(expose_area) gdk_gc_set_clip_rectangle (gc, expose_area);
      gdk_draw_rectangle(window,
                          gc,
                          TRUE,
                          background_area->x,
                          background_area->y,
                          background_area->width,
                          background_area->height);
      if(expose_area) gdk_gc_set_clip_rectangle (gc, NULL);
      g_object_unref(gc);
  }

  gtk_paint_layout(widget->style,
                    window, //drawable to paint onto
                    //state,
                    //GTK_STATE_ACTIVE,
                    GTK_STATE_NORMAL,
	                TRUE, //use_text
                    expose_area,
                    widget,
                    //"cellrenderertext", //const gchar *detail - not documented!?
                    "cellrendererhypertext",
                    cell_area->x + x_offset + cell->xpad,
                    cell_area->y + y_offset + cell->ypad,
                    layout);

  g_object_unref(layout);
#endif
}

GtkCellRenderer*
gtk_cell_renderer_hypertext_new ()
{
  return g_object_new (GTK_TYPE_CELL_RENDERER_HYPERTEXT, NULL);
}


