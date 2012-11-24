/* rotator.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#if defined(GTK_DISABLE_SINGLE_INCLUDES) && !defined (__GTK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#ifndef __rotator_h__
#define __rotator_h__

#include <gtk/gtkcontainer.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtktreeviewcolumn.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkentry.h>

G_BEGIN_DECLS


#if 0
typedef enum
{
  /* drop before/after this row */
  GTK_ROTATOR_DROP_BEFORE,
  GTK_ROTATOR_DROP_AFTER,
  /* drop as a child of this row (with fallback to before or after
   * if into is not possible)
   */
  GTK_ROTATOR_DROP_INTO_OR_BEFORE,
  GTK_ROTATOR_DROP_INTO_OR_AFTER
} RotatorDropPosition;
#endif

#define GTK_TYPE_ROTATOR            (rotator_get_type ())
#define GTK_ROTATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_ROTATOR, Rotator))
#define GTK_ROTATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_ROTATOR, RotatorClass))
#define GTK_IS_ROTATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_ROTATOR))
#define GTK_IS_ROTATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_ROTATOR))
#define GTK_ROTATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_ROTATOR, RotatorClass))

typedef struct _Rotator               Rotator;
typedef struct _RotatorClass          RotatorClass;
typedef struct _RotatorPrivate        RotatorPrivate;
//typedef struct _GtkTreeSelection      GtkTreeSelection;
//typedef struct _GtkTreeSelectionClass GtkTreeSelectionClass;

struct _Rotator
{
  GtkContainer parent;

  RotatorPrivate* priv;
};

struct _RotatorClass
{
  GtkContainerClass parent_class;

  void     (* set_scroll_adjustments)     (Rotator       *tree_view, GtkAdjustment     *hadjustment, GtkAdjustment     *vadjustment);
  //void     (* row_activated)              (Rotator       *tree_view, GtkTreePath       *path, RotatorColumn *column);
  gboolean (* test_expand_row)            (Rotator       *tree_view, GtkTreeIter       *iter, GtkTreePath       *path);
  gboolean (* test_collapse_row)          (Rotator       *tree_view, GtkTreeIter       *iter, GtkTreePath       *path);
  void     (* row_expanded)               (Rotator       *tree_view, GtkTreeIter       *iter, GtkTreePath       *path);
  void     (* row_collapsed)              (Rotator       *tree_view, GtkTreeIter       *iter, GtkTreePath       *path);
  void     (* columns_changed)            (Rotator       *tree_view);
  void     (* cursor_changed)             (Rotator       *tree_view);

  /* Key Binding signals */
  gboolean (* move_cursor)                (Rotator*, GtkMovementStep    step, gint               count);
  gboolean (* select_all)                 (Rotator*);
  gboolean (* unselect_all)               (Rotator*);
  gboolean (* select_cursor_row)          (Rotator*, gboolean           start_editing);
  gboolean (* toggle_cursor_row)          (Rotator*);
  gboolean (* expand_collapse_cursor_row) (Rotator*, gboolean           logical, gboolean           expand, gboolean           open_all);
  gboolean (* select_cursor_parent)       (Rotator*);
  gboolean (* start_interactive_search)   (Rotator*);

  /* Padding for future expansion */
  void (*_gtk_reserved0) (void);
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


//typedef gboolean (* RotatorColumnDropFunc)  (Rotator* tree_view, RotatorColumn       *column, RotatorColumn       *prev_column, RotatorColumn       *next_column, gpointer                 data);
typedef void     (*RotatorMappingFunc)        (Rotator* tree_view, GtkTreePath             *path, gpointer                 user_data);
typedef gboolean (*RotatorSearchEqualFunc)    (GtkTreeModel* model, gint                     column, const gchar             *key, GtkTreeIter             *iter, gpointer                 search_data);
typedef gboolean (*RotatorRowSeparatorFunc)   (GtkTreeModel* model, GtkTreeIter* iter, gpointer           data);
typedef void     (*RotatorSearchPositionFunc) (Rotator* tree_view, GtkWidget* search_dialog, gpointer user_data);


/* Creators */
GType                  rotator_get_type                      (void) G_GNUC_CONST;
GtkWidget*             rotator_new                           (void);
GtkWidget*             rotator_new_with_model                (GtkTreeModel*);

/* Accessors */
GtkTreeModel*          rotator_get_model                     (Rotator*);
void                   rotator_set_model                     (Rotator*, GtkTreeModel*);
GtkTreeSelection*      rotator_get_selection                 (Rotator*);
GtkAdjustment*         rotator_get_hadjustment               (Rotator*);
void                   rotator_set_hadjustment               (Rotator*, GtkAdjustment             *adjustment);
GtkAdjustment*         rotator_get_vadjustment               (Rotator*);
void                   rotator_set_vadjustment               (Rotator*, GtkAdjustment             *adjustment);
gboolean               rotator_get_headers_visible           (Rotator*);
void                   rotator_set_headers_visible           (Rotator*, gboolean                   headers_visible);
void                   rotator_columns_autosize              (Rotator*);
gboolean               rotator_get_headers_clickable         (Rotator*);
void                   rotator_set_headers_clickable         (Rotator*, gboolean                   setting);
void                   rotator_set_rules_hint                (Rotator*, gboolean                   setting);
gboolean               rotator_get_rules_hint                (Rotator*);

/* Column funtions */
#if 0
gint                   rotator_append_column                 (Rotator               *tree_view, RotatorColumn         *column);
gint                   rotator_remove_column                 (Rotator               *tree_view, RotatorColumn         *column);
gint                   rotator_insert_column                 (Rotator               *tree_view, RotatorColumn         *column, gint                       position);
gint                   rotator_insert_column_with_attributes (Rotator               *tree_view, gint                       position, const gchar               *title, GtkCellRenderer           *cell, ...) G_GNUC_NULL_TERMINATED;
gint                   rotator_insert_column_with_data_func  (Rotator               *tree_view, gint                       position, const gchar               *title, GtkCellRenderer           *cell, GtkTreeCellDataFunc        func, gpointer                   data, GDestroyNotify             dnotify);
RotatorColumn*         rotator_get_column                    (Rotator               *tree_view, gint                       n);
GList                 *rotator_get_columns                   (Rotator               *tree_view);
void                   rotator_move_column_after             (Rotator               *tree_view, RotatorColumn         *column, RotatorColumn         *base_column);
void                   rotator_set_expander_column           (Rotator               *tree_view, RotatorColumn         *column);
RotatorColumn*         rotator_get_expander_column           (Rotator               *tree_view);
void                   rotator_set_column_drag_function      (Rotator               *tree_view, RotatorColumnDropFunc  func, gpointer                   user_data, GDestroyNotify             destroy);
#endif

/* Actions */
void                   rotator_scroll_to_point               (Rotator               *tree_view, gint                       tree_x, gint                       tree_y);
#if 0
void                   rotator_scroll_to_cell                (Rotator               *tree_view, GtkTreePath               *path, RotatorColumn         *column, gboolean                   use_align, gfloat                     row_align, gfloat                     col_align);
void                   rotator_row_activated                 (Rotator               *tree_view, GtkTreePath               *path, RotatorColumn         *column);
void                   rotator_expand_all                    (Rotator               *tree_view);
void                   rotator_collapse_all                  (Rotator               *tree_view);
void                   rotator_expand_to_path                (Rotator               *tree_view, GtkTreePath               *path);
gboolean               rotator_expand_row                    (Rotator               *tree_view, GtkTreePath               *path, gboolean                   open_all);
gboolean               rotator_collapse_row                  (Rotator               *tree_view, GtkTreePath               *path);
void                   rotator_map_expanded_rows             (Rotator               *tree_view, RotatorMappingFunc     func, gpointer                   data);
gboolean               rotator_row_expanded                  (Rotator               *tree_view, GtkTreePath               *path);
void                   rotator_set_reorderable               (Rotator               *tree_view, gboolean                   reorderable);
gboolean               rotator_get_reorderable               (Rotator               *tree_view);
void                   rotator_set_cursor                    (Rotator               *tree_view, GtkTreePath               *path, RotatorColumn         *focus_column, gboolean                   start_editing);
void                   rotator_set_cursor_on_cell            (Rotator               *tree_view, GtkTreePath               *path, RotatorColumn         *focus_column, GtkCellRenderer           *focus_cell, gboolean                   start_editing);
void                   rotator_get_cursor                    (Rotator               *tree_view, GtkTreePath              **path, RotatorColumn        **focus_column);


/* Layout information */
GdkWindow             *rotator_get_bin_window                (Rotator               *tree_view);
gboolean               rotator_get_path_at_pos               (Rotator               *tree_view, gint                       x, gint                       y, GtkTreePath              **path, RotatorColumn        **column, gint                      *cell_x, gint                      *cell_y);
void                   rotator_get_cell_area                 (Rotator               *tree_view, GtkTreePath               *path, RotatorColumn         *column, GdkRectangle              *rect);
void                   rotator_get_background_area           (Rotator               *tree_view, GtkTreePath               *path, RotatorColumn         *column, GdkRectangle              *rect);
void                   rotator_get_visible_rect              (Rotator               *tree_view, GdkRectangle              *visible_rect);

#ifndef GTK_DISABLE_DEPRECATED
void                   rotator_widget_to_tree_coords         (Rotator               *tree_view, gint                       wx, gint                       wy, gint                      *tx, gint                      *ty);
void                   rotator_tree_to_widget_coords         (Rotator               *tree_view, gint                       tx, gint                       ty, gint                      *wx, gint                      *wy);
#endif /* !GTK_DISABLE_DEPRECATED */
gboolean               rotator_get_visible_range             (Rotator               *tree_view, GtkTreePath              **start_path, GtkTreePath              **end_path);

/* Drag-and-Drop support */
void                   rotator_enable_model_drag_source      (Rotator               *tree_view, GdkModifierType            start_button_mask, const GtkTargetEntry      *targets, gint                       n_targets, GdkDragAction              actions);
void                   rotator_enable_model_drag_dest        (Rotator               *tree_view, const GtkTargetEntry      *targets, gint                       n_targets, GdkDragAction              actions);
void                   rotator_unset_rows_drag_source        (Rotator               *tree_view);
void                   rotator_unset_rows_drag_dest          (Rotator               *tree_view);


/* These are useful to implement your own custom stuff. */
void                   rotator_set_drag_dest_row             (Rotator               *tree_view, GtkTreePath               *path, RotatorDropPosition    pos);
void                   rotator_get_drag_dest_row             (Rotator               *tree_view, GtkTreePath              **path, RotatorDropPosition   *pos);
gboolean               rotator_get_dest_row_at_pos           (Rotator               *tree_view, gint                       drag_x, gint                       drag_y, GtkTreePath              **path, RotatorDropPosition   *pos);
GdkPixmap             *rotator_create_row_drag_icon          (Rotator               *tree_view, GtkTreePath               *path);

/* Interactive search */
void                       rotator_set_enable_search     (Rotator                *tree_view, gboolean                    enable_search);
gboolean                   rotator_get_enable_search     (Rotator                *tree_view);
gint                       rotator_get_search_column     (Rotator                *tree_view);
void                       rotator_set_search_column     (Rotator                *tree_view, gint                        column);
RotatorSearchEqualFunc rotator_get_search_equal_func (Rotator                *tree_view);
void                       rotator_set_search_equal_func (Rotator                *tree_view, RotatorSearchEqualFunc  search_equal_func, gpointer                    search_user_data, GDestroyNotify              search_destroy);

GtkEntry                     *rotator_get_search_entry         (Rotator                   *tree_view);
void                          rotator_set_search_entry         (Rotator                   *tree_view, GtkEntry                      *entry);
RotatorSearchPositionFunc rotator_get_search_position_func (Rotator                   *tree_view);
void                          rotator_set_search_position_func (Rotator                   *tree_view, RotatorSearchPositionFunc  func, gpointer                       data, GDestroyNotify                 destroy);

/* Convert between the different coordinate systems */
void rotator_convert_widget_to_tree_coords       (Rotator *tree_view, gint         wx, gint         wy, gint        *tx, gint        *ty);
void rotator_convert_tree_to_widget_coords       (Rotator *tree_view, gint         tx, gint         ty, gint        *wx, gint        *wy);
void rotator_convert_widget_to_bin_window_coords (Rotator *tree_view, gint         wx, gint         wy, gint        *bx, gint        *by);
void rotator_convert_bin_window_to_widget_coords (Rotator *tree_view, gint         bx, gint         by, gint        *wx, gint        *wy);
void rotator_convert_tree_to_bin_window_coords   (Rotator *tree_view, gint         tx, gint         ty, gint        *bx, gint        *by);
void rotator_convert_bin_window_to_tree_coords   (Rotator *tree_view, gint         bx, gint         by, gint        *tx, gint        *ty);

/* This function should really never be used.  It is just for use by ATK.
 */
typedef void (* GtkTreeDestroyCountFunc)  (Rotator             *tree_view, GtkTreePath             *path, gint                     children, gpointer                 user_data);
void rotator_set_destroy_count_func (Rotator             *tree_view, GtkTreeDestroyCountFunc  func, gpointer                 data, GDestroyNotify           destroy);
#endif

void     rotator_set_fixed_height_mode (Rotator          *tree_view, gboolean              enable);
gboolean rotator_get_fixed_height_mode (Rotator          *tree_view);
void     rotator_set_hover_selection   (Rotator          *tree_view, gboolean              hover);
gboolean rotator_get_hover_selection   (Rotator          *tree_view);
void     rotator_set_hover_expand      (Rotator          *tree_view, gboolean              expand);
gboolean rotator_get_hover_expand      (Rotator          *tree_view);
void     rotator_set_rubber_banding    (Rotator          *tree_view, gboolean              enable);
gboolean rotator_get_rubber_banding    (Rotator          *tree_view);

gboolean rotator_is_rubber_banding_active (Rotator       *tree_view);

RotatorRowSeparatorFunc rotator_get_row_separator_func (Rotator               *tree_view);
void                        rotator_set_row_separator_func (Rotator                *tree_view, RotatorRowSeparatorFunc func, gpointer                    data, GDestroyNotify              destroy);

#if 0
RotatorGridLines        rotator_get_grid_lines         (Rotator                *tree_view);
void                        rotator_set_grid_lines         (Rotator                *tree_view, RotatorGridLines        grid_lines);
gboolean                    rotator_get_enable_tree_lines  (Rotator                *tree_view);
void                        rotator_set_enable_tree_lines  (Rotator                *tree_view, gboolean                    enabled);
void                        rotator_set_show_expanders     (Rotator                *tree_view, gboolean                    enabled);
gboolean                    rotator_get_show_expanders     (Rotator                *tree_view);
void                        rotator_set_level_indentation  (Rotator                *tree_view, gint                        indentation);
gint                        rotator_get_level_indentation  (Rotator                *tree_view);

/* Convenience functions for setting tooltips */
void          rotator_set_tooltip_row    (Rotator       *tree_view, GtkTooltip        *tooltip, GtkTreePath       *path);
void          rotator_set_tooltip_cell   (Rotator       *tree_view, GtkTooltip        *tooltip, GtkTreePath       *path, RotatorColumn *column, GtkCellRenderer   *cell);
gboolean      rotator_get_tooltip_context(Rotator       *tree_view, gint              *x, gint              *y, gboolean           keyboard_tip, GtkTreeModel     **model, GtkTreePath      **path, GtkTreeIter       *iter);
void          rotator_set_tooltip_column (Rotator       *tree_view, gint               column);
gint          rotator_get_tooltip_column (Rotator       *tree_view);
#endif

G_END_DECLS


#endif /* __rotator_h__ */
