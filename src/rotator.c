/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://samplecat.orford.org          |
* | copyright (C) 2007-2021 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
/* gtktreeview.c
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


#ifdef GTK4_TODO
#include "config.h"
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include "debug/debug.h"
#include "waveform/actor.h"

#include "application.h"
#include "list_store.h"
#include "rotator.h"

#ifndef USE_SYSTEM_GTKGLEXT
#undef gdk_gl_drawable_make_current

#define gdk_gl_drawable_make_current(DRAWABLE, CONTEXT) \
	((G_OBJECT_TYPE(DRAWABLE) == GDK_TYPE_GL_PIXMAP) \
		? gdk_gl_pixmap_make_context_current (DRAWABLE, CONTEXT) \
		: gdk_gl_window_make_context_current (DRAWABLE, CONTEXT) \
	)
#endif

#if 0
#define GTK_ROTATOR_PRIORITY_VALIDATE (GDK_PRIORITY_REDRAW + 5)
#define GTK_ROTATOR_PRIORITY_SCROLL_SYNC (GTK_ROTATOR_PRIORITY_VALIDATE + 2)
#define GTK_ROTATOR_NUM_ROWS_PER_IDLE 500
#define SCROLL_EDGE_SIZE 15
#define EXPANDER_EXTRA_PADDING 4
#define GTK_ROTATOR_SEARCH_DIALOG_TIMEOUT 5000

#define BACKGROUND_HEIGHT(node) (GTK_RBNODE_GET_HEIGHT (node))
#define CELL_HEIGHT(node, separator) ((BACKGROUND_HEIGHT (node)) - (separator))

/* Translate from bin_window coordinates to rbtree (tree coordinates) and
 * vice versa.
 */
#define TREE_WINDOW_Y_TO_RBTREE_Y(tree_view,y) ((y) + tree_view->priv->dy)
#define RBTREE_Y_TO_TREE_WINDOW_Y(tree_view,y) ((y) - tree_view->priv->dy)

/* This is in bin_window coordinates */
#define BACKGROUND_FIRST_PIXEL(tree_view,tree,node) (RBTREE_Y_TO_TREE_WINDOW_Y (tree_view, _gtk_rbtree_node_find_offset ((tree), (node))))
#define CELL_FIRST_PIXEL(tree_view,tree,node,separator) (BACKGROUND_FIRST_PIXEL (tree_view,tree,node) + separator/2)

#define ROW_HEIGHT(tree_view,height) \
  ((height > 0) ? (height) : (tree_view)->priv->expander_size)


typedef struct _RotatorChild RotatorChild;
struct _RotatorChild
{
  GtkWidget *widget;
  gint x;
  gint y;
  gint width;
  gint height;
};


typedef struct _TreeViewDragInfo TreeViewDragInfo;
struct _TreeViewDragInfo
{
  GdkModifierType start_button_mask;
  GtkTargetList *_unused_source_target_list;
  GdkDragAction source_actions;

  GtkTargetList *_unused_dest_target_list;

  guint source_set : 1;
  guint dest_set : 1;
};
#endif


/* Signals */
enum
{
  ROW_ACTIVATED,
  TEST_EXPAND_ROW,
  TEST_COLLAPSE_ROW,
  ROW_EXPANDED,
  ROW_COLLAPSED,
  COLUMNS_CHANGED,
  CURSOR_CHANGED,
  MOVE_CURSOR,
  SELECT_ALL,
  UNSELECT_ALL,
  SELECT_CURSOR_ROW,
  TOGGLE_CURSOR_ROW,
  EXPAND_COLLAPSE_CURSOR_ROW,
  SELECT_CURSOR_PARENT,
  START_INTERACTIVE_SEARCH,
  LAST_SIGNAL
};

/* Properties */
enum {
  PROP_0,
  PROP_MODEL,
#if 0
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HEADERS_VISIBLE,
  PROP_HEADERS_CLICKABLE,
  PROP_EXPANDER_COLUMN,
  PROP_REORDERABLE,
  PROP_RULES_HINT,
  PROP_ENABLE_SEARCH,
  PROP_SEARCH_COLUMN,
  PROP_FIXED_HEIGHT_MODE,
  PROP_HOVER_SELECTION,
  PROP_HOVER_EXPAND,
  PROP_SHOW_EXPANDERS,
  PROP_LEVEL_INDENTATION,
  PROP_RUBBER_BANDING,
  PROP_ENABLE_GRID_LINES,
  PROP_ENABLE_TREE_LINES,
#endif
};

/* object signals */
static void     rotator_finalize             (GObject*);
static void     rotator_set_property         (GObject*, guint prop_id, const GValue*, GParamSpec*);
static void     rotator_get_property         (GObject*, guint prop_id, GValue*, GParamSpec*);

/* gtkobject signals */
static void     rotator_destroy              (GtkObject*);

/* gtkwidget signals */
static void     rotator_realize              (GtkWidget*);
static void     rotator_unrealize            (GtkWidget*);
#if 0
static void     rotator_map                  (GtkWidget*);
static void     rotator_size_request         (GtkWidget*, GtkRequisition*);
#endif
static void     rotator_size_allocate        (GtkWidget*, GtkAllocation*);
static gboolean rotator_expose               (GtkWidget*, GdkEventExpose*);
#if 0
static gboolean rotator_key_press            (GtkWidget*, GdkEventKey      *event);
static gboolean rotator_key_release          (GtkWidget*, GdkEventKey      *event);
static gboolean rotator_motion               (GtkWidget*, GdkEventMotion   *event);
static gboolean rotator_enter_notify         (GtkWidget*, GdkEventCrossing *event);
static gboolean rotator_leave_notify         (GtkWidget*, GdkEventCrossing *event);
#endif
static gboolean rotator_button_press         (GtkWidget*, GdkEventButton   *event);
#if 0
static gboolean rotator_button_release       (GtkWidget*, GdkEventButton   *event);
static gboolean rotator_grab_broken          (GtkWidget*, GdkEventGrabBroken *event);
#if 0
static gboolean rotator_configure            (GtkWidget*, GdkEventConfigure *event);
#endif

static void     rotator_set_focus_child      (GtkContainer*, GtkWidget* child);
static gint     rotator_focus_out            (GtkWidget*, GdkEventFocus*);
static gint     rotator_focus                (GtkWidget*, GtkDirectionType);
static void     rotator_grab_focus           (GtkWidget*);
static void     rotator_style_set            (GtkWidget*, GtkStyle* previous_style);
static void     rotator_grab_notify          (GtkWidget*, gboolean was_grabbed);
static void     rotator_state_changed        (GtkWidget*, GtkStateType previous_state);

/* container signals */
static void     rotator_remove               (GtkContainer*, GtkWidget*);
static void     rotator_forall               (GtkContainer*, gboolean include_internals, GtkCallback, gpointer);

/* Source side drag signals */
static void rotator_drag_begin               (GtkWidget*, GdkDragContext*);
static void rotator_drag_end                 (GtkWidget*, GdkDragContext*);
static void rotator_drag_data_get            (GtkWidget*, GdkDragContext*, GtkSelectionData*, guint info, guint time);
static void rotator_drag_data_delete         (GtkWidget*, GdkDragContext*);

/* Target side drag signals */
static void     rotator_drag_leave           (GtkWidget*, GdkDragContext*, guint time);
static gboolean rotator_drag_motion          (GtkWidget*, GdkDragContext*, gint x, gint y, guint time);
static gboolean rotator_drag_drop            (GtkWidget*, GdkDragContext*, gint x, gint y, guint time);
static void     rotator_drag_data_received   (GtkWidget*, GdkDragContext*, gint x, gint y, GtkSelectionData*, guint info, guint time);

/* tree_model signals */
static void rotator_set_adjustments                 (Rotator     *tree_view, GtkAdjustment   *hadj, GtkAdjustment   *vadj);
#endif
static gboolean rotator_real_move_cursor            (Rotator*, GtkMovementStep step, gint count);
#if 0
static gboolean rotator_real_select_all             (Rotator*);
static gboolean rotator_real_unselect_all           (Rotator*);
#endif
static gboolean rotator_real_select_cursor_row      (Rotator*, gboolean start_editing);
#if 0
static gboolean rotator_real_toggle_cursor_row      (Rotator*);
static gboolean rotator_real_select_cursor_parent   (Rotator*);
#endif
static void rotator_row_changed                     (GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);
static void rotator_row_inserted                    (GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);
static void rotator_row_deleted                     (GtkTreeModel*, GtkTreePath*, gpointer);
#if 0
static void rotator_row_has_child_toggled           (GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);
static void rotator_rows_reordered                  (GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gint* new_order, gpointer);

/* Incremental reflow */
static gboolean validate_row                (Rotator*, GtkRBTree*, GtkRBNode*, GtkTreeIter*, GtkTreePath*);
static void     validate_visible_area       (Rotator*);
static gboolean validate_rows_handler       (Rotator*);
static gboolean do_validate_rows            (Rotator*, gboolean size_request);
static gboolean validate_rows               (Rotator*);
static gboolean presize_handler_callback    (gpointer);
static void     install_presize_handler     (Rotator *tree_view);
static void     install_scroll_sync_handler (Rotator *tree_view);
static void     rotator_set_top_row         (Rotator *tree_view, GtkTreePath*, gint offset);
static void     rotator_top_row_to_dy       (Rotator *tree_view);
static void     invalidate_empty_focus      (Rotator *tree_view);

/* Internal functions */
static gboolean rotator_is_expander_column             (Rotator*, RotatorColumn*);
static void     rotator_add_move_binding               (GtkBindingSet*, guint keyval, guint modmask, gboolean add_shifted_binding, GtkMovementStep step, gint count);
static gint     rotator_unref_and_check_selection_tree (Rotator*, GtkRBTree*);
static void     rotator_queue_draw_path                (Rotator*, GtkTreePath*, const GdkRectangle* clip_rect);
static void     rotator_queue_draw_arrow               (Rotator*, GtkRBTree*, GtkRBNode*, const GdkRectangle* clip_rect);
static void     rotator_draw_arrow                     (Rotator*, GtkRBTree*, GtkRBNode*, gint x, gint y);
static void     rotator_get_arrow_xrange               (Rotator*, GtkRBTree*, gint* x1, gint* x2);
static gint     rotator_new_column_width               (Rotator*, gint i, gint* x);
static void     rotator_adjustment_changed             (GtkAdjustment*, Rotator*);
static void     rotator_build_tree                     (Rotator*, GtkRBTree*, GtkTreeIter*, gint depth, gboolean recurse);
static gboolean rotator_discover_dirty_iter            (Rotator*, GtkTreeIter*, gint depth, gint* height, GtkRBNode* node);
static void     rotator_discover_dirty                 (Rotator*, GtkRBTree*, GtkTreeIter*, gint depth);
static void     rotator_clamp_node_visible             (Rotator*, GtkRBTree*, GtkRBNode*);
static void     rotator_clamp_column_visible           (Rotator*, RotatorColumn*, gboolean focus_to_cell);
static gboolean rotator_maybe_begin_dragging_row       (Rotator*, GdkEventMotion*);
static void     rotator_focus_to_cursor                (Rotator*);
static void     rotator_move_cursor_up_down            (Rotator*, gint count);
static void     rotator_move_cursor_page_up_down       (Rotator*, gint count);
static void     rotator_move_cursor_left_right         (Rotator*, gint count);
static void     rotator_move_cursor_start_end          (Rotator*, gint count);
static gboolean rotator_real_collapse_row              (Rotator*, GtkTreePath*, GtkRBTree*, GtkRBNode*, gboolean animate);
static gboolean rotator_real_expand_row                (Rotator*, GtkTreePath*, GtkRBTree*, GtkRBNode*, gboolean open_all, gboolean animate);
static void     rotator_real_set_cursor                (Rotator*, GtkTreePath*, gboolean clear_and_select, gboolean clamp_node);
static gboolean rotator_has_special_cell               (Rotator*);
static void     column_sizing_notify                   (GObject*, GParamSpec*, gpointer);
static gboolean expand_collapse_timeout                (gpointer);
static void     add_expand_collapse_timeout            (Rotator*, GtkRBTree*, GtkRBNode*, gboolean expand);
static void     remove_expand_collapse_timeout         (Rotator*);
static void     cancel_arrow_animation                 (Rotator*);
static gboolean do_expand_collapse                     (Rotator*);
static void     rotator_stop_rubber_band               (Rotator*);

/* interactive search */
static void     rotator_ensure_interactive_directory (Rotator *tree_view);
static void     rotator_search_dialog_hide     (GtkWidget        *search_dialog, Rotator      *tree_view); static void     rotator_search_position_func      (Rotator      *tree_view, GtkWidget        *search_dialog, gpointer          user_data);
static void     rotator_search_disable_popdown    (GtkEntry         *entry, GtkMenu          *menu, gpointer          data);
static void     rotator_search_preedit_changed    (GtkIMContext     *im_context, Rotator      *tree_view);
static void     rotator_search_activate           (GtkEntry         *entry, Rotator      *tree_view);
static gboolean rotator_real_search_enable_popdown(gpointer          data);
static void     rotator_search_enable_popdown     (GtkWidget*, gpointer          data);
static gboolean rotator_search_delete_event       (GtkWidget*, GdkEventAny *event, Rotator *tree_view);
static gboolean rotator_search_scroll_event       (GtkWidget*, GdkEventScroll *event, Rotator *tree_view);
static gboolean rotator_search_key_press_event    (GtkWidget*, GdkEventKey *event, Rotator *tree_view);
static gboolean rotator_search_move               (GtkWidget*, Rotator*, gboolean up);
static gboolean rotator_search_equal_func         (GtkTreeModel*, gint column, const gchar *key, GtkTreeIter *iter, gpointer          search_data);
static gboolean rotator_search_iter               (GtkTreeModel*, GtkTreeSelection *selection, GtkTreeIter *iter, const gchar      *text, gint             *count, gint              n);
static void     rotator_search_init               (GtkWidget        *entry, Rotator      *tree_view);
static void     rotator_put                       (Rotator*, GtkWidget* child_widget, gint x, gint y, gint width, gint height);
static gboolean rotator_start_editing             (Rotator*, GtkTreePath* cursor);
static void     rotator_real_start_editing        (Rotator*, RotatorColumn*, GtkTreePath*, GtkCellEditable   *cell_editable, GdkRectangle      *cell_area, GdkEvent          *event, guint              flags);
static void     rotator_stop_editing                  (Rotator*, gboolean cancel_editing);
static gboolean rotator_real_start_interactive_search (Rotator*, gboolean keybinding);
static gboolean rotator_start_interactive_search      (Rotator*);
static RotatorColumn *rotator_get_drop_column         (Rotator*, RotatorColumn *column, gint               drop_position);
#endif

/* GtkBuildable */
#if 0
static void rotator_buildable_add_child (GtkBuildable *tree_view, GtkBuilder  *builder, GObject     *child, const gchar *type);
#endif
static void rotator_buildable_init      (GtkBuildableIface *iface);

#if 0
static void     add_scroll_timeout                   (Rotator *tree_view);
static void     remove_scroll_timeout                (Rotator *tree_view);

static guint tree_view_signals [LAST_SIGNAL] = { 0 };
#endif

extern Application* application;

typedef struct
{
	Sample*        sample;
	WaveformActor* actor;
} SampleActor;

static void on_canvas_realise        (GtkWidget*, gpointer);
static void on_allocate              (GtkWidget*, GtkAllocation*, gpointer);
static void rotator_setup_projection (AGlActor*);
static void start_zoom               (Rotator*, float target_zoom);
static void set_position             (WaveformActor*, int j);

float rotate[3] = {30.0, 30.0, 30.0};
float isometric_rotation[3] = {35.264f, 45.0f, 0.0f};
GdkGLConfig*     glconfig       = NULL;
static gboolean  gl_initialised = false;
float            dz             = 20.0;
float            zoom           = 1.0;


struct _RotatorPrivate
{
	GtkTreeModel* model;

	GList*          actors;       // SampleActor*
	GList*          pending_rows; // added but empty

	AGlScene*        scene;
	WaveformContext* wfc;

#if 0
  guint flags;
  /* tree information */
  GtkRBTree *tree;

  GtkRBNode *button_pressed_node;
  GtkRBTree *button_pressed_tree;

  GList *children;
#endif
  gint width;
  gint height;
#if 0
  gint expander_size;

  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;
#endif

#if 0
  GdkWindow *bin_window;
  GdkWindow *header_window;
  GdkWindow *drag_window;
  GdkWindow *drag_highlight_window;
  RotatorColumn *drag_column;

  GtkTreeRowReference *last_button_press;
  GtkTreeRowReference *last_button_press_2;

  /* bin_window offset */
  GtkTreeRowReference *top_row;
  gint top_row_dy;
  /* dy == y pos of top_row + top_row_dy */
  /* we cache it for simplicity of the code */
  gint dy;
  gint drag_column_x;
  gint cursor_offset;

  RotatorColumn *expander_column;
  RotatorColumn *edited_column;
  guint presize_handler_timer;
  guint validate_rows_timer;
  guint scroll_sync_timer;

  /* Focus code */
  RotatorColumn *focus_column;

  /* Selection stuff */
  GtkTreeRowReference *anchor;
  GtkTreeRowReference *cursor;

  /* Column Resizing */
  gint drag_pos;
  gint x_drag;

  /* Prelight information */
  GtkRBNode *prelight_node;
  GtkRBTree *prelight_tree;

  /* The node that's currently being collapsed or expanded */
  GtkRBNode *expanded_collapsed_node;
  GtkRBTree *expanded_collapsed_tree;
  guint expand_collapse_timeout;

  /* Selection information */
  GtkTreeSelection *selection;

  /* Header information */
  gint n_columns;
  GList *columns;
  gint header_height;

  RotatorColumnDropFunc column_drop_func;
  gpointer column_drop_func_data;
  GDestroyNotify column_drop_func_data_destroy;
  GList *column_drag_info;
  RotatorColumnReorder *cur_reorder;

  /* ATK Hack */
  GtkTreeDestroyCountFunc destroy_count_func;
  gpointer destroy_count_data;
  GDestroyNotify destroy_count_destroy;

  /* Scroll timeout (e.g. during dnd) */
  guint scroll_timeout;

  /* Row drag-and-drop */
  GtkTreeRowReference *drag_dest_row;
  RotatorDropPosition drag_dest_pos;
  guint open_dest_timeout;

  gint pressed_button;
  gint press_start_x;
  gint press_start_y;

  gint rubber_band_status;
  gint rubber_band_x;
  gint rubber_band_y;
  gint rubber_band_shift;
  gint rubber_band_ctrl;

  GtkRBNode *rubber_band_start_node;
  GtkRBTree *rubber_band_start_tree;

  GtkRBNode *rubber_band_end_node;
  GtkRBTree *rubber_band_end_tree;

  /* fixed height */
  gint fixed_height;

  /* Scroll-to functionality when unrealized */
  GtkTreeRowReference *scroll_to_path;
  RotatorColumn *scroll_to_column;
  gfloat scroll_to_row_align;
  gfloat scroll_to_col_align;
  guint scroll_to_use_align : 1;

  guint fixed_height_mode : 1;
  guint fixed_height_check : 1;

  guint reorderable : 1;
  guint header_has_focus : 1;
  guint drag_column_window_state : 3;
  /* hint to display rows in alternating colors */
  guint has_rules : 1;
  guint mark_rows_col_dirty : 1;

  /* for DnD */
  guint empty_view_drop : 1;

  guint ctrl_pressed : 1;
  guint shift_pressed : 1;


  guint init_hadjust_value : 1;

  guint in_top_row_to_dy : 1;

  /* interactive search */
  guint enable_search : 1;
  guint disable_popdown : 1;
  guint search_custom_entry_set : 1;
  
  guint hover_selection : 1;
  guint hover_expand : 1;
  guint imcontext_changed : 1;

  guint rubber_banding_enable : 1;

  guint in_grab : 1;

  guint post_validation_flag : 1;


  gint selected_iter;
  gint search_column;
  RotatorSearchPositionFunc search_position_func;
  RotatorSearchEqualFunc search_equal_func;
  gpointer search_user_data;
  GDestroyNotify search_destroy;
  gpointer search_position_user_data;
  GDestroyNotify search_position_destroy;
  GtkWidget *search_window;
  GtkWidget *search_entry;
  guint search_entry_changed_id;
  guint typeselect_flush_timeout;

  gint prev_width;

  RotatorRowSeparatorFunc row_separator_func;
  gpointer row_separator_data;
  GDestroyNotify row_separator_destroy;

  gint level_indentation;

  RotatorGridLines grid_lines;
  GdkGC *grid_line_gc;

  gint tooltip_column;

  gint last_extra_space;
  gint last_extra_space_per_column;
  gint last_number_of_expand_columns;
#endif
};




/* GType Methods
 */

G_DEFINE_TYPE_WITH_CODE (Rotator, rotator, GTK_TYPE_CONTAINER,
	G_ADD_PRIVATE (Rotator)
	G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, rotator_buildable_init)
)


static void
rotator_class_init (RotatorClass* class)
{
#if 0
	GtkBindingSet* binding_set = gtk_binding_set_by_class (class);
#endif

	GObjectClass* o_class = (GObjectClass *) class;
	GtkObjectClass* object_class = (GtkObjectClass *) class;
	GtkWidgetClass* widget_class = (GtkWidgetClass *) class;

	/* GObject signals */
	o_class->set_property = rotator_set_property;
	o_class->get_property = rotator_get_property;
	o_class->finalize = rotator_finalize;

	/* GtkObject signals */
	object_class->destroy = rotator_destroy;

#if 0
	/* GtkWidget signals */
	widget_class->map = rotator_map;
#endif
	widget_class->realize = rotator_realize;
	widget_class->unrealize = rotator_unrealize;
#if 0
	widget_class->size_request = rotator_size_request;
#endif
	widget_class->size_allocate = rotator_size_allocate;
	widget_class->button_press_event = rotator_button_press;
#if 0
	widget_class->button_release_event = rotator_button_release;
	widget_class->grab_broken_event = rotator_grab_broken;
	/*widget_class->configure_event = rotator_configure;*/
	widget_class->motion_notify_event = rotator_motion;
#endif
	widget_class->expose_event = rotator_expose;
#if 0
	widget_class->key_press_event = rotator_key_press;
	widget_class->key_release_event = rotator_key_release;
	widget_class->enter_notify_event = rotator_enter_notify;
	widget_class->leave_notify_event = rotator_leave_notify;
	widget_class->focus_out_event = rotator_focus_out;
	widget_class->drag_begin = rotator_drag_begin;
	widget_class->drag_end = rotator_drag_end;
	widget_class->drag_data_get = rotator_drag_data_get;
	widget_class->drag_data_delete = rotator_drag_data_delete;
	widget_class->drag_leave = rotator_drag_leave;
	widget_class->drag_motion = rotator_drag_motion;
	widget_class->drag_drop = rotator_drag_drop;
	widget_class->drag_data_received = rotator_drag_data_received;
	widget_class->focus = rotator_focus;
	widget_class->grab_focus = rotator_grab_focus;
	widget_class->style_set = rotator_style_set;
	widget_class->grab_notify = rotator_grab_notify;
	widget_class->state_changed = rotator_state_changed;

	/* GtkContainer signals */
	GtkContainerClass* container_class = (GtkContainerClass *) class;
	container_class->remove = rotator_remove;
	container_class->forall = rotator_forall;
	container_class->set_focus_child = rotator_set_focus_child;

	class->set_scroll_adjustments = rotator_set_adjustments;
#endif
	class->move_cursor = rotator_real_move_cursor;
#if 0
	class->select_all = rotator_real_select_all;
	class->unselect_all = rotator_real_unselect_all;
#endif
	class->select_cursor_row = rotator_real_select_cursor_row;
#if 0
	class->toggle_cursor_row = rotator_real_toggle_cursor_row;
	class->select_cursor_parent = rotator_real_select_cursor_parent;
	class->start_interactive_search = rotator_start_interactive_search;
#endif

	/* Properties */
	#define GTK_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB

	g_object_class_install_property (o_class,
	                                 PROP_MODEL,
	                                 g_param_spec_object ("model",
	                                 "TreeView Model",
	                                 "The model for the tree view",
	                                 GTK_TYPE_TREE_MODEL,
	                                 GTK_PARAM_READWRITE));

#if 0
  g_object_class_install_property (o_class,
                                   PROP_HADJUSTMENT,
                                   g_param_spec_object ("hadjustment",
							P_("Horizontal Adjustment"),
                                                        P_("Horizontal Adjustment for the widget"),
                                                        GTK_TYPE_ADJUSTMENT,
                                                        GTK_PARAM_READWRITE));

  g_object_class_install_property (o_class,
                                   PROP_VADJUSTMENT,
                                   g_param_spec_object ("vadjustment",
							P_("Vertical Adjustment"),
                                                        P_("Vertical Adjustment for the widget"),
                                                        GTK_TYPE_ADJUSTMENT,
                                                        GTK_PARAM_READWRITE));

  g_object_class_install_property (o_class,
                                   PROP_HEADERS_VISIBLE,
                                   g_param_spec_boolean ("headers-visible",
							 P_("Headers Visible"),
							 P_("Show the column header buttons"),
							 TRUE,
							 GTK_PARAM_READWRITE));

  g_object_class_install_property (o_class,
                                   PROP_HEADERS_CLICKABLE,
                                   g_param_spec_boolean ("headers-clickable",
							 P_("Headers Clickable"),
							 P_("Column headers respond to click events"),
							 TRUE,
							 GTK_PARAM_READWRITE));

  g_object_class_install_property (o_class,
                                   PROP_EXPANDER_COLUMN,
                                   g_param_spec_object ("expander-column",
							P_("Expander Column"),
							P_("Set the column for the expander column"),
							GTK_TYPE_ROTATOR_COLUMN,
							GTK_PARAM_READWRITE));

  g_object_class_install_property (o_class,
                                   PROP_REORDERABLE,
                                   g_param_spec_boolean ("reorderable",
							 P_("Reorderable"),
							 P_("View is reorderable"),
							 FALSE,
							 GTK_PARAM_READWRITE));

  g_object_class_install_property (o_class,
                                   PROP_RULES_HINT,
                                   g_param_spec_boolean ("rules-hint",
							 P_("Rules Hint"),
							 P_("Set a hint to the theme engine to draw rows in alternating colors"),
							 FALSE,
							 GTK_PARAM_READWRITE));

    g_object_class_install_property (o_class,
				     PROP_ENABLE_SEARCH,
				     g_param_spec_boolean ("enable-search",
							   P_("Enable Search"),
							   P_("View allows user to search through columns interactively"),
							   TRUE,
							   GTK_PARAM_READWRITE));

    g_object_class_install_property (o_class,
				     PROP_SEARCH_COLUMN,
				     g_param_spec_int ("search-column",
						       P_("Search Column"),
						       P_("Model column to search through during interactive search"),
						       -1,
						       G_MAXINT,
						       -1,
						       GTK_PARAM_READWRITE));

    /**
     * Rotator:fixed-height-mode:
     *
     * Setting the ::fixed-height-mode property to %TRUE speeds up 
     * #Rotator by assuming that all rows have the same height. 
     * Only enable this option if all rows are the same height.  
     * Please see rotator_set_fixed_height_mode() for more 
     * information on this option.
     *
     * Since: 2.4
     **/
    g_object_class_install_property (o_class,
                                     PROP_FIXED_HEIGHT_MODE,
                                     g_param_spec_boolean ("fixed-height-mode",
                                                           P_("Fixed Height Mode"),
                                                           P_("Speeds up Rotator by assuming that all rows have the same height"),
                                                           FALSE,
                                                           GTK_PARAM_READWRITE));
    
    /**
     * Rotator:hover-selection:
     * 
     * Enables of disables the hover selection mode of @tree_view.
     * Hover selection makes the selected row follow the pointer.
     * Currently, this works only for the selection modes 
     * %GTK_SELECTION_SINGLE and %GTK_SELECTION_BROWSE.
     *
     * This mode is primarily intended for treeviews in popups, e.g.
     * in #GtkComboBox or #GtkEntryCompletion.
     *
     * Since: 2.6
     */
    g_object_class_install_property (o_class,
                                     PROP_HOVER_SELECTION,
                                     g_param_spec_boolean ("hover-selection",
                                                           P_("Hover Selection"),
                                                           P_("Whether the selection should follow the pointer"),
                                                           FALSE,
                                                           GTK_PARAM_READWRITE));

    /**
     * Rotator:hover-expand:
     * 
     * Enables of disables the hover expansion mode of @tree_view.
     * Hover expansion makes rows expand or collapse if the pointer moves 
     * over them.
     *
     * This mode is primarily intended for treeviews in popups, e.g.
     * in #GtkComboBox or #GtkEntryCompletion.
     *
     * Since: 2.6
     */
    g_object_class_install_property (o_class,
                                     PROP_HOVER_EXPAND,
                                     g_param_spec_boolean ("hover-expand",
                                                           P_("Hover Expand"),
                                                           P_("Whether rows should be expanded/collapsed when the pointer moves over them"),
                                                           FALSE,
                                                           GTK_PARAM_READWRITE));

    /**
     * Rotator:show-expanders:
     *
     * %TRUE if the view has expanders.
     *
     * Since: 2.12
     */
    g_object_class_install_property (o_class,
				     PROP_SHOW_EXPANDERS,
				     g_param_spec_boolean ("show-expanders",
							   P_("Show Expanders"),
							   P_("View has expanders"),
							   TRUE,
							   GTK_PARAM_READWRITE));

    /**
     * Rotator:level-indentation:
     *
     * Extra indentation for each level.
     *
     * Since: 2.12
     */
    g_object_class_install_property (o_class,
				     PROP_LEVEL_INDENTATION,
				     g_param_spec_int ("level-indentation",
						       P_("Level Indentation"),
						       P_("Extra indentation for each level"),
						       0,
						       G_MAXINT,
						       0,
						       GTK_PARAM_READWRITE));

    g_object_class_install_property (o_class,
                                     PROP_RUBBER_BANDING,
                                     g_param_spec_boolean ("rubber-banding",
                                                           P_("Rubber Banding"),
                                                           P_("Whether to enable selection of multiple items by dragging the mouse pointer"),
                                                           FALSE,
                                                           GTK_PARAM_READWRITE));

    g_object_class_install_property (o_class,
                                     PROP_ENABLE_GRID_LINES,
                                     g_param_spec_enum ("enable-grid-lines",
							P_("Enable Grid Lines"),
							P_("Whether grid lines should be drawn in the tree view"),
							GTK_TYPE_ROTATOR_GRID_LINES,
							GTK_ROTATOR_GRID_LINES_NONE,
							GTK_PARAM_READWRITE));

    g_object_class_install_property (o_class,
                                     PROP_ENABLE_TREE_LINES,
                                     g_param_spec_boolean ("enable-tree-lines",
                                                           P_("Enable Tree Lines"),
                                                           P_("Whether tree lines should be drawn in the tree view"),
                                                           FALSE,
                                                           GTK_PARAM_READWRITE));

  /* Style properties */
#define _ROTATOR_EXPANDER_SIZE 12
#define _ROTATOR_VERTICAL_SEPARATOR 2
#define _ROTATOR_HORIZONTAL_SEPARATOR 2

  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("expander-size",
							     P_("Expander Size"),
							     P_("Size of the expander arrow"),
							     0,
							     G_MAXINT,
							     _ROTATOR_EXPANDER_SIZE,
							     GTK_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("vertical-separator",
							     P_("Vertical Separator Width"),
							     P_("Vertical space between cells.  Must be an even number"),
							     0,
							     G_MAXINT,
							     _ROTATOR_VERTICAL_SEPARATOR,
							     GTK_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("horizontal-separator",
							     P_("Horizontal Separator Width"),
							     P_("Horizontal space between cells.  Must be an even number"),
							     0,
							     G_MAXINT,
							     _ROTATOR_HORIZONTAL_SEPARATOR,
							     GTK_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("allow-rules",
								 P_("Allow Rules"),
								 P_("Allow drawing of alternating color rows"),
								 TRUE,
								 GTK_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("indent-expanders",
								 P_("Indent Expanders"),
								 P_("Make the expanders indented"),
								 TRUE,
								 GTK_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boxed ("even-row-color",
                                                               P_("Even Row Color"),
                                                               P_("Color to use for even rows"),
							       GDK_TYPE_COLOR,
							       GTK_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boxed ("odd-row-color",
                                                               P_("Odd Row Color"),
                                                               P_("Color to use for odd rows"),
							       GDK_TYPE_COLOR,
							       GTK_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("row-ending-details",
								 P_("Row Ending details"),
								 P_("Enable extended row background theming"),
								 FALSE,
								 GTK_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("grid-line-width",
							     P_("Grid line width"),
							     P_("Width, in pixels, of the tree view grid lines"),
							     0, G_MAXINT, 1,
							     GTK_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("tree-line-width",
							     P_("Tree line width"),
							     P_("Width, in pixels, of the tree view lines"),
							     0, G_MAXINT, 1,
							     GTK_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_string ("grid-line-pattern",
								P_("Grid line pattern"),
								P_("Dash pattern used to draw the tree view grid lines"),
								"\1\1",
								GTK_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_string ("tree-line-pattern",
								P_("Tree line pattern"),
								P_("Dash pattern used to draw the tree view lines"),
								"\1\1",
								GTK_PARAM_READABLE));

  /* Signals */
  /**
   * Rotator::set-scroll-adjustments
   * @horizontal: the horizontal #GtkAdjustment
   * @vertical: the vertical #GtkAdjustment
   *
   * Set the scroll adjustments for the tree view. Usually scrolled containers
   * like #GtkScrolledWindow will emit this signal to connect two instances
   * of #GtkScrollbar to the scroll directions of the #Rotator.
   */
  widget_class->set_scroll_adjustments_signal =
    g_signal_new (I_("set-scroll-adjustments"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (RotatorClass, set_scroll_adjustments),
		  NULL, NULL,
		  _gtk_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2,
		  GTK_TYPE_ADJUSTMENT,
		  GTK_TYPE_ADJUSTMENT);

  /**
   * Rotator::row-activated:
   * @tree_view: the object on which the signal is emitted
   * @path: the #GtkTreePath for the activated row
   * @column: the #RotatorColumn in which the activation occurred
   *
   * The "row-activated" signal is emitted when the method
   * rotator_row_activated() is called or the user double clicks 
   * a treeview row. It is also emitted when a non-editable row is 
   * selected and one of the keys: Space, Shift+Space, Return or 
   * Enter is pressed.
   * 
   * For selection handling refer to the <link linkend="TreeWidget">tree 
   * widget conceptual overview</link> as well as #GtkTreeSelection.
   */
  tree_view_signals[ROW_ACTIVATED] =
    g_signal_new (I_("row-activated"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (RotatorClass, row_activated),
		  NULL, NULL,
		  _gtk_marshal_VOID__BOXED_OBJECT,
		  G_TYPE_NONE, 2,
		  GTK_TYPE_TREE_PATH,
		  GTK_TYPE_ROTATOR_COLUMN);

  /**
   * Rotator::test-expand-row:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the row to expand
   * @path: a tree path that points to the row 
   * 
   * The given row is about to be expanded (show its children nodes). Use this
   * signal if you need to control the expandability of individual rows.
   *
   * Returns: %FALSE to allow expansion, %TRUE to reject
   */
  tree_view_signals[TEST_EXPAND_ROW] =
    g_signal_new (I_("test-expand-row"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (RotatorClass, test_expand_row),
		  _gtk_boolean_handled_accumulator, NULL,
		  _gtk_marshal_BOOLEAN__BOXED_BOXED,
		  G_TYPE_BOOLEAN, 2,
		  GTK_TYPE_TREE_ITER,
		  GTK_TYPE_TREE_PATH);

  /**
   * Rotator::test-collapse-row:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the row to collapse
   * @path: a tree path that points to the row 
   * 
   * The given row is about to be collapsed (hide its children nodes). Use this
   * signal if you need to control the collapsibility of individual rows.
   *
   * Returns: %FALSE to allow collapsing, %TRUE to reject
   */
  tree_view_signals[TEST_COLLAPSE_ROW] =
    g_signal_new (I_("test-collapse-row"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (RotatorClass, test_collapse_row),
		  _gtk_boolean_handled_accumulator, NULL,
		  _gtk_marshal_BOOLEAN__BOXED_BOXED,
		  G_TYPE_BOOLEAN, 2,
		  GTK_TYPE_TREE_ITER,
		  GTK_TYPE_TREE_PATH);

  /**
   * Rotator::row-expanded:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the expanded row
   * @path: a tree path that points to the row 
   * 
   * The given row has been expanded (child nodes are shown).
   */
  tree_view_signals[ROW_EXPANDED] =
    g_signal_new (I_("row-expanded"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (RotatorClass, row_expanded),
		  NULL, NULL,
		  _gtk_marshal_VOID__BOXED_BOXED,
		  G_TYPE_NONE, 2,
		  GTK_TYPE_TREE_ITER,
		  GTK_TYPE_TREE_PATH);

  /**
   * Rotator::row-collapsed:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the collapsed row
   * @path: a tree path that points to the row 
   * 
   * The given row has been collapsed (child nodes are hidden).
   */
  tree_view_signals[ROW_COLLAPSED] =
    g_signal_new (I_("row-collapsed"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (RotatorClass, row_collapsed),
		  NULL, NULL,
		  _gtk_marshal_VOID__BOXED_BOXED,
		  G_TYPE_NONE, 2,
		  GTK_TYPE_TREE_ITER,
		  GTK_TYPE_TREE_PATH);

  /**
   * Rotator::columns-changed:
   * @tree_view: the object on which the signal is emitted 
   * 
   * The number of columns of the treeview has changed.
   */
  tree_view_signals[COLUMNS_CHANGED] =
    g_signal_new (I_("columns-changed"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (RotatorClass, columns_changed),
		  NULL, NULL,
		  _gtk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * Rotator::cursor-changed:
   * @tree_view: the object on which the signal is emitted
   * 
   * The position of the cursor (focused cell) has changed.
   */
  tree_view_signals[CURSOR_CHANGED] =
    g_signal_new (I_("cursor-changed"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (RotatorClass, cursor_changed),
		  NULL, NULL,
		  _gtk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  tree_view_signals[MOVE_CURSOR] =
    g_signal_new (I_("move-cursor"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (RotatorClass, move_cursor),
		  NULL, NULL,
		  _gtk_marshal_BOOLEAN__ENUM_INT,
		  G_TYPE_BOOLEAN, 2,
		  GTK_TYPE_MOVEMENT_STEP,
		  G_TYPE_INT);

  tree_view_signals[SELECT_ALL] =
    g_signal_new (I_("select-all"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (RotatorClass, select_all),
		  NULL, NULL,
		  _gtk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);

  tree_view_signals[UNSELECT_ALL] =
    g_signal_new (I_("unselect-all"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (RotatorClass, unselect_all),
		  NULL, NULL,
		  _gtk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);

  tree_view_signals[SELECT_CURSOR_ROW] =
    g_signal_new (I_("select-cursor-row"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (RotatorClass, select_cursor_row),
		  NULL, NULL,
		  _gtk_marshal_BOOLEAN__BOOLEAN,
		  G_TYPE_BOOLEAN, 1,
		  G_TYPE_BOOLEAN);

  tree_view_signals[TOGGLE_CURSOR_ROW] =
    g_signal_new (I_("toggle-cursor-row"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (RotatorClass, toggle_cursor_row),
		  NULL, NULL,
		  _gtk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);

  tree_view_signals[EXPAND_COLLAPSE_CURSOR_ROW] =
    g_signal_new (I_("expand-collapse-cursor-row"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (RotatorClass, expand_collapse_cursor_row),
		  NULL, NULL,
		  _gtk_marshal_BOOLEAN__BOOLEAN_BOOLEAN_BOOLEAN,
		  G_TYPE_BOOLEAN, 3,
		  G_TYPE_BOOLEAN,
		  G_TYPE_BOOLEAN,
		  G_TYPE_BOOLEAN);

  tree_view_signals[SELECT_CURSOR_PARENT] =
    g_signal_new (I_("select-cursor-parent"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (RotatorClass, select_cursor_parent),
		  NULL, NULL,
		  _gtk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);

  tree_view_signals[START_INTERACTIVE_SEARCH] =
    g_signal_new (I_("start-interactive-search"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (RotatorClass, start_interactive_search),
		  NULL, NULL,
		  _gtk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);

  /* Key bindings */
  rotator_add_move_binding (binding_set, GDK_Up, 0, TRUE,
				  GTK_MOVEMENT_DISPLAY_LINES, -1);
  rotator_add_move_binding (binding_set, GDK_KP_Up, 0, TRUE,
				  GTK_MOVEMENT_DISPLAY_LINES, -1);

  rotator_add_move_binding (binding_set, GDK_Down, 0, TRUE,
				  GTK_MOVEMENT_DISPLAY_LINES, 1);
  rotator_add_move_binding (binding_set, GDK_KP_Down, 0, TRUE,
				  GTK_MOVEMENT_DISPLAY_LINES, 1);

  rotator_add_move_binding (binding_set, GDK_p, GDK_CONTROL_MASK, FALSE,
				  GTK_MOVEMENT_DISPLAY_LINES, -1);

  rotator_add_move_binding (binding_set, GDK_n, GDK_CONTROL_MASK, FALSE,
				  GTK_MOVEMENT_DISPLAY_LINES, 1);

  rotator_add_move_binding (binding_set, GDK_Home, 0, TRUE,
				  GTK_MOVEMENT_BUFFER_ENDS, -1);
  rotator_add_move_binding (binding_set, GDK_KP_Home, 0, TRUE,
				  GTK_MOVEMENT_BUFFER_ENDS, -1);

  rotator_add_move_binding (binding_set, GDK_End, 0, TRUE,
				  GTK_MOVEMENT_BUFFER_ENDS, 1);
  rotator_add_move_binding (binding_set, GDK_KP_End, 0, TRUE,
				  GTK_MOVEMENT_BUFFER_ENDS, 1);

  rotator_add_move_binding (binding_set, GDK_Page_Up, 0, TRUE,
				  GTK_MOVEMENT_PAGES, -1);
  rotator_add_move_binding (binding_set, GDK_KP_Page_Up, 0, TRUE,
				  GTK_MOVEMENT_PAGES, -1);

  rotator_add_move_binding (binding_set, GDK_Page_Down, 0, TRUE,
				  GTK_MOVEMENT_PAGES, 1);
  rotator_add_move_binding (binding_set, GDK_KP_Page_Down, 0, TRUE,
				  GTK_MOVEMENT_PAGES, 1);


  gtk_binding_entry_add_signal (binding_set, GDK_Right, 0, "move-cursor", 2,
				G_TYPE_ENUM, GTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, 1);

  gtk_binding_entry_add_signal (binding_set, GDK_Left, 0, "move-cursor", 2,
				G_TYPE_ENUM, GTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, -1);

  gtk_binding_entry_add_signal (binding_set, GDK_KP_Right, 0, "move-cursor", 2,
				G_TYPE_ENUM, GTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, 1);

  gtk_binding_entry_add_signal (binding_set, GDK_KP_Left, 0, "move-cursor", 2,
				G_TYPE_ENUM, GTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, -1);

  gtk_binding_entry_add_signal (binding_set, GDK_Right, GDK_CONTROL_MASK,
                                "move-cursor", 2,
				G_TYPE_ENUM, GTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, 1);

  gtk_binding_entry_add_signal (binding_set, GDK_Left, GDK_CONTROL_MASK,
                                "move-cursor", 2,
				G_TYPE_ENUM, GTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, -1);

  gtk_binding_entry_add_signal (binding_set, GDK_KP_Right, GDK_CONTROL_MASK,
                                "move-cursor", 2,
				G_TYPE_ENUM, GTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, 1);

  gtk_binding_entry_add_signal (binding_set, GDK_KP_Left, GDK_CONTROL_MASK,
                                "move-cursor", 2,
				G_TYPE_ENUM, GTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, -1);

  gtk_binding_entry_add_signal (binding_set, GDK_space, GDK_CONTROL_MASK, "toggle-cursor-row", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Space, GDK_CONTROL_MASK, "toggle-cursor-row", 0);

  gtk_binding_entry_add_signal (binding_set, GDK_a, GDK_CONTROL_MASK, "select-all", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_slash, GDK_CONTROL_MASK, "select-all", 0);

  gtk_binding_entry_add_signal (binding_set, GDK_A, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "unselect-all", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_backslash, GDK_CONTROL_MASK, "unselect-all", 0);

  gtk_binding_entry_add_signal (binding_set, GDK_space, GDK_SHIFT_MASK, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Space, GDK_SHIFT_MASK, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);

  gtk_binding_entry_add_signal (binding_set, GDK_space, 0, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Space, 0, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_Return, 0, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_ISO_Enter, 0, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Enter, 0, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);

  /* expand and collapse rows */
  gtk_binding_entry_add_signal (binding_set, GDK_plus, 0, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, FALSE);

  gtk_binding_entry_add_signal (binding_set, GDK_asterisk, 0,
                                "expand-collapse-cursor-row", 3,
                                G_TYPE_BOOLEAN, TRUE,
                                G_TYPE_BOOLEAN, TRUE,
                                G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Multiply, 0,
                                "expand-collapse-cursor-row", 3,
                                G_TYPE_BOOLEAN, TRUE,
                                G_TYPE_BOOLEAN, TRUE,
                                G_TYPE_BOOLEAN, TRUE);

  gtk_binding_entry_add_signal (binding_set, GDK_slash, 0,
                                "expand-collapse-cursor-row", 3,
                                G_TYPE_BOOLEAN, TRUE,
                                G_TYPE_BOOLEAN, FALSE,
                                G_TYPE_BOOLEAN, FALSE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Divide, 0,
                                "expand-collapse-cursor-row", 3,
                                G_TYPE_BOOLEAN, TRUE,
                                G_TYPE_BOOLEAN, FALSE,
                                G_TYPE_BOOLEAN, FALSE);

  /* Not doable on US keyboards */
  gtk_binding_entry_add_signal (binding_set, GDK_plus, GDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Add, 0, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, FALSE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Add, GDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Add, GDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_Right, GDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Right, GDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_Right,
                                GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Right,
                                GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);

  gtk_binding_entry_add_signal (binding_set, GDK_minus, 0, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, FALSE);
  gtk_binding_entry_add_signal (binding_set, GDK_minus, GDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Subtract, 0, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, FALSE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Subtract, GDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_Left, GDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Left, GDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_Left,
                                GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Left,
                                GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE);

  gtk_binding_entry_add_signal (binding_set, GDK_BackSpace, 0, "select-cursor-parent", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_BackSpace, GDK_CONTROL_MASK, "select-cursor-parent", 0);

  gtk_binding_entry_add_signal (binding_set, GDK_f, GDK_CONTROL_MASK, "start-interactive-search", 0);

  gtk_binding_entry_add_signal (binding_set, GDK_F, GDK_CONTROL_MASK, "start-interactive-search", 0);
#endif
}

static void
rotator_buildable_init (GtkBuildableIface *iface)
{
#if 0
	iface->add_child = rotator_buildable_add_child;
#endif
}

static gboolean
__init (Rotator* tree_view)
{
	dbg(2, "...");

	{
		gtk_gl_init(NULL, NULL);
		if(_debug_){
			gint major, minor;
#ifdef USE_SYSTEM_GTKGLEXT
			gdk_gl_query_version (&major, &minor);
#else
			glXQueryVersion (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), &major, &minor);
#endif
			g_print ("GtkGLExt version %d.%d\n", major, minor);
		}
	}

	if(!(glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE))){
		gerr ("Cannot initialise gtkglext.");
		return false;
	}

	return true;
}


static void
rotator_init (Rotator* tree_view)
{
	GtkWidget* widget = (GtkWidget*)tree_view;

	RotatorPrivate* _r = tree_view->priv = rotator_get_instance_private(tree_view);

	GTK_WIDGET_SET_FLAGS (tree_view, GTK_CAN_FOCUS);

	gtk_widget_set_redraw_on_allocate (GTK_WIDGET (tree_view), FALSE);

#if 0
	tree_view->priv->flags =  GTK_ROTATOR_SHOW_EXPANDERS
                            | GTK_ROTATOR_DRAW_KEYFOCUS
                            | GTK_ROTATOR_HEADERS_VISIBLE;

	tree_view->priv->dy = 0;
	tree_view->priv->cursor_offset = 0;
	tree_view->priv->n_columns = 0;
	tree_view->priv->header_height = 1;
	tree_view->priv->x_drag = 0;
	tree_view->priv->drag_pos = -1;
	tree_view->priv->header_has_focus = FALSE;
	tree_view->priv->pressed_button = -1;
	tree_view->priv->press_start_x = -1;
	tree_view->priv->press_start_y = -1;
	tree_view->priv->reorderable = FALSE;
	tree_view->priv->presize_handler_timer = 0;
	tree_view->priv->scroll_sync_timer = 0;
	tree_view->priv->fixed_height = -1;
	tree_view->priv->fixed_height_mode = FALSE;
	tree_view->priv->fixed_height_check = 0;
	rotator_set_adjustments (tree_view, NULL, NULL);
	tree_view->priv->selection = _gtk_tree_selection_new_with_tree_view (tree_view);
	tree_view->priv->enable_search = TRUE;
	tree_view->priv->search_column = -1;
	tree_view->priv->search_position_func = rotator_search_position_func;
	tree_view->priv->search_equal_func = rotator_search_equal_func;
	tree_view->priv->search_custom_entry_set = FALSE;
	tree_view->priv->typeselect_flush_timeout = 0;
	tree_view->priv->init_hadjust_value = TRUE;    
	tree_view->priv->width = 0;
		  
	tree_view->priv->hover_selection = FALSE;
	tree_view->priv->hover_expand = FALSE;

	tree_view->priv->level_indentation = 0;

	tree_view->priv->rubber_banding_enable = FALSE;

	tree_view->priv->grid_lines = GTK_ROTATOR_GRID_LINES_NONE;
	tree_view->priv->tree_lines_enabled = FALSE;

	tree_view->priv->tooltip_column = -1;

	tree_view->priv->post_validation_flag = FALSE;
#endif

	g_return_if_fail(glconfig || __init(tree_view));
	gtk_widget_set_gl_capability((GtkWidget*)widget, glconfig, agl_get_gl_context(), 1, GDK_GL_RGBA_TYPE);

	_r->wfc = wf_context_new((AGlActor*)(_r->scene = (AGlScene*)agl_actor__new_root(widget)));

	_r->scene->user_data = widget;

	typedef struct {
		Rotator* rotator;
		WaveformActor* actor;
	} C;

	GList* g_list_move_to_front (GList* list, GList* front)
	{
		if(!front || !front->prev) return list;

		GList* prev = front->prev;
		prev->next = NULL;
		front->prev = NULL;

		return g_list_concat(front, list);
	}

	void fade_out_done (WaveformActor* actor, gpointer user_data)
	{
		Rotator* rotator = user_data;

		bool fade_out_really_done (C* c)
		{
			Rotator* r = c->rotator;
			WaveformActor* actor = c->actor;

			((AGlActor*)actor)->root->enable_animations = false;
			int i = 0;
			for(GList* l=r->priv->actors;l;i++,l=l->next){
				wf_actor_set_z (((SampleActor*)l->data)->actor, -i * dz, NULL, NULL);
			}
			((AGlActor*)actor)->root->enable_animations = true;

			wf_actor_fade_in(c->actor, 1.0f, NULL, NULL);

			g_free(c);

			return G_SOURCE_REMOVE;
		}
		g_idle_add((GSourceFunc)fade_out_really_done, AGL_NEW(C, .rotator = rotator, .actor = actor));
	}

	void rotator_on_selection_change (SamplecatModel* m, Sample* sample, gpointer _rotator)
	{
		PF;
		Rotator* rotator = _rotator;
		RotatorPrivate* _r = rotator->priv;

		int compare (gconstpointer item, gconstpointer sample)
		{
			return (((SampleActor*)item)->sample == sample) ? 0 : 1;
		}

		GList* front = g_list_find_custom(_r->actors, sample, compare);

		int j = 0;
		for(GList* l = _r->actors;l!=front;l=l->next) j++;

		float _dz = 0;
		bool forward = j < g_list_length(_r->actors) / 2;
		if(forward){
			// rotating forward

			for(GList* l = _r->actors; l != front; l = l->next){
				wf_actor_fade_out(((SampleActor*)l->data)->actor, fade_out_done, rotator);
				_dz += dz;
			}
		}else{
			// rotating backward

			for(GList* l = front; l; l = l->next){
				wf_actor_fade_out(((SampleActor*)l->data)->actor, fade_out_done, rotator);
				_dz += dz;
			}
		}

		int i = 0;
		for(GList* l=_r->actors;l;i++,l=l->next){
			wf_actor_set_z (((SampleActor*)l->data)->actor, (forward ? _dz : -_dz) - i * dz, NULL, NULL);
		}

		_r->actors = g_list_move_to_front(_r->actors, front);
	}

	g_signal_connect((gpointer)samplecat.model, "selection-changed", G_CALLBACK(rotator_on_selection_change), tree_view);
}


/* GObject Methods
 */

static void
rotator_set_property (GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
  Rotator* tree_view = GTK_ROTATOR (object);

  switch (prop_id) {
    case PROP_MODEL:
      rotator_set_model (tree_view, g_value_get_object (value));
      break;
#if 0
    case PROP_HADJUSTMENT:
      rotator_set_hadjustment (tree_view, g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      rotator_set_vadjustment (tree_view, g_value_get_object (value));
      break;
    case PROP_HEADERS_VISIBLE:
      rotator_set_headers_visible (tree_view, g_value_get_boolean (value));
      break;
    case PROP_HEADERS_CLICKABLE:
      rotator_set_headers_clickable (tree_view, g_value_get_boolean (value));
      break;
    case PROP_EXPANDER_COLUMN:
      rotator_set_expander_column (tree_view, g_value_get_object (value));
      break;
    case PROP_REORDERABLE:
      rotator_set_reorderable (tree_view, g_value_get_boolean (value));
      break;
    case PROP_RULES_HINT:
      rotator_set_rules_hint (tree_view, g_value_get_boolean (value));
      break;
    case PROP_ENABLE_SEARCH:
      rotator_set_enable_search (tree_view, g_value_get_boolean (value));
      break;
    case PROP_SEARCH_COLUMN:
      rotator_set_search_column (tree_view, g_value_get_int (value));
      break;
    case PROP_FIXED_HEIGHT_MODE:
      rotator_set_fixed_height_mode (tree_view, g_value_get_boolean (value));
      break;
    case PROP_HOVER_SELECTION:
      tree_view->priv->hover_selection = g_value_get_boolean (value);
      break;
    case PROP_HOVER_EXPAND:
      tree_view->priv->hover_expand = g_value_get_boolean (value);
      break;
    case PROP_SHOW_EXPANDERS:
      rotator_set_show_expanders (tree_view, g_value_get_boolean (value));
      break;
    case PROP_LEVEL_INDENTATION:
      tree_view->priv->level_indentation = g_value_get_int (value);
      break;
    case PROP_RUBBER_BANDING:
      tree_view->priv->rubber_banding_enable = g_value_get_boolean (value);
      break;
    case PROP_ENABLE_GRID_LINES:
      rotator_set_grid_lines (tree_view, g_value_get_enum (value));
      break;
    case PROP_ENABLE_TREE_LINES:
      rotator_set_enable_tree_lines (tree_view, g_value_get_boolean (value));
      break;
#endif
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
rotator_get_property (GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
#if 0
  Rotator *tree_view;

  tree_view = GTK_ROTATOR (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, tree_view->priv->model);
      break;
    case PROP_HADJUSTMENT:
      g_value_set_object (value, tree_view->priv->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, tree_view->priv->vadjustment);
      break;
    case PROP_HEADERS_VISIBLE:
      g_value_set_boolean (value, rotator_get_headers_visible (tree_view));
      break;
    case PROP_HEADERS_CLICKABLE:
      g_value_set_boolean (value, rotator_get_headers_clickable (tree_view));
      break;
    case PROP_EXPANDER_COLUMN:
      g_value_set_object (value, tree_view->priv->expander_column);
      break;
    case PROP_REORDERABLE:
      g_value_set_boolean (value, tree_view->priv->reorderable);
      break;
    case PROP_RULES_HINT:
      g_value_set_boolean (value, tree_view->priv->has_rules);
      break;
    case PROP_ENABLE_SEARCH:
      g_value_set_boolean (value, tree_view->priv->enable_search);
      break;
    case PROP_SEARCH_COLUMN:
      g_value_set_int (value, tree_view->priv->search_column);
      break;
    case PROP_FIXED_HEIGHT_MODE:
      g_value_set_boolean (value, tree_view->priv->fixed_height_mode);
      break;
    case PROP_HOVER_SELECTION:
      g_value_set_boolean (value, tree_view->priv->hover_selection);
      break;
    case PROP_HOVER_EXPAND:
      g_value_set_boolean (value, tree_view->priv->hover_expand);
      break;
    case PROP_SHOW_EXPANDERS:
      g_value_set_boolean (value, GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_SHOW_EXPANDERS));
      break;
    case PROP_LEVEL_INDENTATION:
      g_value_set_int (value, tree_view->priv->level_indentation);
      break;
    case PROP_RUBBER_BANDING:
      g_value_set_boolean (value, tree_view->priv->rubber_banding_enable);
      break;
    case PROP_ENABLE_GRID_LINES:
      g_value_set_enum (value, tree_view->priv->grid_lines);
      break;
    case PROP_ENABLE_TREE_LINES:
      g_value_set_boolean (value, tree_view->priv->tree_lines_enabled);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
#endif
}

static void
rotator_finalize (GObject *object)
{
  G_OBJECT_CLASS (rotator_parent_class)->finalize (object);
}

#if 0


static void
rotator_buildable_add_child (GtkBuildable *tree_view, GtkBuilder* builder, GObject *child, const gchar *type)
{
  rotator_append_column (GTK_ROTATOR (tree_view), GTK_ROTATOR_COLUMN (child));
}

/* GtkObject Methods
 */

static void
rotator_free_rbtree (Rotator *tree_view)
{
  _gtk_rbtree_free (tree_view->priv->tree);
  
  tree_view->priv->tree = NULL;
  tree_view->priv->button_pressed_node = NULL;
  tree_view->priv->button_pressed_tree = NULL;
  tree_view->priv->prelight_tree = NULL;
  tree_view->priv->prelight_node = NULL;
  tree_view->priv->expanded_collapsed_node = NULL;
  tree_view->priv->expanded_collapsed_tree = NULL;
}
#endif


static void
rotator_destroy (GtkObject* object)
{
  Rotator* tree_view = GTK_ROTATOR (object);
#if 0
  GList *list;

  rotator_stop_editing (tree_view, TRUE);

  if (tree_view->priv->columns != NULL)
    {
      list = tree_view->priv->columns;
      while (list)
	{
	  RotatorColumn *column;
	  column = GTK_ROTATOR_COLUMN (list->data);
	  list = list->next;
	  rotator_remove_column (tree_view, column);
	}
      tree_view->priv->columns = NULL;
    }

  if (tree_view->priv->tree != NULL)
    {
      rotator_unref_and_check_selection_tree (tree_view, tree_view->priv->tree);

      rotator_free_rbtree (tree_view);
    }

  if (tree_view->priv->selection != NULL)
    {
      _gtk_tree_selection_set_tree_view (tree_view->priv->selection, NULL);
      g_object_unref (tree_view->priv->selection);
      tree_view->priv->selection = NULL;
    }

  if (tree_view->priv->scroll_to_path != NULL)
    {
      gtk_tree_row_reference_free (tree_view->priv->scroll_to_path);
      tree_view->priv->scroll_to_path = NULL;
    }

  if (tree_view->priv->drag_dest_row != NULL)
    {
      gtk_tree_row_reference_free (tree_view->priv->drag_dest_row);
      tree_view->priv->drag_dest_row = NULL;
    }

  if (tree_view->priv->last_button_press != NULL)
    {
      gtk_tree_row_reference_free (tree_view->priv->last_button_press);
      tree_view->priv->last_button_press = NULL;
    }

  if (tree_view->priv->last_button_press_2 != NULL)
    {
      gtk_tree_row_reference_free (tree_view->priv->last_button_press_2);
      tree_view->priv->last_button_press_2 = NULL;
    }

  if (tree_view->priv->top_row != NULL)
    {
      gtk_tree_row_reference_free (tree_view->priv->top_row);
      tree_view->priv->top_row = NULL;
    }

  if (tree_view->priv->column_drop_func_data &&
      tree_view->priv->column_drop_func_data_destroy)
    {
      tree_view->priv->column_drop_func_data_destroy (tree_view->priv->column_drop_func_data);
      tree_view->priv->column_drop_func_data = NULL;
    }

  if (tree_view->priv->destroy_count_destroy &&
      tree_view->priv->destroy_count_data)
    {
      tree_view->priv->destroy_count_destroy (tree_view->priv->destroy_count_data);
      tree_view->priv->destroy_count_data = NULL;
    }

  gtk_tree_row_reference_free (tree_view->priv->cursor);
  tree_view->priv->cursor = NULL;

  gtk_tree_row_reference_free (tree_view->priv->anchor);
  tree_view->priv->anchor = NULL;

  /* destroy interactive search dialog */
  if (tree_view->priv->search_window)
    {
      gtk_widget_destroy (tree_view->priv->search_window);
      tree_view->priv->search_window = NULL;
      tree_view->priv->search_entry = NULL;
      if (tree_view->priv->typeselect_flush_timeout)
	{
	  g_source_remove (tree_view->priv->typeselect_flush_timeout);
	  tree_view->priv->typeselect_flush_timeout = 0;
	}
    }

  if (tree_view->priv->search_destroy && tree_view->priv->search_user_data)
    {
      tree_view->priv->search_destroy (tree_view->priv->search_user_data);
      tree_view->priv->search_user_data = NULL;
    }

  if (tree_view->priv->search_position_destroy && tree_view->priv->search_position_user_data)
    {
      tree_view->priv->search_position_destroy (tree_view->priv->search_position_user_data);
      tree_view->priv->search_position_user_data = NULL;
    }

  if (tree_view->priv->row_separator_destroy && tree_view->priv->row_separator_data)
    {
      tree_view->priv->row_separator_destroy (tree_view->priv->row_separator_data);
      tree_view->priv->row_separator_data = NULL;
    }
#endif
 
  rotator_set_model (tree_view, NULL);

#if 0
  if (tree_view->priv->hadjustment)
    {
      g_object_unref (tree_view->priv->hadjustment);
      tree_view->priv->hadjustment = NULL;
    }
  if (tree_view->priv->vadjustment)
    {
      g_object_unref (tree_view->priv->vadjustment);
      tree_view->priv->vadjustment = NULL;
    }
#endif

  GTK_OBJECT_CLASS (rotator_parent_class)->destroy (object);
}

#if 0

/* GtkWidget Methods
 */

/* GtkWidget::map helper */
static void
rotator_map_buttons (Rotator *tree_view)
{
  GList *list;

  g_return_if_fail (GTK_WIDGET_MAPPED (tree_view));

  if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_HEADERS_VISIBLE))
    {
      RotatorColumn *column;

      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  column = list->data;
          if (GTK_WIDGET_VISIBLE (column->button) &&
              !GTK_WIDGET_MAPPED (column->button))
            gtk_widget_map (column->button);
	}
      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  column = list->data;
	  if (column->visible == FALSE)
	    continue;
	  if (column->resizable)
	    {
	      gdk_window_raise (column->window);
	      gdk_window_show (column->window);
	    }
	  else
	    gdk_window_hide (column->window);
	}
      gdk_window_show (tree_view->priv->header_window);
    }
}

static void
rotator_map (GtkWidget *widget)
{
  Rotator *tree_view = GTK_ROTATOR (widget);
  GList *tmp_list;

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

  tmp_list = tree_view->priv->children;
  while (tmp_list)
    {
      RotatorChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  if (!GTK_WIDGET_MAPPED (child->widget))
	    gtk_widget_map (child->widget);
	}
    }
  gdk_window_show (tree_view->priv->bin_window);

  rotator_map_buttons (tree_view);

  gdk_window_show (widget->window);
}
#endif


static void
_add_by_iter(Rotator* tree_view, GtkTreeIter* iter)
{
	static int i = 0;
	static uint32_t colours[] = {0x66eeffff, 0xffff00ff, 0xff9933ff, 0x66ff66ff, 0xccccccff, 0x0000ffff};

	RotatorPrivate* _r = tree_view->priv;

	Sample* sample = samplecat_list_store_get_sample_by_iter(iter);

	if(!sample->online) return;

	Waveform* w = waveform_load_new(sample->full_path);
	int n_frames = waveform_get_n_frames(w);
	dbg(0, "  %s %i", sample->name, n_frames);
	WaveformActor* a = wf_context_add_new_actor (_r->wfc, w);
	if (a) {
		agl_actor__add_child((AGlActor*)_r->scene, (AGlActor*)a);

		_r->actors = g_list_append(_r->actors, (SampleActor*)AGL_NEW(SampleActor,
			.sample = sample,
			.actor = a
		));

		WfSampleRegion region = {0, n_frames};

		wf_actor_set_region (a, &region); // TODO why needed? why doesnt it have a default?
		wf_actor_set_colour (a, colours[i++ % G_N_ELEMENTS(colours)]);

		sample_unref(sample);
	}

	start_zoom(tree_view, zoom);
}


static void
_add_all (Rotator* rotator)
{
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter_first(rotator->priv->model, &iter)){
		do {
			_add_by_iter(rotator, &iter);
		} while(gtk_tree_model_iter_next(rotator->priv->model, &iter));
	}
}


static void
rotator_realize (GtkWidget* widget)
{
	//Rotator* tree_view = GTK_ROTATOR (widget);

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	/* Make the main, clipping window */
	GdkWindowAttr attributes = {
		.window_type = GDK_WINDOW_CHILD,
		.x = widget->allocation.x,
		.y = widget->allocation.y,
		.width = widget->allocation.width,
		.height = widget->allocation.height,
		.wclass = GDK_INPUT_OUTPUT,
		.visual = gtk_widget_get_visual (widget),
		.colormap = gtk_widget_get_colormap (widget),
		.event_mask = (GDK_VISIBILITY_NOTIFY_MASK | GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | gtk_widget_get_events (widget))
	};

	gint attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
	gdk_window_set_user_data (widget->window, widget);

#if 0
	/* Make the window for the tree */
	attributes.x = 0;
	attributes.y = 0;//ROTATOR_HEADER_HEIGHT (tree_view);
	attributes.width = MAX (tree_view->priv->width, widget->allocation.width);
	attributes.height = widget->allocation.height;
	attributes.event_mask = (GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | gtk_widget_get_events (widget));

	tree_view->priv->bin_window = gdk_window_new (widget->window, &attributes, attributes_mask);
	gdk_window_set_user_data (tree_view->priv->bin_window, widget);

	/* Make the column header window */
	attributes.x = 0;
	attributes.y = 0;
	attributes.width = MAX (tree_view->priv->width, widget->allocation.width);
	attributes.height = tree_view->priv->header_height;
	attributes.event_mask = (GDK_EXPOSURE_MASK |
                           GDK_SCROLL_MASK |
                           GDK_BUTTON_PRESS_MASK |
                           GDK_BUTTON_RELEASE_MASK |
                           GDK_KEY_PRESS_MASK |
                           GDK_KEY_RELEASE_MASK |
                           gtk_widget_get_events (widget));

	tree_view->priv->header_window = gdk_window_new (widget->window, &attributes, attributes_mask);
	gdk_window_set_user_data (tree_view->priv->header_window, widget);
#endif

	widget->style = gtk_style_attach (widget->style, widget->window);
	gdk_window_set_back_pixmap (widget->window, NULL, FALSE);
#if 0
	gdk_window_set_background (tree_view->priv->bin_window, &widget->style->base[widget->state]);
	gtk_style_set_background (widget->style, tree_view->priv->header_window, GTK_STATE_NORMAL);

	install_presize_handler (tree_view); 
#endif

#ifdef HAVE_GTK_2_18
	gtk_widget_set_can_focus(canvas, true);
#endif

	gboolean waveform_view_load_new_on_idle(gpointer _view)
	{
		GtkWidget* view = _view;
		Rotator* rotator = GTK_ROTATOR (view);
		g_return_val_if_fail(view, G_SOURCE_REMOVE);
		on_canvas_realise(view, NULL);

		_add_all(rotator);

		gtk_widget_queue_draw(view); //testing.
		return G_SOURCE_REMOVE;
	}
	g_idle_add(waveform_view_load_new_on_idle, widget);
}


static void
rotator_unrealize (GtkWidget *widget)
{
#if 0
  Rotator *tree_view = GTK_ROTATOR (widget);
  RotatorPrivate *priv = tree_view->priv;

  if (priv->scroll_timeout != 0)
    {
      g_source_remove (priv->scroll_timeout);
      priv->scroll_timeout = 0;
    }

  if (priv->open_dest_timeout != 0)
    {
      g_source_remove (priv->open_dest_timeout);
      priv->open_dest_timeout = 0;
    }

  remove_expand_collapse_timeout (tree_view);
  
  if (priv->presize_handler_timer != 0)
    {
      g_source_remove (priv->presize_handler_timer);
      priv->presize_handler_timer = 0;
    }

  if (priv->validate_rows_timer != 0)
    {
      g_source_remove (priv->validate_rows_timer);
      priv->validate_rows_timer = 0;
    }

  if (priv->scroll_sync_timer != 0)
    {
      g_source_remove (priv->scroll_sync_timer);
      priv->scroll_sync_timer = 0;
    }

  if (priv->typeselect_flush_timeout)
    {
      g_source_remove (priv->typeselect_flush_timeout);
      priv->typeselect_flush_timeout = 0;
    }
  
  GList *list;
  for (list = priv->columns; list; list = list->next)
    _rotator_column_unrealize_button (GTK_ROTATOR_COLUMN (list->data));
#endif

#if 0
  gdk_window_set_user_data (priv->bin_window, NULL);
  gdk_window_destroy (priv->bin_window);
  priv->bin_window = NULL;
#endif

#if 0
  gdk_window_set_user_data (priv->header_window, NULL);
  gdk_window_destroy (priv->header_window);
  priv->header_window = NULL;

  if (priv->drag_window)
    {
      gdk_window_set_user_data (priv->drag_window, NULL);
      gdk_window_destroy (priv->drag_window);
      priv->drag_window = NULL;
    }

  if (priv->drag_highlight_window)
    {
      gdk_window_set_user_data (priv->drag_highlight_window, NULL);
      gdk_window_destroy (priv->drag_highlight_window);
      priv->drag_highlight_window = NULL;
    }

  if (priv->grid_line_gc)
    {
      g_object_unref (priv->grid_line_gc);
      priv->grid_line_gc = NULL;
    }
#endif

  GTK_WIDGET_CLASS (rotator_parent_class)->unrealize (widget);
}

#if 0
/* GtkWidget::size_request helper */
static void
rotator_size_request_columns (Rotator *tree_view)
{
  GList *list;

  tree_view->priv->header_height = 0;

  if (tree_view->priv->model)
    {
      for (list = tree_view->priv->columns; list; list = list->next)
        {
          GtkRequisition requisition;
          RotatorColumn *column = list->data;

	  if (column->button == NULL)
	    continue;

          column = list->data;
	  
          gtk_widget_size_request (column->button, &requisition);
	  column->button_request = requisition.width;
          tree_view->priv->header_height = MAX (tree_view->priv->header_height, requisition.height);
        }
    }
}


/* Called only by ::size_request */
static void
rotator_update_size (Rotator *tree_view)
{
  GList *list;
  RotatorColumn *column;
  gint i;

  if (tree_view->priv->model == NULL)
    {
      tree_view->priv->width = 0;
      tree_view->priv->prev_width = 0;                   
      tree_view->priv->height = 0;
      return;
    }

  tree_view->priv->prev_width = tree_view->priv->width;  
  tree_view->priv->width = 0;

  /* keep this in sync with size_allocate below */
  for (list = tree_view->priv->columns, i = 0; list; list = list->next, i++)
    {
      gint real_requested_width = 0;
      column = list->data;
      if (!column->visible)
	continue;

      if (column->use_resized_width)
	{
	  real_requested_width = column->resized_width;
	}
      else if (column->column_type == GTK_ROTATOR_COLUMN_FIXED)
	{
	  real_requested_width = column->fixed_width;
	}
      else if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_HEADERS_VISIBLE))
	{
	  real_requested_width = MAX (column->requested_width, column->button_request);
	}
      else
	{
	  real_requested_width = column->requested_width;
	}

      if (column->min_width != -1)
	real_requested_width = MAX (real_requested_width, column->min_width);
      if (column->max_width != -1)
	real_requested_width = MIN (real_requested_width, column->max_width);

      tree_view->priv->width += real_requested_width;
    }

  if (tree_view->priv->tree == NULL)
    tree_view->priv->height = 0;
  else
    tree_view->priv->height = tree_view->priv->tree->root->offset;
}

static void
rotator_size_request (GtkWidget      *widget,
			    GtkRequisition *requisition)
{
  Rotator *tree_view = GTK_ROTATOR (widget);
  GList *tmp_list;

  /* we validate GTK_ROTATOR_NUM_ROWS_PER_IDLE rows initially just to make
   * sure we have some size. In practice, with a lot of static lists, this
   * should get a good width.
   */
  do_validate_rows (tree_view, FALSE);
  rotator_size_request_columns (tree_view);
  rotator_update_size (GTK_ROTATOR (widget));

  requisition->width = tree_view->priv->width;
  requisition->height = tree_view->priv->height + ROTATOR_HEADER_HEIGHT (tree_view);

  tmp_list = tree_view->priv->children;

  while (tmp_list)
    {
      RotatorChild *child = tmp_list->data;
      GtkRequisition child_requisition;

      tmp_list = tmp_list->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
        gtk_widget_size_request (child->widget, &child_requisition);
    }
}


static void
invalidate_column (Rotator       *tree_view,
                   RotatorColumn *column)
{
  gint column_offset = 0;
  GList *list;
  GtkWidget *widget = GTK_WIDGET (tree_view);
  gboolean rtl;

  if (!GTK_WIDGET_REALIZED (widget))
    return;

  rtl = (gtk_widget_get_direction (GTK_WIDGET (tree_view)) == GTK_TEXT_DIR_RTL);
  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
       list;
       list = (rtl ? list->prev : list->next))
    {
      RotatorColumn *tmpcolumn = list->data;
      if (tmpcolumn == column)
	{
	  GdkRectangle invalid_rect;
	  
	  invalid_rect.x = column_offset;
	  invalid_rect.y = 0;
	  invalid_rect.width = column->width;
	  invalid_rect.height = widget->allocation.height;
	  
	  gdk_window_invalidate_rect (widget->window, &invalid_rect, TRUE);
	  break;
	}
      
      column_offset += tmpcolumn->width;
    }
}

static void
invalidate_last_column (Rotator *tree_view)
{
  GList *last_column;
  gboolean rtl;

  rtl = (gtk_widget_get_direction (GTK_WIDGET (tree_view)) == GTK_TEXT_DIR_RTL);

  for (last_column = (rtl ? g_list_first (tree_view->priv->columns) : g_list_last (tree_view->priv->columns));
       last_column;
       last_column = (rtl ? last_column->next : last_column->prev))
    {
      if (GTK_ROTATOR_COLUMN (last_column->data)->visible)
        {
          invalidate_column (tree_view, last_column->data);
          return;
        }
    }
}

static gint
rotator_get_real_requested_width_from_column (Rotator       *tree_view,
                                                    RotatorColumn *column)
{
  gint real_requested_width;

  if (column->use_resized_width)
    {
      real_requested_width = column->resized_width;
    }
  else if (column->column_type == GTK_ROTATOR_COLUMN_FIXED)
    {
      real_requested_width = column->fixed_width;
    }
  else if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_HEADERS_VISIBLE))
    {
      real_requested_width = MAX (column->requested_width, column->button_request);
    }
  else
    {
      real_requested_width = column->requested_width;
      if (real_requested_width < 0)
        real_requested_width = 0;
    }

  if (column->min_width != -1)
    real_requested_width = MAX (real_requested_width, column->min_width);
  if (column->max_width != -1)
    real_requested_width = MIN (real_requested_width, column->max_width);

  return real_requested_width;
}

/* GtkWidget::size_allocate helper */
static void
rotator_size_allocate_columns (GtkWidget *widget,
				     gboolean  *width_changed)
{
  Rotator *tree_view;
  GList *list, *first_column, *last_column;
  RotatorColumn *column;
  GtkAllocation allocation;
  gint width = 0;
  gint extra, extra_per_column, extra_for_last;
  gint full_requested_width = 0;
  gint number_of_expand_columns = 0;
  gboolean column_changed = FALSE;
  gboolean rtl;
  gboolean update_expand;
  
  tree_view = GTK_ROTATOR (widget);

  for (last_column = g_list_last (tree_view->priv->columns);
       last_column && !(GTK_ROTATOR_COLUMN (last_column->data)->visible);
       last_column = last_column->prev)
    ;
  if (last_column == NULL)
    return;

  for (first_column = g_list_first (tree_view->priv->columns);
       first_column && !(GTK_ROTATOR_COLUMN (first_column->data)->visible);
       first_column = first_column->next)
    ;

  allocation.y = 0;
  allocation.height = tree_view->priv->header_height;

  rtl = (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL);

  /* find out how many extra space and expandable columns we have */
  for (list = tree_view->priv->columns; list != last_column->next; list = list->next)
    {
      column = (RotatorColumn *)list->data;

      if (!column->visible)
	continue;

      full_requested_width += rotator_get_real_requested_width_from_column (tree_view, column);

      if (column->expand)
	number_of_expand_columns++;
    }

  /* Only update the expand value if the width of the widget has changed,
   * or the number of expand columns has changed, or if there are no expand
   * columns, or if we didn't have an size-allocation yet after the
   * last validated node.
   */
  update_expand = (width_changed && *width_changed == TRUE)
      || number_of_expand_columns != tree_view->priv->last_number_of_expand_columns
      || number_of_expand_columns == 0
      || tree_view->priv->post_validation_flag == TRUE;

  tree_view->priv->post_validation_flag = FALSE;

  if (!update_expand)
    {
      extra = tree_view->priv->last_extra_space;
      extra_for_last = MAX (widget->allocation.width - full_requested_width - extra, 0);
    }
  else
    {
      extra = MAX (widget->allocation.width - full_requested_width, 0);
      extra_for_last = 0;

      tree_view->priv->last_extra_space = extra;
    }

  if (number_of_expand_columns > 0)
    extra_per_column = extra/number_of_expand_columns;
  else
    extra_per_column = 0;

  if (update_expand)
    {
      tree_view->priv->last_extra_space_per_column = extra_per_column;
      tree_view->priv->last_number_of_expand_columns = number_of_expand_columns;
    }

  for (list = (rtl ? last_column : first_column); 
       list != (rtl ? first_column->prev : last_column->next);
       list = (rtl ? list->prev : list->next)) 
    {
      gint real_requested_width = 0;
      gint old_width;

      column = list->data;
      old_width = column->width;

      if (!column->visible)
	continue;

      /* We need to handle the dragged button specially.
       */
      if (column == tree_view->priv->drag_column)
	{
	  GtkAllocation drag_allocation;
	  gdk_drawable_get_size (tree_view->priv->drag_window,
				 &(drag_allocation.width),
				 &(drag_allocation.height));
	  drag_allocation.x = 0;
	  drag_allocation.y = 0;
	  gtk_widget_size_allocate (tree_view->priv->drag_column->button,
				    &drag_allocation);
	  width += drag_allocation.width;
	  continue;
	}

      real_requested_width = rotator_get_real_requested_width_from_column (tree_view, column);

      allocation.x = width;
      column->width = real_requested_width;

      if (column->expand)
	{
	  if (number_of_expand_columns == 1)
	    {
	      /* We add the remander to the last column as
	       * */
	      column->width += extra;
	    }
	  else
	    {
	      column->width += extra_per_column;
	      extra -= extra_per_column;
	      number_of_expand_columns --;
	    }
	}
      else if (number_of_expand_columns == 0 &&
	       list == last_column)
	{
	  column->width += extra;
	}

      /* In addition to expand, the last column can get even more
       * extra space so all available space is filled up.
       */
      if (extra_for_last > 0 && list == last_column)
	column->width += extra_for_last;

      g_object_notify (G_OBJECT (column), "width");

      allocation.width = column->width;
      width += column->width;

      if (column->width > old_width)
        column_changed = TRUE;

      gtk_widget_size_allocate (column->button, &allocation);

      if (column->window)
	gdk_window_move_resize (column->window,
                                allocation.x + (rtl ? 0 : allocation.width) - ROTATOR_DRAG_WIDTH/2,
				allocation.y,
                                ROTATOR_DRAG_WIDTH, allocation.height);
    }

  /* We change the width here.  The user might have been resizing columns,
   * so the total width of the tree view changes.
   */
  tree_view->priv->width = width;
  if (width_changed)
    *width_changed = TRUE;

  if (column_changed)
    gtk_widget_queue_draw (GTK_WIDGET (tree_view));
}
#endif


static void
rotator_size_allocate (GtkWidget* widget, GtkAllocation* allocation)
{
	Rotator* tree_view = GTK_ROTATOR (widget);
	AGlActor* scene = (AGlActor*)tree_view->priv->scene;

	widget->allocation = *allocation;

	scene->region = (AGlfRegion){
		.x2 = allocation->width,
		.y2 = allocation->height
	};

	if (GTK_WIDGET_REALIZED (widget)){
		gdk_window_move_resize (widget->window, allocation->x, allocation->y, allocation->width, allocation->height);
	}

#if 0
  if (tree_view->priv->tree == NULL)
    invalidate_empty_focus (tree_view);
#endif

	if (GTK_WIDGET_REALIZED(widget)) gtk_widget_queue_draw (widget);

	start_zoom(tree_view, zoom);
}


#if 0
/* Grabs the focus and unsets the GTK_ROTATOR_DRAW_KEYFOCUS flag */
static void
grab_focus_and_unset_draw_keyfocus (Rotator *tree_view)
{
  if (GTK_WIDGET_CAN_FOCUS (tree_view) && !GTK_WIDGET_HAS_FOCUS (tree_view))
    gtk_widget_grab_focus (GTK_WIDGET (tree_view));
  GTK_ROTATOR_UNSET_FLAG (tree_view, GTK_ROTATOR_DRAW_KEYFOCUS);
}

static inline gboolean
row_is_separator (Rotator *tree_view,
		  GtkTreeIter *iter,
		  GtkTreePath *path)
{
  gboolean is_separator = FALSE;

  if (tree_view->priv->row_separator_func)
    {
      GtkTreeIter tmpiter;

      if (iter)
	tmpiter = *iter;
      else
	gtk_tree_model_get_iter (tree_view->priv->model, &tmpiter, path);

      is_separator = tree_view->priv->row_separator_func (tree_view->priv->model,
                                                          &tmpiter,
                                                          tree_view->priv->row_separator_data);
    }

  return is_separator;
}
#endif


static gboolean
rotator_button_press (GtkWidget* widget, GdkEventButton* event)
{
#if 0
  Rotator *tree_view = GTK_ROTATOR (widget);
  GList *list;
  RotatorColumn *column = NULL;
  gint i;
  GdkRectangle background_area;
  GdkRectangle cell_area;
  gint vertical_separator;
  gint horizontal_separator;
  gboolean path_is_selectable;
  gboolean rtl;

  rtl = (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL);
  rotator_stop_editing (tree_view, FALSE);
  gtk_widget_style_get (widget, "vertical-separator", &vertical_separator, "horizontal-separator", &horizontal_separator, NULL);

  // Because grab_focus can cause reentrancy, we delay grab_focus until after we're done handling the button press.

  if (event->window == tree_view->priv->bin_window)
    {
      GtkRBNode *node;
      GtkRBTree *tree;
      GtkTreePath *path;
      gchar *path_string;
      gint depth;
      gint new_y;
      gint y_offset;
      gint dval;
      gint pre_val, aft_val;
      RotatorColumn *column = NULL;
      GtkCellRenderer *focus_cell = NULL;
      gint column_handled_click = FALSE;
      gboolean row_double_click = FALSE;
      gboolean rtl;
      gboolean node_selected;

      /* Empty tree? */
      if (tree_view->priv->tree == NULL)
	{
	  grab_focus_and_unset_draw_keyfocus (tree_view);
	  return TRUE;
	}

      /* are we in an arrow? */
      if (tree_view->priv->prelight_node &&
          GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_ARROW_PRELIT) &&
	  ROTATOR_DRAW_EXPANDERS (tree_view))
	{
	  if (event->button == 1)
	    {
	      gtk_grab_add (widget);
	      tree_view->priv->button_pressed_node = tree_view->priv->prelight_node;
	      tree_view->priv->button_pressed_tree = tree_view->priv->prelight_tree;
	      rotator_draw_arrow (GTK_ROTATOR (widget),
					tree_view->priv->prelight_tree,
					tree_view->priv->prelight_node,
					event->x,
					event->y);
	    }

	  grab_focus_and_unset_draw_keyfocus (tree_view);
	  return TRUE;
	}

      /* find the node that was clicked */
      new_y = TREE_WINDOW_Y_TO_RBTREE_Y(tree_view, event->y);
      if (new_y < 0)
	new_y = 0;
      y_offset = -_gtk_rbtree_find_offset (tree_view->priv->tree, new_y, &tree, &node);

      if (node == NULL)
	{
	  /* We clicked in dead space */
	  grab_focus_and_unset_draw_keyfocus (tree_view);
	  return TRUE;
	}

      /* Get the path and the node */
      path = _rotator_find_path (tree_view, tree, node);
      path_is_selectable = !row_is_separator (tree_view, NULL, path);

      if (!path_is_selectable)
	{
	  gtk_tree_path_free (path);
	  grab_focus_and_unset_draw_keyfocus (tree_view);
	  return TRUE;
	}

      depth = gtk_tree_path_get_depth (path);
      background_area.y = y_offset + event->y;
      background_area.height = ROW_HEIGHT (tree_view, GTK_RBNODE_GET_HEIGHT (node));
      background_area.x = 0;


      /* Let the column have a chance at selecting it. */
      rtl = (gtk_widget_get_direction (GTK_WIDGET (tree_view)) == GTK_TEXT_DIR_RTL);
      for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
	   list; list = (rtl ? list->prev : list->next))
	{
	  RotatorColumn *candidate = list->data;

	  if (!candidate->visible)
	    continue;

	  background_area.width = candidate->width;
	  if ((background_area.x > (gint) event->x) ||
	      (background_area.x + background_area.width <= (gint) event->x))
	    {
	      background_area.x += background_area.width;
	      continue;
	    }

	  /* we found the focus column */
	  column = candidate;
	  cell_area = background_area;
	  cell_area.width -= horizontal_separator;
	  cell_area.height -= vertical_separator;
	  cell_area.x += horizontal_separator/2;
	  cell_area.y += vertical_separator/2;
	  if (rotator_is_expander_column (tree_view, column))
	    {
	      if (!rtl)
		cell_area.x += (depth - 1) * tree_view->priv->level_indentation;
	      cell_area.width -= (depth - 1) * tree_view->priv->level_indentation;

              if (ROTATOR_DRAW_EXPANDERS (tree_view))
	        {
		  if (!rtl)
		    cell_area.x += depth * tree_view->priv->expander_size;
	          cell_area.width -= depth * tree_view->priv->expander_size;
		}
	    }
	  break;
	}

      if (column == NULL)
	{
	  gtk_tree_path_free (path);
	  grab_focus_and_unset_draw_keyfocus (tree_view);
	  return FALSE;
	}

      tree_view->priv->focus_column = column;

      /* decide if we edit */
      if (event->type == GDK_BUTTON_PRESS && event->button == 1 &&
	  !(event->state & gtk_accelerator_get_default_mod_mask ()))
	{
	  GtkTreePath *anchor;
	  GtkTreeIter iter;

	  gtk_tree_model_get_iter (tree_view->priv->model, &iter, path);
	  rotator_column_cell_set_cell_data (column,
						   tree_view->priv->model,
						   &iter,
						   GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_PARENT),
						   node->children?TRUE:FALSE);

	  if (tree_view->priv->anchor)
	    anchor = gtk_tree_row_reference_get_path (tree_view->priv->anchor);
	  else
	    anchor = NULL;

	  if ((anchor && !gtk_tree_path_compare (anchor, path))
	      || !_rotator_column_has_editable_cell (column))
	    {
	      GtkCellEditable *cell_editable = NULL;

	      /* FIXME: get the right flags */
	      guint flags = 0;

	      path_string = gtk_tree_path_to_string (path);

	      if (_rotator_column_cell_event (column,
						    &cell_editable,
						    (GdkEvent *)event,
						    path_string,
						    &background_area,
						    &cell_area, flags))
		{
		  if (cell_editable != NULL)
		    {
		      gint left, right;
		      GdkRectangle area;

		      area = cell_area;
		      _rotator_column_get_neighbor_sizes (column,	_gtk_tree_view_column_get_edited_cell (column), &left, &right);

		      area.x += left;
		      area.width -= right + left;

		      rotator_real_start_editing (tree_view,
							column,
							path,
							cell_editable,
							&area,
							(GdkEvent *)event,
							flags);
		      g_free (path_string);
		      gtk_tree_path_free (path);
		      gtk_tree_path_free (anchor);
		      return TRUE;
		    }
		  column_handled_click = TRUE;
		}
	      g_free (path_string);
	    }
	  if (anchor)
	    gtk_tree_path_free (anchor);
	}

      /* select */
      node_selected = GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_SELECTED);
      pre_val = tree_view->priv->vadjustment->value;

      /* we only handle selection modifications on the first button press
       */
      if (event->type == GDK_BUTTON_PRESS)
        {
          if ((event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
            tree_view->priv->ctrl_pressed = TRUE;
          if ((event->state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK)
            tree_view->priv->shift_pressed = TRUE;

          focus_cell = _rotator_column_get_cell_at_pos (column, event->x - background_area.x);
          if (focus_cell)
            rotator_column_focus_cell (column, focus_cell);

          if (event->state & GDK_CONTROL_MASK)
            {
              rotator_real_set_cursor (tree_view, path, FALSE, TRUE);
              rotator_real_toggle_cursor_row (tree_view);
            }
          else if (event->state & GDK_SHIFT_MASK)
            {
              rotator_real_set_cursor (tree_view, path, FALSE, TRUE);
              rotator_real_select_cursor_row (tree_view, FALSE);
            }
          else
            {
              rotator_real_set_cursor (tree_view, path, TRUE, TRUE);
            }

          tree_view->priv->ctrl_pressed = FALSE;
          tree_view->priv->shift_pressed = FALSE;
        }

      /* the treeview may have been scrolled because of _set_cursor,
       * correct here
       */

      aft_val = tree_view->priv->vadjustment->value;
      dval = pre_val - aft_val;

      cell_area.y += dval;
      background_area.y += dval;

      /* Save press to possibly begin a drag
       */
      if (!column_handled_click &&
	  !tree_view->priv->in_grab &&
	  tree_view->priv->pressed_button < 0)
        {
          tree_view->priv->pressed_button = event->button;
          tree_view->priv->press_start_x = event->x;
          tree_view->priv->press_start_y = event->y;

	  if (tree_view->priv->rubber_banding_enable
	      && !node_selected
	      && tree_view->priv->selection->type == GTK_SELECTION_MULTIPLE)
	    {
	      tree_view->priv->press_start_y += tree_view->priv->dy;
	      tree_view->priv->rubber_band_x = event->x;
	      tree_view->priv->rubber_band_y = event->y + tree_view->priv->dy;
	      tree_view->priv->rubber_band_status = RUBBER_BAND_MAYBE_START;

	      if ((event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
		tree_view->priv->rubber_band_ctrl = TRUE;
	      if ((event->state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK)
		tree_view->priv->rubber_band_shift = TRUE;
	    }
        }

      /* Test if a double click happened on the same row. */
      if (event->button == 1)
        {
          /* We also handle triple clicks here, because a user could have done
           * a first click and a second double click on different rows.
           */
          if ((event->type == GDK_2BUTTON_PRESS
               || event->type == GDK_3BUTTON_PRESS)
              && tree_view->priv->last_button_press)
            {
              GtkTreePath *lsc;

              lsc = gtk_tree_row_reference_get_path (tree_view->priv->last_button_press);

              if (lsc)
                {
                  row_double_click = !gtk_tree_path_compare (lsc, path);
                  gtk_tree_path_free (lsc);
                }
            }

          if (row_double_click)
            {
              if (tree_view->priv->last_button_press)
                gtk_tree_row_reference_free (tree_view->priv->last_button_press);
              if (tree_view->priv->last_button_press_2)
                gtk_tree_row_reference_free (tree_view->priv->last_button_press_2);
              tree_view->priv->last_button_press = NULL;
              tree_view->priv->last_button_press_2 = NULL;
            }
          else
            {
              if (tree_view->priv->last_button_press)
                gtk_tree_row_reference_free (tree_view->priv->last_button_press);
              tree_view->priv->last_button_press = tree_view->priv->last_button_press_2;
              tree_view->priv->last_button_press_2 = gtk_tree_row_reference_new_proxy (G_OBJECT (tree_view), tree_view->priv->model, path);
            }
        }

      if (row_double_click)
	{
	  gtk_grab_remove (widget);
	  rotator_row_activated (tree_view, path, column);

          if (tree_view->priv->pressed_button == event->button)
            tree_view->priv->pressed_button = -1;
	}

      gtk_tree_path_free (path);

      /* If we activated the row through a double click we don't want to grab
       * focus back, as moving focus to another widget is pretty common.
       */
      if (!row_double_click)
	grab_focus_and_unset_draw_keyfocus (tree_view);

      return TRUE;
    }

  /* We didn't click in the window.  Let's check to see if we clicked on a column resize window.
   */
  for (i = 0, list = tree_view->priv->columns; list; list = list->next, i++)
    {
      column = list->data;
      if (event->window == column->window &&
	  column->resizable &&
	  column->window)
	{
	  gpointer drag_data;

	  if (event->type == GDK_2BUTTON_PRESS &&
	      rotator_column_get_sizing (column) != GTK_ROTATOR_COLUMN_AUTOSIZE)
	    {
	      column->use_resized_width = FALSE;
	      _rotator_column_autosize (tree_view, column);
	      return TRUE;
	    }

	  if (gdk_pointer_grab (column->window, FALSE,
				GDK_POINTER_MOTION_HINT_MASK |
				GDK_BUTTON1_MOTION_MASK |
				GDK_BUTTON_RELEASE_MASK,
				NULL, NULL, event->time))
	    return FALSE;

	  gtk_grab_add (widget);
	  GTK_ROTATOR_SET_FLAG (tree_view, GTK_ROTATOR_IN_COLUMN_RESIZE);
	  column->resized_width = column->width - tree_view->priv->last_extra_space_per_column;

	  /* block attached dnd signal handler */
	  drag_data = g_object_get_data (G_OBJECT (widget), "gtk-site-data");
	  if (drag_data)
	    g_signal_handlers_block_matched (widget,
					     G_SIGNAL_MATCH_DATA,
					     0, 0, NULL, NULL,
					     drag_data);

	  tree_view->priv->drag_pos = i;
	  tree_view->priv->x_drag = column->button->allocation.x + (rtl ? 0 : column->button->allocation.width);

	  if (!GTK_WIDGET_HAS_FOCUS (widget))
	    gtk_widget_grab_focus (widget);

	  return TRUE;
	}
    }
#endif
  return FALSE;
}


#if 0
/* GtkWidget::button_release_event helper */
static gboolean
rotator_button_release_drag_column (GtkWidget      *widget,
					  GdkEventButton *event)
{
  Rotator *tree_view;
  GList *l;
  gboolean rtl;

  tree_view = GTK_ROTATOR (widget);

  rtl = (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL);
  gdk_display_pointer_ungrab (gtk_widget_get_display (widget), GDK_CURRENT_TIME);
  gdk_display_keyboard_ungrab (gtk_widget_get_display (widget), GDK_CURRENT_TIME);

  /* Move the button back */
  g_object_ref (tree_view->priv->drag_column->button);
  gtk_container_remove (GTK_CONTAINER (tree_view), tree_view->priv->drag_column->button);
  gtk_widget_set_parent_window (tree_view->priv->drag_column->button, tree_view->priv->header_window);
  gtk_widget_set_parent (tree_view->priv->drag_column->button, GTK_WIDGET (tree_view));
  g_object_unref (tree_view->priv->drag_column->button);
  gtk_widget_queue_resize (widget);
  if (tree_view->priv->drag_column->resizable)
    {
      gdk_window_raise (tree_view->priv->drag_column->window);
      gdk_window_show (tree_view->priv->drag_column->window);
    }
  else
    gdk_window_hide (tree_view->priv->drag_column->window);

  gtk_widget_grab_focus (tree_view->priv->drag_column->button);

  if (rtl)
    {
      if (tree_view->priv->cur_reorder &&
	  tree_view->priv->cur_reorder->right_column != tree_view->priv->drag_column)
	rotator_move_column_after (tree_view, tree_view->priv->drag_column,
					 tree_view->priv->cur_reorder->right_column);
    }
  else
    {
      if (tree_view->priv->cur_reorder &&
	  tree_view->priv->cur_reorder->left_column != tree_view->priv->drag_column)
	rotator_move_column_after (tree_view, tree_view->priv->drag_column,
					 tree_view->priv->cur_reorder->left_column);
    }
  tree_view->priv->drag_column = NULL;
  gdk_window_hide (tree_view->priv->drag_window);

  for (l = tree_view->priv->column_drag_info; l != NULL; l = l->next)
    g_slice_free (RotatorColumnReorder, l->data);
  g_list_free (tree_view->priv->column_drag_info);
  tree_view->priv->column_drag_info = NULL;
  tree_view->priv->cur_reorder = NULL;

  if (tree_view->priv->drag_highlight_window)
    gdk_window_hide (tree_view->priv->drag_highlight_window);

  /* Reset our flags */
  tree_view->priv->drag_column_window_state = DRAG_COLUMN_WINDOW_STATE_UNSET;
  GTK_ROTATOR_UNSET_FLAG (tree_view, GTK_ROTATOR_IN_COLUMN_DRAG);

  return TRUE;
}

/* GtkWidget::button_release_event helper */
static gboolean
rotator_button_release_column_resize (GtkWidget      *widget,
					    GdkEventButton *event)
{
  Rotator *tree_view;
  gpointer drag_data;

  tree_view = GTK_ROTATOR (widget);

  tree_view->priv->drag_pos = -1;

  /* unblock attached dnd signal handler */
  drag_data = g_object_get_data (G_OBJECT (widget), "gtk-site-data");
  if (drag_data)
    g_signal_handlers_unblock_matched (widget,
				       G_SIGNAL_MATCH_DATA,
				       0, 0, NULL, NULL,
				       drag_data);

  GTK_ROTATOR_UNSET_FLAG (tree_view, GTK_ROTATOR_IN_COLUMN_RESIZE);
  gtk_grab_remove (widget);
  gdk_display_pointer_ungrab (gdk_drawable_get_display (event->window),
			      event->time);
  return TRUE;
}

static gboolean
rotator_button_release (GtkWidget      *widget,
			      GdkEventButton *event)
{
  Rotator *tree_view = GTK_ROTATOR (widget);

  if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_IN_COLUMN_DRAG))
    return rotator_button_release_drag_column (widget, event);

  if (tree_view->priv->rubber_band_status)
    rotator_stop_rubber_band (tree_view);

  if (tree_view->priv->pressed_button == event->button)
    tree_view->priv->pressed_button = -1;

  if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_IN_COLUMN_RESIZE))
    return rotator_button_release_column_resize (widget, event);

  if (tree_view->priv->button_pressed_node == NULL)
    return FALSE;

  if (event->button == 1)
    {
      gtk_grab_remove (widget);
      if (tree_view->priv->button_pressed_node == tree_view->priv->prelight_node &&
          GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_ARROW_PRELIT))
	{
	  GtkTreePath *path = NULL;

	  path = _rotator_find_path (tree_view,
					   tree_view->priv->button_pressed_tree,
					   tree_view->priv->button_pressed_node);
	  /* Actually activate the node */
	  if (tree_view->priv->button_pressed_node->children == NULL)
	    rotator_real_expand_row (tree_view, path,
					   tree_view->priv->button_pressed_tree,
					   tree_view->priv->button_pressed_node,
					   FALSE, TRUE);
	  else
	    rotator_real_collapse_row (GTK_ROTATOR (widget), path,
					     tree_view->priv->button_pressed_tree,
					     tree_view->priv->button_pressed_node, TRUE);
	  gtk_tree_path_free (path);
	}

      tree_view->priv->button_pressed_tree = NULL;
      tree_view->priv->button_pressed_node = NULL;
    }

  return TRUE;
}

static gboolean
rotator_grab_broken (GtkWidget          *widget,
			   GdkEventGrabBroken *event)
{
  Rotator *tree_view = GTK_ROTATOR (widget);

  if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_IN_COLUMN_DRAG))
    rotator_button_release_drag_column (widget, (GdkEventButton *)event);

  if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_IN_COLUMN_RESIZE))
    rotator_button_release_column_resize (widget, (GdkEventButton *)event);

  return TRUE;
}

#if 0
static gboolean
rotator_configure (GtkWidget *widget,
			 GdkEventConfigure *event)
{
  Rotator *tree_view;

  tree_view = GTK_ROTATOR (widget);
  tree_view->priv->search_position_func (tree_view, tree_view->priv->search_window);

  return FALSE;
}
#endif

/* GtkWidget::motion_event function set.
 */

static gboolean
coords_are_over_arrow (Rotator *tree_view,
                       GtkRBTree   *tree,
                       GtkRBNode   *node,
                       /* these are in bin window coords */
                       gint         x,
                       gint         y)
{
  GdkRectangle arrow;
  gint x2;

  if (!GTK_WIDGET_REALIZED (tree_view))
    return FALSE;

  if ((node->flags & GTK_RBNODE_IS_PARENT) == 0)
    return FALSE;

  arrow.y = BACKGROUND_FIRST_PIXEL (tree_view, tree, node);

  rotator_get_arrow_xrange (tree_view, tree, &arrow.x, &x2);

  arrow.width = x2 - arrow.x;

  return (x >= arrow.x &&
          x < (arrow.x + arrow.width) &&
	  y >= arrow.y &&
	  y < (arrow.y + arrow.height));
}

static void
do_prelight (Rotator *tree_view,
             GtkRBTree   *tree,
             GtkRBNode   *node,
	     /* these are in bin_window coords */
             gint         x,
             gint         y)
{
  if (tree_view->priv->prelight_tree == tree &&
      tree_view->priv->prelight_node == node)
    {
      /*  We are still on the same node,
	  but we might need to take care of the arrow  */

      if (tree && node && ROTATOR_DRAW_EXPANDERS (tree_view))
	{
	  gboolean over_arrow;
	  gboolean flag_set;

	  over_arrow = coords_are_over_arrow (tree_view, tree, node, x, y);
	  flag_set = GTK_ROTATOR_FLAG_SET (tree_view,
					     GTK_ROTATOR_ARROW_PRELIT);

	  if (over_arrow != flag_set)
	    {
	      if (over_arrow)
		GTK_ROTATOR_SET_FLAG (tree_view,
					GTK_ROTATOR_ARROW_PRELIT);
	      else
		GTK_ROTATOR_UNSET_FLAG (tree_view,
					  GTK_ROTATOR_ARROW_PRELIT);

	      rotator_draw_arrow (tree_view, tree, node, x, y);
	    }
	}

      return;
    }

  if (tree_view->priv->prelight_tree && tree_view->priv->prelight_node)
    {
      /*  Unprelight the old node and arrow  */

      GTK_RBNODE_UNSET_FLAG (tree_view->priv->prelight_node,
			     GTK_RBNODE_IS_PRELIT);

      if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_ARROW_PRELIT)
	  && ROTATOR_DRAW_EXPANDERS (tree_view))
	{
	  GTK_ROTATOR_UNSET_FLAG (tree_view, GTK_ROTATOR_ARROW_PRELIT);
	  
	  rotator_draw_arrow (tree_view,
				    tree_view->priv->prelight_tree,
				    tree_view->priv->prelight_node,
				    x,
				    y);
	}

      _rotator_queue_draw_node (tree_view,
				      tree_view->priv->prelight_tree,
				      tree_view->priv->prelight_node,
				      NULL);
    }


  if (tree_view->priv->hover_expand)
    remove_auto_expand_timeout (tree_view);

  /*  Set the new prelight values  */
  tree_view->priv->prelight_node = node;
  tree_view->priv->prelight_tree = tree;

  if (!node || !tree)
    return;

  /*  Prelight the new node and arrow  */

  if (ROTATOR_DRAW_EXPANDERS (tree_view)
      && coords_are_over_arrow (tree_view, tree, node, x, y))
    {
      GTK_ROTATOR_SET_FLAG (tree_view, GTK_ROTATOR_ARROW_PRELIT);

      rotator_draw_arrow (tree_view, tree, node, x, y);
    }

  GTK_RBNODE_SET_FLAG (node, GTK_RBNODE_IS_PRELIT);

  _rotator_queue_draw_node (tree_view, tree, node, NULL);

}

static void
prelight_or_select (Rotator *tree_view,
		    GtkRBTree   *tree,
		    GtkRBNode   *node,
		    /* these are in bin_window coords */
		    gint         x,
		    gint         y)
{
  GtkSelectionMode mode = gtk_tree_selection_get_mode (tree_view->priv->selection);
  
  if (tree_view->priv->hover_selection &&
      (mode == GTK_SELECTION_SINGLE || mode == GTK_SELECTION_BROWSE) &&
      !(tree_view->priv->edited_column &&
	tree_view->priv->edited_column->editable_widget))
    {
      if (node)
	{
	  if (!GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_SELECTED))
	    {
	      GtkTreePath *path;
	      
	      path = _rotator_find_path (tree_view, tree, node);
	      gtk_tree_selection_select_path (tree_view->priv->selection, path);
	      if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_SELECTED))
		{
		  GTK_ROTATOR_UNSET_FLAG (tree_view, GTK_ROTATOR_DRAW_KEYFOCUS);
		  rotator_real_set_cursor (tree_view, path, FALSE, FALSE);
		}
	      gtk_tree_path_free (path);
	    }
	}

      else if (mode == GTK_SELECTION_SINGLE)
	gtk_tree_selection_unselect_all (tree_view->priv->selection);
    }

    do_prelight (tree_view, tree, node, x, y);
}

static void
ensure_unprelighted (Rotator *tree_view)
{
  do_prelight (tree_view,
	       NULL, NULL,
	       -1000, -1000); /* coords not possibly over an arrow */

  g_assert (tree_view->priv->prelight_node == NULL);
}




/* Our motion arrow is either a box (in the case of the original spot)
 * or an arrow.  It is expander_size wide.
 */
/*
 * 11111111111111
 * 01111111111110
 * 00111111111100
 * 00011111111000
 * 00001111110000
 * 00000111100000
 * 00000111100000
 * 00000111100000
 * ~ ~ ~ ~ ~ ~ ~
 * 00000111100000
 * 00000111100000
 * 00000111100000
 * 00001111110000
 * 00011111111000
 * 00111111111100
 * 01111111111110
 * 11111111111111
 */

static void
rotator_motion_draw_column_motion_arrow (Rotator *tree_view)
{
  RotatorColumnReorder *reorder = tree_view->priv->cur_reorder;
  GtkWidget *widget = GTK_WIDGET (tree_view);
  GdkBitmap *mask = NULL;
  gint x;
  gint y;
  gint width;
  gint height;
  gint arrow_type = DRAG_COLUMN_WINDOW_STATE_UNSET;
  GdkWindowAttr attributes;
  guint attributes_mask;

  if (!reorder ||
      reorder->left_column == tree_view->priv->drag_column ||
      reorder->right_column == tree_view->priv->drag_column)
    arrow_type = DRAG_COLUMN_WINDOW_STATE_ORIGINAL;
  else if (reorder->left_column || reorder->right_column)
    {
      GdkRectangle visible_rect;
      rotator_get_visible_rect (tree_view, &visible_rect);
      if (reorder->left_column)
	x = reorder->left_column->button->allocation.x + reorder->left_column->button->allocation.width;
      else
	x = reorder->right_column->button->allocation.x;

      if (x < visible_rect.x)
	arrow_type = DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT;
      else if (x > visible_rect.x + visible_rect.width)
	arrow_type = DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT;
      else
        arrow_type = DRAG_COLUMN_WINDOW_STATE_ARROW;
    }

  /* We want to draw the rectangle over the initial location. */
  if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ORIGINAL)
    {
      GdkGC *gc;
      GdkColor col;

      if (tree_view->priv->drag_column_window_state != DRAG_COLUMN_WINDOW_STATE_ORIGINAL)
	{
	  if (tree_view->priv->drag_highlight_window)
	    {
	      gdk_window_set_user_data (tree_view->priv->drag_highlight_window,
					NULL);
	      gdk_window_destroy (tree_view->priv->drag_highlight_window);
	    }

	  attributes.window_type = GDK_WINDOW_CHILD;
	  attributes.wclass = GDK_INPUT_OUTPUT;
          attributes.x = tree_view->priv->drag_column_x;
          attributes.y = 0;
	  width = attributes.width = tree_view->priv->drag_column->button->allocation.width;
	  height = attributes.height = tree_view->priv->drag_column->button->allocation.height;
	  attributes.visual = gtk_widget_get_visual (GTK_WIDGET (tree_view));
	  attributes.colormap = gtk_widget_get_colormap (GTK_WIDGET (tree_view));
	  attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK | GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK;
	  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	  tree_view->priv->drag_highlight_window = gdk_window_new (tree_view->priv->header_window, &attributes, attributes_mask);
	  gdk_window_set_user_data (tree_view->priv->drag_highlight_window, GTK_WIDGET (tree_view));

	  mask = gdk_pixmap_new (tree_view->priv->drag_highlight_window, width, height, 1);
	  gc = gdk_gc_new (mask);
	  col.pixel = 1;
	  gdk_gc_set_foreground (gc, &col);
	  gdk_draw_rectangle (mask, gc, TRUE, 0, 0, width, height);
	  col.pixel = 0;
	  gdk_gc_set_foreground(gc, &col);
	  gdk_draw_rectangle (mask, gc, TRUE, 2, 2, width - 4, height - 4);
	  g_object_unref (gc);

	  gdk_window_shape_combine_mask (tree_view->priv->drag_highlight_window,
					 mask, 0, 0);
	  if (mask) g_object_unref (mask);
	  tree_view->priv->drag_column_window_state = DRAG_COLUMN_WINDOW_STATE_ORIGINAL;
	}
    }
  else if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW)
    {
      gint i, j = 1;
      GdkGC *gc;
      GdkColor col;

      width = tree_view->priv->expander_size;

      /* Get x, y, width, height of arrow */
      gdk_window_get_origin (tree_view->priv->header_window, &x, &y);
      if (reorder->left_column)
	{
	  x += reorder->left_column->button->allocation.x + reorder->left_column->button->allocation.width - width/2;
	  height = reorder->left_column->button->allocation.height;
	}
      else
	{
	  x += reorder->right_column->button->allocation.x - width/2;
	  height = reorder->right_column->button->allocation.height;
	}
      y -= tree_view->priv->expander_size/2; /* The arrow takes up only half the space */
      height += tree_view->priv->expander_size;

      /* Create the new window */
      if (tree_view->priv->drag_column_window_state != DRAG_COLUMN_WINDOW_STATE_ARROW)
	{
	  if (tree_view->priv->drag_highlight_window)
	    {
	      gdk_window_set_user_data (tree_view->priv->drag_highlight_window,
					NULL);
	      gdk_window_destroy (tree_view->priv->drag_highlight_window);
	    }

	  attributes.window_type = GDK_WINDOW_TEMP;
	  attributes.wclass = GDK_INPUT_OUTPUT;
	  attributes.visual = gtk_widget_get_visual (GTK_WIDGET (tree_view));
	  attributes.colormap = gtk_widget_get_colormap (GTK_WIDGET (tree_view));
	  attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK | GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK;
	  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
          attributes.x = x;
          attributes.y = y;
	  attributes.width = width;
	  attributes.height = height;
	  tree_view->priv->drag_highlight_window = gdk_window_new (gtk_widget_get_root_window (widget),
								   &attributes, attributes_mask);
	  gdk_window_set_user_data (tree_view->priv->drag_highlight_window, GTK_WIDGET (tree_view));

	  mask = gdk_pixmap_new (tree_view->priv->drag_highlight_window, width, height, 1);
	  gc = gdk_gc_new (mask);
	  col.pixel = 1;
	  gdk_gc_set_foreground (gc, &col);
	  gdk_draw_rectangle (mask, gc, TRUE, 0, 0, width, height);

	  /* Draw the 2 arrows as per above */
	  col.pixel = 0;
	  gdk_gc_set_foreground (gc, &col);
	  for (i = 0; i < width; i ++)
	    {
	      if (i == (width/2 - 1))
		continue;
	      gdk_draw_line (mask, gc, i, j, i, height - j);
	      if (i < (width/2 - 1))
		j++;
	      else
		j--;
	    }
	  g_object_unref (gc);
	  gdk_window_shape_combine_mask (tree_view->priv->drag_highlight_window,
					 mask, 0, 0);
	  if (mask) g_object_unref (mask);
	}

      tree_view->priv->drag_column_window_state = DRAG_COLUMN_WINDOW_STATE_ARROW;
      gdk_window_move (tree_view->priv->drag_highlight_window, x, y);
    }
  else if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT ||
	   arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT)
    {
      gint i, j = 1;
      GdkGC *gc;
      GdkColor col;

      width = tree_view->priv->expander_size;

      /* Get x, y, width, height of arrow */
      width = width/2; /* remember, the arrow only takes half the available width */
      gdk_window_get_origin (widget->window, &x, &y);
      if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT)
	x += widget->allocation.width - width;

      if (reorder->left_column)
	height = reorder->left_column->button->allocation.height;
      else
	height = reorder->right_column->button->allocation.height;

      y -= tree_view->priv->expander_size;
      height += 2*tree_view->priv->expander_size;

      /* Create the new window */
      if (tree_view->priv->drag_column_window_state != DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT &&
	  tree_view->priv->drag_column_window_state != DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT)
	{
	  if (tree_view->priv->drag_highlight_window)
	    {
	      gdk_window_set_user_data (tree_view->priv->drag_highlight_window,
					NULL);
	      gdk_window_destroy (tree_view->priv->drag_highlight_window);
	    }

	  attributes.window_type = GDK_WINDOW_TEMP;
	  attributes.wclass = GDK_INPUT_OUTPUT;
	  attributes.visual = gtk_widget_get_visual (GTK_WIDGET (tree_view));
	  attributes.colormap = gtk_widget_get_colormap (GTK_WIDGET (tree_view));
	  attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK | GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK;
	  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
          attributes.x = x;
          attributes.y = y;
	  attributes.width = width;
	  attributes.height = height;
	  tree_view->priv->drag_highlight_window = gdk_window_new (NULL, &attributes, attributes_mask);
	  gdk_window_set_user_data (tree_view->priv->drag_highlight_window, GTK_WIDGET (tree_view));

	  mask = gdk_pixmap_new (tree_view->priv->drag_highlight_window, width, height, 1);
	  gc = gdk_gc_new (mask);
	  col.pixel = 1;
	  gdk_gc_set_foreground (gc, &col);
	  gdk_draw_rectangle (mask, gc, TRUE, 0, 0, width, height);

	  /* Draw the 2 arrows as per above */
	  col.pixel = 0;
	  gdk_gc_set_foreground (gc, &col);
	  j = tree_view->priv->expander_size;
	  for (i = 0; i < width; i ++)
	    {
	      gint k;
	      if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT)
		k = width - i - 1;
	      else
		k = i;
	      gdk_draw_line (mask, gc, k, j, k, height - j);
	      gdk_draw_line (mask, gc, k, 0, k, tree_view->priv->expander_size - j);
	      gdk_draw_line (mask, gc, k, height, k, height - tree_view->priv->expander_size + j);
	      j--;
	    }
	  g_object_unref (gc);
	  gdk_window_shape_combine_mask (tree_view->priv->drag_highlight_window,
					 mask, 0, 0);
	  if (mask) g_object_unref (mask);
	}

      tree_view->priv->drag_column_window_state = arrow_type;
      gdk_window_move (tree_view->priv->drag_highlight_window, x, y);
   }
  else
    {
      g_warning (G_STRLOC"Invalid RotatorColumnReorder struct");
      gdk_window_hide (tree_view->priv->drag_highlight_window);
      return;
    }

  gdk_window_show (tree_view->priv->drag_highlight_window);
  gdk_window_raise (tree_view->priv->drag_highlight_window);
}

static gboolean
rotator_motion_resize_column (GtkWidget      *widget, GdkEventMotion *event)
{
  gint x;
  gint new_width;
  RotatorColumn *column;
  Rotator *tree_view = GTK_ROTATOR (widget);

  column = rotator_get_column (tree_view, tree_view->priv->drag_pos);

  if (event->is_hint || event->window != widget->window)
    gtk_widget_get_pointer (widget, &x, NULL);
  else
    x = event->x;

  if (tree_view->priv->hadjustment)
    x += tree_view->priv->hadjustment->value;

  new_width = rotator_new_column_width (tree_view, tree_view->priv->drag_pos, &x);
  if (x != tree_view->priv->x_drag &&
      (new_width != column->fixed_width))
    {
      column->use_resized_width = TRUE;
      column->resized_width = new_width;
      if (column->expand)
	column->resized_width -= tree_view->priv->last_extra_space_per_column;
      gtk_widget_queue_resize (widget);
    }

  return FALSE;
}


static void
rotator_update_current_reorder (Rotator *tree_view)
{
  RotatorColumnReorder *reorder = NULL;
  GList *list;
  gint mouse_x;

  gdk_window_get_pointer (tree_view->priv->header_window, &mouse_x, NULL, NULL);
  for (list = tree_view->priv->column_drag_info; list; list = list->next)
    {
      reorder = (RotatorColumnReorder *) list->data;
      if (mouse_x >= reorder->left_align && mouse_x < reorder->right_align)
	break;
      reorder = NULL;
    }

  /*  if (reorder && reorder == tree_view->priv->cur_reorder)
      return;*/

  tree_view->priv->cur_reorder = reorder;
  rotator_motion_draw_column_motion_arrow (tree_view);
}

static void
rotator_vertical_autoscroll (Rotator *tree_view)
{
  GdkRectangle visible_rect;
  gint y;
  gint offset;
  gfloat value;

  gdk_window_get_pointer (tree_view->priv->bin_window, NULL, &y, NULL);
  y += tree_view->priv->dy;

  rotator_get_visible_rect (tree_view, &visible_rect);

  /* see if we are near the edge. */
  offset = y - (visible_rect.y + 2 * SCROLL_EDGE_SIZE);
  if (offset > 0)
    {
      offset = y - (visible_rect.y + visible_rect.height - 2 * SCROLL_EDGE_SIZE);
      if (offset < 0)
	return;
    }

  value = CLAMP (tree_view->priv->vadjustment->value + offset, 0.0,
		 tree_view->priv->vadjustment->upper - tree_view->priv->vadjustment->page_size);
  gtk_adjustment_set_value (tree_view->priv->vadjustment, value);
}

static gboolean
rotator_horizontal_autoscroll (Rotator *tree_view)
{
  GdkRectangle visible_rect;
  gint x;
  gint offset;
  gfloat value;

  gdk_window_get_pointer (tree_view->priv->bin_window, &x, NULL, NULL);

  rotator_get_visible_rect (tree_view, &visible_rect);

  /* See if we are near the edge. */
  offset = x - (visible_rect.x + SCROLL_EDGE_SIZE);
  if (offset > 0)
    {
      offset = x - (visible_rect.x + visible_rect.width - SCROLL_EDGE_SIZE);
      if (offset < 0)
	return TRUE;
    }
  offset = offset/3;

  value = CLAMP (tree_view->priv->hadjustment->value + offset,
		 0.0, tree_view->priv->hadjustment->upper - tree_view->priv->hadjustment->page_size);
  gtk_adjustment_set_value (tree_view->priv->hadjustment, value);

  return TRUE;

}

static gboolean
rotator_motion_drag_column (GtkWidget      *widget,
				  GdkEventMotion *event)
{
  Rotator *tree_view = (Rotator *) widget;
  RotatorColumn *column = tree_view->priv->drag_column;
  gint x, y;

  /* Sanity Check */
  if ((column == NULL) ||
      (event->window != tree_view->priv->drag_window))
    return FALSE;

  /* Handle moving the header */
  gdk_window_get_position (tree_view->priv->drag_window, &x, &y);
  x = CLAMP (x + (gint)event->x - column->drag_x, 0,
	     MAX (tree_view->priv->width, GTK_WIDGET (tree_view)->allocation.width) - column->button->allocation.width);
  gdk_window_move (tree_view->priv->drag_window, x, y);
  
  /* autoscroll, if needed */
  rotator_horizontal_autoscroll (tree_view);
  /* Update the current reorder position and arrow; */
  rotator_update_current_reorder (tree_view);

  return TRUE;
}

static void
rotator_stop_rubber_band (Rotator *tree_view)
{
  remove_scroll_timeout (tree_view);
  gtk_grab_remove (GTK_WIDGET (tree_view));

  if (tree_view->priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    {
      GtkTreePath *tmp_path;

      gtk_widget_queue_draw (GTK_WIDGET (tree_view));

      /* The anchor path should be set to the start path */
      tmp_path = _rotator_find_path (tree_view,
					   tree_view->priv->rubber_band_start_tree,
					   tree_view->priv->rubber_band_start_node);

      if (tree_view->priv->anchor)
	gtk_tree_row_reference_free (tree_view->priv->anchor);

      tree_view->priv->anchor =
	gtk_tree_row_reference_new_proxy (G_OBJECT (tree_view),
					  tree_view->priv->model,
					  tmp_path);

      gtk_tree_path_free (tmp_path);

      /* ... and the cursor to the end path */
      tmp_path = _rotator_find_path (tree_view,
					   tree_view->priv->rubber_band_end_tree,
					   tree_view->priv->rubber_band_end_node);
      rotator_real_set_cursor (GTK_ROTATOR (tree_view), tmp_path, FALSE, FALSE);
      gtk_tree_path_free (tmp_path);

      _gtk_tree_selection_emit_changed (tree_view->priv->selection);
    }

  /* Clear status variables */
  tree_view->priv->rubber_band_status = RUBBER_BAND_OFF;
  tree_view->priv->rubber_band_shift = 0;
  tree_view->priv->rubber_band_ctrl = 0;

  tree_view->priv->rubber_band_start_node = NULL;
  tree_view->priv->rubber_band_start_tree = NULL;
  tree_view->priv->rubber_band_end_node = NULL;
  tree_view->priv->rubber_band_end_tree = NULL;
}

static void
rotator_update_rubber_band_selection_range (Rotator *tree_view,
						 GtkRBTree   *start_tree,
						 GtkRBNode   *start_node,
						 GtkRBTree   *end_tree,
						 GtkRBNode   *end_node,
						 gboolean     select,
						 gboolean     skip_start,
						 gboolean     skip_end)
{
  if (start_node == end_node)
    return;

  /* We skip the first node and jump inside the loop */
  if (skip_start)
    goto skip_first;

  do
    {
      /* Small optimization by assuming insensitive nodes are never
       * selected.
       */
      if (!GTK_RBNODE_FLAG_SET (start_node, GTK_RBNODE_IS_SELECTED))
        {
	  GtkTreePath *path;
	  gboolean selectable;

	  path = _rotator_find_path (tree_view, start_tree, start_node);
	  selectable = _gtk_tree_selection_row_is_selectable (tree_view->priv->selection, start_node, path);
	  gtk_tree_path_free (path);

	  if (!selectable)
	    goto node_not_selectable;
	}

      if (select)
        {
	  if (tree_view->priv->rubber_band_shift)
	    GTK_RBNODE_SET_FLAG (start_node, GTK_RBNODE_IS_SELECTED);
	  else if (tree_view->priv->rubber_band_ctrl)
	    {
	      /* Toggle the selection state */
	      if (GTK_RBNODE_FLAG_SET (start_node, GTK_RBNODE_IS_SELECTED))
		GTK_RBNODE_UNSET_FLAG (start_node, GTK_RBNODE_IS_SELECTED);
	      else
		GTK_RBNODE_SET_FLAG (start_node, GTK_RBNODE_IS_SELECTED);
	    }
	  else
	    GTK_RBNODE_SET_FLAG (start_node, GTK_RBNODE_IS_SELECTED);
	}
      else
        {
	  /* Mirror the above */
	  if (tree_view->priv->rubber_band_shift)
	    GTK_RBNODE_UNSET_FLAG (start_node, GTK_RBNODE_IS_SELECTED);
	  else if (tree_view->priv->rubber_band_ctrl)
	    {
	      /* Toggle the selection state */
	      if (GTK_RBNODE_FLAG_SET (start_node, GTK_RBNODE_IS_SELECTED))
		GTK_RBNODE_UNSET_FLAG (start_node, GTK_RBNODE_IS_SELECTED);
	      else
		GTK_RBNODE_SET_FLAG (start_node, GTK_RBNODE_IS_SELECTED);
	    }
	  else
	    GTK_RBNODE_UNSET_FLAG (start_node, GTK_RBNODE_IS_SELECTED);
	}

      _rotator_queue_draw_node (tree_view, start_tree, start_node, NULL);

node_not_selectable:
      if (start_node == end_node)
	break;

skip_first:

      if (start_node->children)
        {
	  start_tree = start_node->children;
	  start_node = start_tree->root;
	  while (start_node->left != start_tree->nil)
	    start_node = start_node->left;
	}
      else
        {
	  _gtk_rbtree_next_full (start_tree, start_node, &start_tree, &start_node);

	  if (!start_tree)
	    /* Ran out of tree */
	    break;
	}

      if (skip_end && start_node == end_node)
	break;
    }
  while (TRUE);
}

static void
rotator_update_rubber_band_selection (Rotator *tree_view)
{
  GtkRBTree *start_tree, *end_tree;
  GtkRBNode *start_node, *end_node;

  _gtk_rbtree_find_offset (tree_view->priv->tree, MIN (tree_view->priv->press_start_y, tree_view->priv->rubber_band_y), &start_tree, &start_node);
  _gtk_rbtree_find_offset (tree_view->priv->tree, MAX (tree_view->priv->press_start_y, tree_view->priv->rubber_band_y), &end_tree, &end_node);

  /* Handle the start area first */
  if (!tree_view->priv->rubber_band_start_node)
    {
      rotator_update_rubber_band_selection_range (tree_view,
						       start_tree,
						       start_node,
						       end_tree,
						       end_node,
						       TRUE,
						       FALSE,
						       FALSE);
    }
  else if (_gtk_rbtree_node_find_offset (start_tree, start_node) <
           _gtk_rbtree_node_find_offset (tree_view->priv->rubber_band_start_tree, tree_view->priv->rubber_band_start_node))
    {
      /* New node is above the old one; selection became bigger */
      rotator_update_rubber_band_selection_range (tree_view,
						       start_tree,
						       start_node,
						       tree_view->priv->rubber_band_start_tree,
						       tree_view->priv->rubber_band_start_node,
						       TRUE,
						       FALSE,
						       TRUE);
    }
  else if (_gtk_rbtree_node_find_offset (start_tree, start_node) >
           _gtk_rbtree_node_find_offset (tree_view->priv->rubber_band_start_tree, tree_view->priv->rubber_band_start_node))
    {
      /* New node is below the old one; selection became smaller */
      rotator_update_rubber_band_selection_range (tree_view,
						       tree_view->priv->rubber_band_start_tree,
						       tree_view->priv->rubber_band_start_node,
						       start_tree,
						       start_node,
						       FALSE,
						       FALSE,
						       TRUE);
    }

  tree_view->priv->rubber_band_start_tree = start_tree;
  tree_view->priv->rubber_band_start_node = start_node;

  /* Next, handle the end area */
  if (!tree_view->priv->rubber_band_end_node)
    {
      /* In the event this happens, start_node was also NULL; this case is
       * handled above.
       */
    }
  else if (!end_node)
    {
      /* Find the last node in the tree */
      _gtk_rbtree_find_offset (tree_view->priv->tree, tree_view->priv->height - 1,
			       &end_tree, &end_node);

      /* Selection reached end of the tree */
      rotator_update_rubber_band_selection_range (tree_view,
						       tree_view->priv->rubber_band_end_tree,
						       tree_view->priv->rubber_band_end_node,
						       end_tree,
						       end_node,
						       TRUE,
						       TRUE,
						       FALSE);
    }
  else if (_gtk_rbtree_node_find_offset (end_tree, end_node) >
           _gtk_rbtree_node_find_offset (tree_view->priv->rubber_band_end_tree, tree_view->priv->rubber_band_end_node))
    {
      /* New node is below the old one; selection became bigger */
      rotator_update_rubber_band_selection_range (tree_view,
						       tree_view->priv->rubber_band_end_tree,
						       tree_view->priv->rubber_band_end_node,
						       end_tree,
						       end_node,
						       TRUE,
						       TRUE,
						       FALSE);
    }
  else if (_gtk_rbtree_node_find_offset (end_tree, end_node) <
           _gtk_rbtree_node_find_offset (tree_view->priv->rubber_band_end_tree, tree_view->priv->rubber_band_end_node))
    {
      /* New node is above the old one; selection became smaller */
      rotator_update_rubber_band_selection_range (tree_view,
						       end_tree,
						       end_node,
						       tree_view->priv->rubber_band_end_tree,
						       tree_view->priv->rubber_band_end_node,
						       FALSE,
						       TRUE,
						       FALSE);
    }

  tree_view->priv->rubber_band_end_tree = end_tree;
  tree_view->priv->rubber_band_end_node = end_node;
}

static void
rotator_update_rubber_band (Rotator *tree_view)
{
  gint x, y;
  GdkRectangle old_area;
  GdkRectangle new_area;
  GdkRectangle common;
  GdkRegion *invalid_region;

  old_area.x = MIN (tree_view->priv->press_start_x, tree_view->priv->rubber_band_x);
  old_area.y = MIN (tree_view->priv->press_start_y, tree_view->priv->rubber_band_y) - tree_view->priv->dy;
  old_area.width = ABS (tree_view->priv->rubber_band_x - tree_view->priv->press_start_x) + 1;
  old_area.height = ABS (tree_view->priv->rubber_band_y - tree_view->priv->press_start_y) + 1;

  gdk_window_get_pointer (tree_view->priv->bin_window, &x, &y, NULL);

  x = MAX (x, 0);
  y = MAX (y, 0) + tree_view->priv->dy;

  new_area.x = MIN (tree_view->priv->press_start_x, x);
  new_area.y = MIN (tree_view->priv->press_start_y, y) - tree_view->priv->dy;
  new_area.width = ABS (x - tree_view->priv->press_start_x) + 1;
  new_area.height = ABS (y - tree_view->priv->press_start_y) + 1;

  invalid_region = gdk_region_rectangle (&old_area);
  gdk_region_union_with_rect (invalid_region, &new_area);

  gdk_rectangle_intersect (&old_area, &new_area, &common);
  if (common.width > 2 && common.height > 2)
    {
      GdkRegion *common_region;

      /* make sure the border is invalidated */
      common.x += 1;
      common.y += 1;
      common.width -= 2;
      common.height -= 2;

      common_region = gdk_region_rectangle (&common);

      gdk_region_subtract (invalid_region, common_region);
      gdk_region_destroy (common_region);
    }

  gdk_window_invalidate_region (tree_view->priv->bin_window, invalid_region, TRUE);

  gdk_region_destroy (invalid_region);

  tree_view->priv->rubber_band_x = x;
  tree_view->priv->rubber_band_y = y;

  rotator_update_rubber_band_selection (tree_view);
}

static void
rotator_paint_rubber_band (Rotator  *tree_view,
				GdkRectangle *area)
{
  cairo_t *cr;
  GdkRectangle rect;
  GdkRectangle rubber_rect;

  rubber_rect.x = MIN (tree_view->priv->press_start_x, tree_view->priv->rubber_band_x);
  rubber_rect.y = MIN (tree_view->priv->press_start_y, tree_view->priv->rubber_band_y) - tree_view->priv->dy;
  rubber_rect.width = ABS (tree_view->priv->press_start_x - tree_view->priv->rubber_band_x) + 1;
  rubber_rect.height = ABS (tree_view->priv->press_start_y - tree_view->priv->rubber_band_y) + 1;

  if (!gdk_rectangle_intersect (&rubber_rect, area, &rect))
    return;

  cr = gdk_cairo_create (tree_view->priv->bin_window);
  cairo_set_line_width (cr, 1.0);

  cairo_set_source_rgba (cr,
			 GTK_WIDGET (tree_view)->style->fg[GTK_STATE_NORMAL].red / 65535.,
			 GTK_WIDGET (tree_view)->style->fg[GTK_STATE_NORMAL].green / 65535.,
			 GTK_WIDGET (tree_view)->style->fg[GTK_STATE_NORMAL].blue / 65535.,
			 .25);

  gdk_cairo_rectangle (cr, &rect);
  cairo_clip (cr);
  cairo_paint (cr);

  cairo_set_source_rgb (cr,
			GTK_WIDGET (tree_view)->style->fg[GTK_STATE_NORMAL].red / 65535.,
			GTK_WIDGET (tree_view)->style->fg[GTK_STATE_NORMAL].green / 65535.,
			GTK_WIDGET (tree_view)->style->fg[GTK_STATE_NORMAL].blue / 65535.);

  cairo_rectangle (cr,
		   rubber_rect.x + 0.5, rubber_rect.y + 0.5,
		   rubber_rect.width - 1, rubber_rect.height - 1);
  cairo_stroke (cr);

  cairo_destroy (cr);
}

static gboolean
rotator_motion_bin_window (GtkWidget      *widget,
				 GdkEventMotion *event)
{
  Rotator *tree_view;
  GtkRBTree *tree;
  GtkRBNode *node;
  gint new_y;

  tree_view = (Rotator *) widget;

  if (tree_view->priv->tree == NULL)
    return FALSE;

  if (tree_view->priv->rubber_band_status == RUBBER_BAND_MAYBE_START)
    {
      gtk_grab_add (GTK_WIDGET (tree_view));
      rotator_update_rubber_band (tree_view);

      tree_view->priv->rubber_band_status = RUBBER_BAND_ACTIVE;
    }
  else if (tree_view->priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    {
      rotator_update_rubber_band (tree_view);

      add_scroll_timeout (tree_view);
    }

  /* only check for an initiated drag when a button is pressed */
  if (tree_view->priv->pressed_button >= 0
      && !tree_view->priv->rubber_band_status)
    rotator_maybe_begin_dragging_row (tree_view, event);

  new_y = TREE_WINDOW_Y_TO_RBTREE_Y(tree_view, event->y);
  if (new_y < 0)
    new_y = 0;

  _gtk_rbtree_find_offset (tree_view->priv->tree, new_y, &tree, &node);

  /* If we are currently pressing down a button, we don't want to prelight anything else. */
  if ((tree_view->priv->button_pressed_node != NULL) &&
      (tree_view->priv->button_pressed_node != node))
    node = NULL;

  prelight_or_select (tree_view, tree, node, event->x, event->y);

  return TRUE;
}

static gboolean
rotator_motion (GtkWidget      *widget,
		      GdkEventMotion *event)
{
  Rotator *tree_view;

  tree_view = (Rotator *) widget;

  /* Resizing a column */
  if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_IN_COLUMN_RESIZE))
    return rotator_motion_resize_column (widget, event);

  /* Drag column */
  if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_IN_COLUMN_DRAG))
    return rotator_motion_drag_column (widget, event);

  /* Sanity check it */
  if (event->window == tree_view->priv->bin_window)
    return rotator_motion_bin_window (widget, event);

  return FALSE;
}

/* Invalidate the focus rectangle near the edge of the bin_window; used when
 * the tree is empty.
 */
static void
invalidate_empty_focus (Rotator *tree_view)
{
  GdkRectangle area;

  if (!GTK_WIDGET_HAS_FOCUS (tree_view))
    return;

  area.x = 0;
  area.y = 0;
  gdk_drawable_get_size (tree_view->priv->bin_window, &area.width, &area.height);
  gdk_window_invalidate_rect (tree_view->priv->bin_window, &area, FALSE);
}

/* Draws a focus rectangle near the edge of the bin_window; used when the tree
 * is empty.
 */
static void
draw_empty_focus (Rotator *tree_view, GdkRectangle *clip_area)
{
  gint w, h;

  if (!GTK_WIDGET_HAS_FOCUS (tree_view))
    return;

  gdk_drawable_get_size (tree_view->priv->bin_window, &w, &h);

  w -= 2;
  h -= 2;

  if (w > 0 && h > 0)
    gtk_paint_focus (GTK_WIDGET (tree_view)->style,
		     tree_view->priv->bin_window,
		     GTK_WIDGET_STATE (tree_view),
		     clip_area,
		     GTK_WIDGET (tree_view),
		     NULL,
		     1, 1, w, h);
}
#endif


static void
draw (Rotator* rotator)
{
	RotatorPrivate* _r = rotator->priv;

	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST); // is needed because?
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// If the gl_context is shared, this must be done on every draw
	rotator_setup_projection((AGlActor*)_r->scene);

	glPushMatrix(); // modelview matrix
	{
		GList* l = g_list_last(_r->actors);
		for(;l;l=l->prev){
			SampleActor* sa = l->data;
			AGlActor* actor = (AGlActor*)sa->actor;
			glTranslatef(0, 0, ((WaveformActor*)actor)->z);
			agl_actor__paint(actor);
			glTranslatef(0, 0, -((WaveformActor*)actor)->z);
		}
	}
	glPopMatrix();

	glDisable(GL_DEPTH_TEST);
}


static gboolean
rotator_expose (GtkWidget* widget, GdkEventExpose* event)
{
	Rotator* tree_view = GTK_ROTATOR (widget);
	RotatorPrivate* _r = tree_view->priv;

	if (_r->scene && gdk_gl_drawable_make_current (_r->scene->gl.gdk.drawable, _r->scene->gl.gdk.context)) {
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		draw((Rotator*)widget);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

#ifdef USE_SYSTEM_GTKGLEXT
		gdk_gl_drawable_swap_buffers(_r->scene->gl.gdk.drawable);
#else
		gdk_gl_window_swap_buffers(_r->scene->gl.gdk.drawable);
#endif
	}

	return true;
}

#if 0
enum
{
  DROP_HOME,
  DROP_RIGHT,
  DROP_LEFT,
  DROP_END
};

/* returns 0x1 when no column has been found -- yes it's hackish */
static RotatorColumn *
rotator_get_drop_column (Rotator       *tree_view,
			       RotatorColumn *column,
			       gint               drop_position)
{
  RotatorColumn *left_column = NULL;
  RotatorColumn *cur_column = NULL;
  GList *tmp_list;

  if (!column->reorderable)
    return (RotatorColumn *)0x1;

  switch (drop_position)
    {
      case DROP_HOME:
	/* find first column where we can drop */
	tmp_list = tree_view->priv->columns;
	if (column == GTK_ROTATOR_COLUMN (tmp_list->data))
	  return (RotatorColumn *)0x1;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    cur_column = GTK_ROTATOR_COLUMN (tmp_list->data);
	    tmp_list = tmp_list->next;

	    if (left_column && left_column->visible == FALSE)
	      continue;

	    if (!tree_view->priv->column_drop_func)
	      return left_column;

	    if (!tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	      {
		left_column = cur_column;
		continue;
	      }

	    return left_column;
	  }

	if (!tree_view->priv->column_drop_func)
	  return left_column;

	if (tree_view->priv->column_drop_func (tree_view, column, left_column, NULL, tree_view->priv->column_drop_func_data))
	  return left_column;
	else
	  return (RotatorColumn *)0x1;
	break;

      case DROP_RIGHT:
	/* find first column after column where we can drop */
	tmp_list = tree_view->priv->columns;

	for (; tmp_list; tmp_list = tmp_list->next)
	  if (GTK_ROTATOR_COLUMN (tmp_list->data) == column)
	    break;

	if (!tmp_list || !tmp_list->next)
	  return (RotatorColumn *)0x1;

	tmp_list = tmp_list->next;
	left_column = GTK_ROTATOR_COLUMN (tmp_list->data);
	tmp_list = tmp_list->next;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    cur_column = GTK_ROTATOR_COLUMN (tmp_list->data);
	    tmp_list = tmp_list->next;

	    if (left_column && left_column->visible == FALSE)
	      {
		left_column = cur_column;
		if (tmp_list)
		  tmp_list = tmp_list->next;
	        continue;
	      }

	    if (!tree_view->priv->column_drop_func)
	      return left_column;

	    if (!tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	      {
		left_column = cur_column;
		continue;
	      }

	    return left_column;
	  }

	if (!tree_view->priv->column_drop_func)
	  return left_column;

	if (tree_view->priv->column_drop_func (tree_view, column, left_column, NULL, tree_view->priv->column_drop_func_data))
	  return left_column;
	else
	  return (RotatorColumn *)0x1;
	break;

      case DROP_LEFT:
	/* find first column before column where we can drop */
	tmp_list = tree_view->priv->columns;

	for (; tmp_list; tmp_list = tmp_list->next)
	  if (GTK_ROTATOR_COLUMN (tmp_list->data) == column)
	    break;

	if (!tmp_list || !tmp_list->prev)
	  return (RotatorColumn *)0x1;

	tmp_list = tmp_list->prev;
	cur_column = GTK_ROTATOR_COLUMN (tmp_list->data);
	tmp_list = tmp_list->prev;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    left_column = GTK_ROTATOR_COLUMN (tmp_list->data);

	    if (left_column && !left_column->visible)
	      {
		/*if (!tmp_list->prev)
		  return (RotatorColumn *)0x1;
		  */
/*
		cur_column = GTK_ROTATOR_COLUMN (tmp_list->prev->data);
		tmp_list = tmp_list->prev->prev;
		continue;*/

		cur_column = left_column;
		if (tmp_list)
		  tmp_list = tmp_list->prev;
		continue;
	      }

	    if (!tree_view->priv->column_drop_func)
	      return left_column;

	    if (tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	      return left_column;

	    cur_column = left_column;
	    tmp_list = tmp_list->prev;
	  }

	if (!tree_view->priv->column_drop_func)
	  return NULL;

	if (tree_view->priv->column_drop_func (tree_view, column, NULL, cur_column, tree_view->priv->column_drop_func_data))
	  return NULL;
	else
	  return (RotatorColumn *)0x1;
	break;

      case DROP_END:
	/* same as DROP_HOME case, but doing it backwards */
	tmp_list = g_list_last (tree_view->priv->columns);
	cur_column = NULL;

	if (column == GTK_ROTATOR_COLUMN (tmp_list->data))
	  return (RotatorColumn *)0x1;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    left_column = GTK_ROTATOR_COLUMN (tmp_list->data);

	    if (left_column && !left_column->visible)
	      {
		cur_column = left_column;
		tmp_list = tmp_list->prev;
	      }

	    if (!tree_view->priv->column_drop_func)
	      return left_column;

	    if (tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	      return left_column;

	    cur_column = left_column;
	    tmp_list = tmp_list->prev;
	  }

	if (!tree_view->priv->column_drop_func)
	  return NULL;

	if (tree_view->priv->column_drop_func (tree_view, column, NULL, cur_column, tree_view->priv->column_drop_func_data))
	  return NULL;
	else
	  return (RotatorColumn *)0x1;
	break;
    }

  return (RotatorColumn *)0x1;
}

static gboolean
rotator_key_press (GtkWidget   *widget,
			 GdkEventKey *event)
{
  Rotator *tree_view = (Rotator *) widget;

  if (tree_view->priv->rubber_band_status)
    {
      if (event->keyval == GDK_Escape)
	rotator_stop_rubber_band (tree_view);

      return TRUE;
    }

  if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_IN_COLUMN_DRAG))
    {
      if (event->keyval == GDK_Escape)
	{
	  tree_view->priv->cur_reorder = NULL;
	  rotator_button_release_drag_column (widget, NULL);
	}
      return TRUE;
    }

  if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_HEADERS_VISIBLE))
    {
      GList *focus_column;
      gboolean rtl;

      rtl = (gtk_widget_get_direction (GTK_WIDGET (tree_view)) == GTK_TEXT_DIR_RTL);

      for (focus_column = tree_view->priv->columns;
           focus_column;
           focus_column = focus_column->next)
        {
          RotatorColumn *column = GTK_ROTATOR_COLUMN (focus_column->data);

          if (GTK_WIDGET_HAS_FOCUS (column->button))
            break;
        }

      if (focus_column &&
          (event->state & GDK_SHIFT_MASK) && (event->state & GDK_MOD1_MASK) &&
          (event->keyval == GDK_Left || event->keyval == GDK_KP_Left
           || event->keyval == GDK_Right || event->keyval == GDK_KP_Right))
        {
          RotatorColumn *column = GTK_ROTATOR_COLUMN (focus_column->data);

          if (!column->resizable)
            {
              gtk_widget_error_bell (widget);
              return TRUE;
            }

          if (event->keyval == (rtl ? GDK_Right : GDK_Left)
              || event->keyval == (rtl ? GDK_KP_Right : GDK_KP_Left))
            {
              gint old_width = column->resized_width;

              column->resized_width = MAX (column->resized_width,
                                           column->width);
              column->resized_width -= 2;
              if (column->resized_width < 0)
                column->resized_width = 0;

              if (column->min_width == -1)
                column->resized_width = MAX (column->button->requisition.width,
                                             column->resized_width);
              else
                column->resized_width = MAX (column->min_width,
                                             column->resized_width);

              if (column->max_width != -1)
                column->resized_width = MIN (column->resized_width,
                                             column->max_width);

              column->use_resized_width = TRUE;

              if (column->resized_width != old_width)
                gtk_widget_queue_resize (widget);
              else
                gtk_widget_error_bell (widget);
            }
          else if (event->keyval == (rtl ? GDK_Left : GDK_Right)
                   || event->keyval == (rtl ? GDK_KP_Left : GDK_KP_Right))
            {
              gint old_width = column->resized_width;

              column->resized_width = MAX (column->resized_width,
                                           column->width);
              column->resized_width += 2;

              if (column->max_width != -1)
                column->resized_width = MIN (column->resized_width,
                                             column->max_width);

              column->use_resized_width = TRUE;

              if (column->resized_width != old_width)
                gtk_widget_queue_resize (widget);
              else
                gtk_widget_error_bell (widget);
            }

          return TRUE;
        }

      if (focus_column &&
          (event->state & GDK_MOD1_MASK) &&
          (event->keyval == GDK_Left || event->keyval == GDK_KP_Left
           || event->keyval == GDK_Right || event->keyval == GDK_KP_Right
           || event->keyval == GDK_Home || event->keyval == GDK_KP_Home
           || event->keyval == GDK_End || event->keyval == GDK_KP_End))
        {
          RotatorColumn *column = GTK_ROTATOR_COLUMN (focus_column->data);

          if (event->keyval == (rtl ? GDK_Right : GDK_Left)
              || event->keyval == (rtl ? GDK_KP_Right : GDK_KP_Left))
            {
              RotatorColumn *col;
              col = rotator_get_drop_column (tree_view, column, DROP_LEFT);
              if (col != (RotatorColumn *)0x1)
                rotator_move_column_after (tree_view, column, col);
              else
                gtk_widget_error_bell (widget);
            }
          else if (event->keyval == (rtl ? GDK_Left : GDK_Right)
                   || event->keyval == (rtl ? GDK_KP_Left : GDK_KP_Right))
            {
              RotatorColumn *col;
              col = rotator_get_drop_column (tree_view, column, DROP_RIGHT);
              if (col != (RotatorColumn *)0x1)
                rotator_move_column_after (tree_view, column, col);
              else
                gtk_widget_error_bell (widget);
            }
          else if (event->keyval == GDK_Home || event->keyval == GDK_KP_Home)
            {
              RotatorColumn *col;
              col = rotator_get_drop_column (tree_view, column, DROP_HOME);
              if (col != (RotatorColumn *)0x1)
                rotator_move_column_after (tree_view, column, col);
              else
                gtk_widget_error_bell (widget);
            }
          else if (event->keyval == GDK_End || event->keyval == GDK_KP_End)
            {
              RotatorColumn *col;
              col = rotator_get_drop_column (tree_view, column, DROP_END);
              if (col != (RotatorColumn *)0x1)
                rotator_move_column_after (tree_view, column, col);
              else
                gtk_widget_error_bell (widget);
            }

          return TRUE;
        }
    }

  /* Chain up to the parent class.  It handles the keybindings. */
  if (GTK_WIDGET_CLASS (rotator_parent_class)->key_press_event (widget, event))
    return TRUE;

  /* We pass the event to the search_entry.  If its text changes, then we start
   * the typeahead find capabilities. */
  if (GTK_WIDGET_HAS_FOCUS (tree_view)
      && tree_view->priv->enable_search
      && !tree_view->priv->search_custom_entry_set)
    {
      GdkEvent *new_event;
      char *old_text;
      const char *new_text;
      gboolean retval;
      GdkScreen *screen;
      gboolean text_modified;
      gulong popup_menu_id;

      rotator_ensure_interactive_directory (tree_view);

      /* Make a copy of the current text */
      old_text = g_strdup (gtk_entry_get_text (GTK_ENTRY (tree_view->priv->search_entry)));
      new_event = gdk_event_copy ((GdkEvent *) event);
      g_object_unref (((GdkEventKey *) new_event)->window);
      ((GdkEventKey *) new_event)->window = g_object_ref (tree_view->priv->search_window->window);
      gtk_widget_realize (tree_view->priv->search_window);

      popup_menu_id = g_signal_connect (tree_view->priv->search_entry, 
					"popup-menu", G_CALLBACK (gtk_true),
                                        NULL);

      /* Move the entry off screen */
      screen = gtk_widget_get_screen (GTK_WIDGET (tree_view));
      gtk_window_move (GTK_WINDOW (tree_view->priv->search_window),
		       gdk_screen_get_width (screen) + 1,
		       gdk_screen_get_height (screen) + 1);
      gtk_widget_show (tree_view->priv->search_window);

      /* Send the event to the window.  If the preedit_changed signal is emitted
       * during this event, we will set priv->imcontext_changed  */
      tree_view->priv->imcontext_changed = FALSE;
      retval = gtk_widget_event (tree_view->priv->search_window, new_event);
      gdk_event_free (new_event);
      gtk_widget_hide (tree_view->priv->search_window);

      g_signal_handler_disconnect (tree_view->priv->search_entry, 
				   popup_menu_id);

      /* We check to make sure that the entry tried to handle the text, and that
       * the text has changed.
       */
      new_text = gtk_entry_get_text (GTK_ENTRY (tree_view->priv->search_entry));
      text_modified = strcmp (old_text, new_text) != 0;
      g_free (old_text);
      if (tree_view->priv->imcontext_changed ||    /* we're in a preedit */
	  (retval && text_modified))               /* ...or the text was modified */
	{
	  if (rotator_real_start_interactive_search (tree_view, FALSE))
	    {
	      gtk_widget_grab_focus (GTK_WIDGET (tree_view));
	      return TRUE;
	    }
	  else
	    {
	      gtk_entry_set_text (GTK_ENTRY (tree_view->priv->search_entry), "");
	      return FALSE;
	    }
	}
    }

  return FALSE;
}

static gboolean
rotator_key_release (GtkWidget   *widget,
			   GdkEventKey *event)
{
  Rotator *tree_view = GTK_ROTATOR (widget);

  if (tree_view->priv->rubber_band_status)
    return TRUE;

  return GTK_WIDGET_CLASS (rotator_parent_class)->key_release_event (widget, event);
}

/* FIXME Is this function necessary? Can I get an enter_notify event
 * w/o either an expose event or a mouse motion event?
 */
static gboolean
rotator_enter_notify (GtkWidget        *widget,
			    GdkEventCrossing *event)
{
  Rotator *tree_view = GTK_ROTATOR (widget);
  GtkRBTree *tree;
  GtkRBNode *node;
  gint new_y;

  /* Sanity check it */
  if (event->window != tree_view->priv->bin_window)
    return FALSE;

  if (tree_view->priv->tree == NULL)
    return FALSE;

  /* find the node internally */
  new_y = TREE_WINDOW_Y_TO_RBTREE_Y(tree_view, event->y);
  if (new_y < 0)
    new_y = 0;
  _gtk_rbtree_find_offset (tree_view->priv->tree, new_y, &tree, &node);

  if ((tree_view->priv->button_pressed_node == NULL) ||
      (tree_view->priv->button_pressed_node == node))
    prelight_or_select (tree_view, tree, node, event->x, event->y);

  return TRUE;
}

static gboolean
rotator_leave_notify (GtkWidget        *widget,
			    GdkEventCrossing *event)
{
  Rotator *tree_view;

  if (event->mode == GDK_CROSSING_GRAB)
    return TRUE;

  tree_view = GTK_ROTATOR (widget);

  if (tree_view->priv->prelight_node)
    _rotator_queue_draw_node (tree_view,
                                   tree_view->priv->prelight_tree,
                                   tree_view->priv->prelight_node,
                                   NULL);

  prelight_or_select (tree_view,
		      NULL, NULL,
		      -1000, -1000); /* coords not possibly over an arrow */

  return TRUE;
}


static gint
rotator_focus_out (GtkWidget     *widget, GdkEventFocus *event)
{
  Rotator *tree_view;

  tree_view = GTK_ROTATOR (widget);

  gtk_widget_queue_draw (widget);

  /* destroy interactive search dialog */
  if (tree_view->priv->search_window)
    rotator_search_dialog_hide (tree_view->priv->search_window, tree_view);

  return FALSE;
}


/* Incremental Reflow
 */

static void
rotator_node_queue_redraw (Rotator *tree_view,
				 GtkRBTree   *tree,
				 GtkRBNode   *node)
{
  gint y;

  y = _gtk_rbtree_node_find_offset (tree, node)
    - tree_view->priv->vadjustment->value
    + ROTATOR_HEADER_HEIGHT (tree_view);

  gtk_widget_queue_draw_area (GTK_WIDGET (tree_view),
			      0, y,
			      GTK_WIDGET (tree_view)->allocation.width,
			      GTK_RBNODE_GET_HEIGHT (node));
}

static gboolean
node_is_visible (Rotator *tree_view,
		 GtkRBTree   *tree,
		 GtkRBNode   *node)
{
  int y;
  int height;

  y = _gtk_rbtree_node_find_offset (tree, node);
  height = ROW_HEIGHT (tree_view, GTK_RBNODE_GET_HEIGHT (node));

  if (y >= tree_view->priv->vadjustment->value &&
      y + height <= (tree_view->priv->vadjustment->value
	             + tree_view->priv->vadjustment->page_size))
    return TRUE;

  return FALSE;
}

/* Returns TRUE if it updated the size
 */
static gboolean
validate_row (Rotator *tree_view,
	      GtkRBTree   *tree,
	      GtkRBNode   *node,
	      GtkTreeIter *iter,
	      GtkTreePath *path)
{
  RotatorColumn *column;
  GList *list, *first_column, *last_column;
  gint height = 0;
  gint horizontal_separator;
  gint vertical_separator;
  gint focus_line_width;
  gint depth = gtk_tree_path_get_depth (path);
  gboolean retval = FALSE;
  gboolean is_separator = FALSE;
  gboolean draw_vgrid_lines, draw_hgrid_lines;
  gint focus_pad;
  gint grid_line_width;
  gboolean wide_separators;
  gint separator_height;

  /* double check the row needs validating */
  if (! GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_INVALID) &&
      ! GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_COLUMN_INVALID))
    return FALSE;

  is_separator = row_is_separator (tree_view, iter, NULL);

  gtk_widget_style_get (GTK_WIDGET (tree_view),
			"focus-padding", &focus_pad,
			"focus-line-width", &focus_line_width,
			"horizontal-separator", &horizontal_separator,
			"vertical-separator", &vertical_separator,
			"grid-line-width", &grid_line_width,
                        "wide-separators",  &wide_separators,
                        "separator-height", &separator_height,
			NULL);
  
  draw_vgrid_lines =
    tree_view->priv->grid_lines == GTK_ROTATOR_GRID_LINES_VERTICAL
    || tree_view->priv->grid_lines == GTK_ROTATOR_GRID_LINES_BOTH;
  draw_hgrid_lines =
    tree_view->priv->grid_lines == GTK_ROTATOR_GRID_LINES_HORIZONTAL
    || tree_view->priv->grid_lines == GTK_ROTATOR_GRID_LINES_BOTH;

  for (last_column = g_list_last (tree_view->priv->columns);
       last_column && !(GTK_ROTATOR_COLUMN (last_column->data)->visible);
       last_column = last_column->prev)
    ;

  for (first_column = g_list_first (tree_view->priv->columns);
       first_column && !(GTK_ROTATOR_COLUMN (first_column->data)->visible);
       first_column = first_column->next)
    ;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      gint tmp_width;
      gint tmp_height;

      column = list->data;

      if (! column->visible)
	continue;

      if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_COLUMN_INVALID) && !column->dirty)
	continue;

      rotator_column_cell_set_cell_data (column, tree_view->priv->model, iter,
					       GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_PARENT),
					       node->children?TRUE:FALSE);
      rotator_column_cell_get_size (column,
					  NULL, NULL, NULL,
					  &tmp_width, &tmp_height);

      if (!is_separator)
	{
          tmp_height += vertical_separator;
	  height = MAX (height, tmp_height);
	  height = MAX (height, tree_view->priv->expander_size);
	}
      else
        {
          if (wide_separators)
            height = separator_height + 2 * focus_pad;
          else
            height = 2 + 2 * focus_pad;
        }

      if (rotator_is_expander_column (tree_view, column))
        {
	  tmp_width = tmp_width + horizontal_separator + (depth - 1) * tree_view->priv->level_indentation;

	  if (ROTATOR_DRAW_EXPANDERS (tree_view))
	    tmp_width += depth * tree_view->priv->expander_size;
	}
      else
	tmp_width = tmp_width + horizontal_separator;

      if (draw_vgrid_lines)
        {
	  if (list->data == first_column || list->data == last_column)
	    tmp_width += grid_line_width / 2.0;
	  else
	    tmp_width += grid_line_width;
	}

      if (tmp_width > column->requested_width)
	{
	  retval = TRUE;
	  column->requested_width = tmp_width;
	}
    }

  if (draw_hgrid_lines)
    height += grid_line_width;

  if (height != GTK_RBNODE_GET_HEIGHT (node))
    {
      retval = TRUE;
      _gtk_rbtree_node_set_height (tree, node, height);
    }
  _gtk_rbtree_node_mark_valid (tree, node);
  tree_view->priv->post_validation_flag = TRUE;

  return retval;
}


static void
validate_visible_area (Rotator *tree_view)
{
  GtkTreePath *path = NULL;
  GtkTreePath *above_path = NULL;
  GtkTreeIter iter;
  GtkRBTree *tree = NULL;
  GtkRBNode *node = NULL;
  gboolean need_redraw = FALSE;
  gboolean size_changed = FALSE;
  gint total_height;
  gint area_above = 0;
  gint area_below = 0;

  if (tree_view->priv->tree == NULL)
    return;

  if (! GTK_RBNODE_FLAG_SET (tree_view->priv->tree->root, GTK_RBNODE_DESCENDANTS_INVALID) &&
      tree_view->priv->scroll_to_path == NULL)
    return;

  total_height = GTK_WIDGET (tree_view)->allocation.height - ROTATOR_HEADER_HEIGHT (tree_view);

  if (total_height == 0)
    return;

  /* First, we check to see if we need to scroll anywhere
   */
  if (tree_view->priv->scroll_to_path)
    {
      path = gtk_tree_row_reference_get_path (tree_view->priv->scroll_to_path);
      if (path && !_rotator_find_node (tree_view, path, &tree, &node))
	{
          /* we are going to scroll, and will update dy */
	  gtk_tree_model_get_iter (tree_view->priv->model, &iter, path);
	  if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_INVALID) ||
	      GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_COLUMN_INVALID))
	    {
	      _rotator_queue_draw_node (tree_view, tree, node, NULL);
	      if (validate_row (tree_view, tree, node, &iter, path))
		size_changed = TRUE;
	    }

	  if (tree_view->priv->scroll_to_use_align)
	    {
	      gint height = ROW_HEIGHT (tree_view, GTK_RBNODE_GET_HEIGHT (node));
	      area_above = (total_height - height) *
		tree_view->priv->scroll_to_row_align;
	      area_below = total_height - area_above - height;
	      area_above = MAX (area_above, 0);
	      area_below = MAX (area_below, 0);
	    }
	  else
	    {
	      /* two cases:
	       * 1) row not visible
	       * 2) row visible
	       */
	      gint dy;
	      gint height = ROW_HEIGHT (tree_view, GTK_RBNODE_GET_HEIGHT (node));

	      dy = _gtk_rbtree_node_find_offset (tree, node);

	      if (dy >= tree_view->priv->vadjustment->value &&
		  dy + height <= (tree_view->priv->vadjustment->value
		                  + tree_view->priv->vadjustment->page_size))
	        {
		  /* row visible: keep the row at the same position */
		  area_above = dy - tree_view->priv->vadjustment->value;
		  area_below = (tree_view->priv->vadjustment->value +
		                tree_view->priv->vadjustment->page_size)
		               - dy - height;
		}
	      else
	        {
		  /* row not visible */
		  if (dy >= 0
		      && dy + height <= tree_view->priv->vadjustment->page_size)
		    {
		      /* row at the beginning -- fixed */
		      area_above = dy;
		      area_below = tree_view->priv->vadjustment->page_size
				   - area_above - height;
		    }
		  else if (dy >= (tree_view->priv->vadjustment->upper -
			          tree_view->priv->vadjustment->page_size))
		    {
		      /* row at the end -- fixed */
		      area_above = dy - (tree_view->priv->vadjustment->upper -
			           tree_view->priv->vadjustment->page_size);
                      area_below = tree_view->priv->vadjustment->page_size -
                                   area_above - height;

                      if (area_below < 0)
                        {
			  area_above = tree_view->priv->vadjustment->page_size - height;
                          area_below = 0;
                        }
		    }
		  else
		    {
		      /* row somewhere in the middle, bring it to the top
		       * of the view
		       */
		      area_above = 0;
		      area_below = total_height - height;
		    }
		}
	    }
	}
      else
	/* the scroll to isn't valid; ignore it.
	 */
	{
	  if (tree_view->priv->scroll_to_path && !path)
	    {
	      gtk_tree_row_reference_free (tree_view->priv->scroll_to_path);
	      tree_view->priv->scroll_to_path = NULL;
	    }
	  if (path)
	    gtk_tree_path_free (path);
	  path = NULL;
	}      
    }

  /* We didn't have a scroll_to set, so we just handle things normally
   */
  if (path == NULL)
    {
      gint offset;

      offset = _gtk_rbtree_find_offset (tree_view->priv->tree,
					TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, 0),
					&tree, &node);
      if (node == NULL)
	{
	  /* In this case, nothing has been validated */
	  path = gtk_tree_path_new_first ();
	  _rotator_find_node (tree_view, path, &tree, &node);
	}
      else
	{
	  path = _rotator_find_path (tree_view, tree, node);
	  total_height += offset;
	}

      gtk_tree_model_get_iter (tree_view->priv->model, &iter, path);

      if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_INVALID) ||
	  GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_COLUMN_INVALID))
	{
	  _rotator_queue_draw_node (tree_view, tree, node, NULL);
	  if (validate_row (tree_view, tree, node, &iter, path))
	    size_changed = TRUE;
	}
      area_above = 0;
      area_below = total_height - ROW_HEIGHT (tree_view, GTK_RBNODE_GET_HEIGHT (node));
    }

  above_path = gtk_tree_path_copy (path);

  /* if we do not validate any row above the new top_row, we will make sure
   * that the row immediately above top_row has been validated. (if we do not
   * do this, _gtk_rbtree_find_offset will find the row above top_row, because
   * when invalidated that row's height will be zero. and this will mess up
   * scrolling).
   */
  if (area_above == 0)
    {
      GtkRBTree *tmptree;
      GtkRBNode *tmpnode;

      _rotator_find_node (tree_view, above_path, &tmptree, &tmpnode);
      _gtk_rbtree_prev_full (tmptree, tmpnode, &tmptree, &tmpnode);

      if (tmpnode)
        {
	  GtkTreePath *tmppath;
	  GtkTreeIter tmpiter;

	  tmppath = _rotator_find_path (tree_view, tmptree, tmpnode);
	  gtk_tree_model_get_iter (tree_view->priv->model, &tmpiter, tmppath);

	  if (GTK_RBNODE_FLAG_SET (tmpnode, GTK_RBNODE_INVALID) ||
	      GTK_RBNODE_FLAG_SET (tmpnode, GTK_RBNODE_COLUMN_INVALID))
	    {
	      _rotator_queue_draw_node (tree_view, tmptree, tmpnode, NULL);
	      if (validate_row (tree_view, tmptree, tmpnode, &tmpiter, tmppath))
		size_changed = TRUE;
	    }

	  gtk_tree_path_free (tmppath);
	}
    }

  /* Now, we walk forwards and backwards, measuring rows. Unfortunately,
   * backwards is much slower then forward, as there is no iter_prev function.
   * We go forwards first in case we run out of tree.  Then we go backwards to
   * fill out the top.
   */
  while (node && area_below > 0)
    {
      if (node->children)
	{
	  GtkTreeIter parent = iter;
	  gboolean has_child;

	  tree = node->children;
	  node = tree->root;

          g_assert (node != tree->nil);

	  while (node->left != tree->nil)
	    node = node->left;
	  has_child = gtk_tree_model_iter_children (tree_view->priv->model,
						    &iter,
						    &parent);
	  ROTATOR_INTERNAL_ASSERT_VOID (has_child);
	  gtk_tree_path_down (path);
	}
      else
	{
	  gboolean done = FALSE;
	  do
	    {
	      node = _gtk_rbtree_next (tree, node);
	      if (node != NULL)
		{
		  gboolean has_next = gtk_tree_model_iter_next (tree_view->priv->model, &iter);
		  done = TRUE;
		  gtk_tree_path_next (path);

		  /* Sanity Check! */
		  ROTATOR_INTERNAL_ASSERT_VOID (has_next);
		}
	      else
		{
		  GtkTreeIter parent_iter = iter;
		  gboolean has_parent;

		  node = tree->parent_node;
		  tree = tree->parent_tree;
		  if (tree == NULL)
		    break;
		  has_parent = gtk_tree_model_iter_parent (tree_view->priv->model,
							   &iter,
							   &parent_iter);
		  gtk_tree_path_up (path);

		  /* Sanity check */
		  ROTATOR_INTERNAL_ASSERT_VOID (has_parent);
		}
	    }
	  while (!done);
	}

      if (!node)
        break;

      if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_INVALID) ||
	  GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_COLUMN_INVALID))
	{
	  _rotator_queue_draw_node (tree_view, tree, node, NULL);
	  if (validate_row (tree_view, tree, node, &iter, path))
	      size_changed = TRUE;
	}

      area_below -= ROW_HEIGHT (tree_view, GTK_RBNODE_GET_HEIGHT (node));
    }
  gtk_tree_path_free (path);

  /* If we ran out of tree, and have extra area_below left, we need to add it
   * to area_above */
  if (area_below > 0)
    area_above += area_below;

  _rotator_find_node (tree_view, above_path, &tree, &node);

  /* We walk backwards */
  while (area_above > 0)
    {
      _gtk_rbtree_prev_full (tree, node, &tree, &node);

      /* Always find the new path in the tree.  We cannot just assume
       * a gtk_tree_path_prev() is enough here, as there might be children
       * in between this node and the previous sibling node.  If this
       * appears to be a performance hotspot in profiles, we can look into
       * intrigate logic for keeping path, node and iter in sync like
       * we do for forward walks.  (Which will be hard because of the lacking
       * iter_prev).
       */

      if (node == NULL)
	break;

      gtk_tree_path_free (above_path);
      above_path = _rotator_find_path (tree_view, tree, node);

      gtk_tree_model_get_iter (tree_view->priv->model, &iter, above_path);

      if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_INVALID) ||
	  GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_COLUMN_INVALID))
	{
	  _rotator_queue_draw_node (tree_view, tree, node, NULL);
	  if (validate_row (tree_view, tree, node, &iter, above_path))
	    size_changed = TRUE;
	}
      area_above -= ROW_HEIGHT (tree_view, GTK_RBNODE_GET_HEIGHT (node));
    }

  /* if we scrolled to a path, we need to set the dy here,
   * and sync the top row accordingly
   */
  if (tree_view->priv->scroll_to_path)
    {
      rotator_set_top_row (tree_view, above_path, -area_above);
      rotator_top_row_to_dy (tree_view);

      need_redraw = TRUE;
    }
  else if (tree_view->priv->height <= tree_view->priv->vadjustment->page_size)
    {
      /* when we are not scrolling, we should never set dy to something
       * else than zero. we update top_row to be in sync with dy = 0.
       */
      gtk_adjustment_set_value (GTK_ADJUSTMENT (tree_view->priv->vadjustment), 0);
    }
  else if (tree_view->priv->vadjustment->value + tree_view->priv->vadjustment->page_size > tree_view->priv->height)
    {
      gtk_adjustment_set_value (GTK_ADJUSTMENT (tree_view->priv->vadjustment), tree_view->priv->height - tree_view->priv->vadjustment->page_size);
    }
  else
    rotator_top_row_to_dy (tree_view);

  /* update width/height and queue a resize */
  if (size_changed)
    {
      GtkRequisition requisition;

      /* We temporarily guess a size, under the assumption that it will be the
       * same when we get our next size_allocate.  If we don't do this, we'll be
       * in an inconsistent state if we call top_row_to_dy. */

      gtk_widget_size_request (GTK_WIDGET (tree_view), &requisition);
      tree_view->priv->hadjustment->upper = MAX (tree_view->priv->hadjustment->upper, (gfloat)requisition.width);
      tree_view->priv->vadjustment->upper = MAX (tree_view->priv->vadjustment->upper, (gfloat)requisition.height);
      gtk_adjustment_changed (tree_view->priv->hadjustment);
      gtk_adjustment_changed (tree_view->priv->vadjustment);
      gtk_widget_queue_resize (GTK_WIDGET (tree_view));
    }

  if (tree_view->priv->scroll_to_path)
    {
      gtk_tree_row_reference_free (tree_view->priv->scroll_to_path);
      tree_view->priv->scroll_to_path = NULL;
    }

  if (above_path)
    gtk_tree_path_free (above_path);

  if (tree_view->priv->scroll_to_column)
    {
      tree_view->priv->scroll_to_column = NULL;
    }
  if (need_redraw)
    gtk_widget_queue_draw (GTK_WIDGET (tree_view));
}

static void
initialize_fixed_height_mode (Rotator *tree_view)
{
  if (!tree_view->priv->tree)
    return;

  if (tree_view->priv->fixed_height < 0)
    {
      GtkTreeIter iter;
      GtkTreePath *path;

      GtkRBTree *tree = NULL;
      GtkRBNode *node = NULL;

      tree = tree_view->priv->tree;
      node = tree->root;

      path = _rotator_find_path (tree_view, tree, node);
      gtk_tree_model_get_iter (tree_view->priv->model, &iter, path);

      validate_row (tree_view, tree, node, &iter, path);

      gtk_tree_path_free (path);

      tree_view->priv->fixed_height = ROW_HEIGHT (tree_view, GTK_RBNODE_GET_HEIGHT (node));
    }

   _gtk_rbtree_set_fixed_height (tree_view->priv->tree,
                                 tree_view->priv->fixed_height, TRUE);
}

/* Our strategy for finding nodes to validate is a little convoluted.  We find
 * the left-most uninvalidated node.  We then try walking right, validating
 * nodes.  Once we find a valid node, we repeat the previous process of finding
 * the first invalid node.
 */

static gboolean
do_validate_rows (Rotator *tree_view, gboolean queue_resize)
{
  GtkRBTree *tree = NULL;
  GtkRBNode *node = NULL;
  gboolean validated_area = FALSE;
  gint retval = TRUE;
  GtkTreePath *path = NULL;
  GtkTreeIter iter;
  gint i = 0;

  gint prev_height = -1;
  gboolean fixed_height = TRUE;

  g_assert (tree_view);

  if (tree_view->priv->tree == NULL)
      return FALSE;

  if (tree_view->priv->fixed_height_mode)
    {
      if (tree_view->priv->fixed_height < 0)
        initialize_fixed_height_mode (tree_view);

      return FALSE;
    }

  do
    {
      if (! GTK_RBNODE_FLAG_SET (tree_view->priv->tree->root, GTK_RBNODE_DESCENDANTS_INVALID))
	{
	  retval = FALSE;
	  goto done;
	}

      if (path != NULL)
	{
	  node = _gtk_rbtree_next (tree, node);
	  if (node != NULL)
	    {
	      ROTATOR_INTERNAL_ASSERT (gtk_tree_model_iter_next (tree_view->priv->model, &iter), FALSE);
	      gtk_tree_path_next (path);
	    }
	  else
	    {
	      gtk_tree_path_free (path);
	      path = NULL;
	    }
	}

      if (path == NULL)
	{
	  tree = tree_view->priv->tree;
	  node = tree_view->priv->tree->root;

	  g_assert (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_DESCENDANTS_INVALID));

	  do
	    {
	      if (node->left != tree->nil &&
		  GTK_RBNODE_FLAG_SET (node->left, GTK_RBNODE_DESCENDANTS_INVALID))
		{
		  node = node->left;
		}
	      else if (node->right != tree->nil &&
		       GTK_RBNODE_FLAG_SET (node->right, GTK_RBNODE_DESCENDANTS_INVALID))
		{
		  node = node->right;
		}
	      else if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_INVALID) ||
		       GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_COLUMN_INVALID))
		{
		  break;
		}
	      else if (node->children != NULL)
		{
		  tree = node->children;
		  node = tree->root;
		}
	      else
		/* RBTree corruption!  All bad */
		g_assert_not_reached ();
	    }
	  while (TRUE);
	  path = _rotator_find_path (tree_view, tree, node);
	  gtk_tree_model_get_iter (tree_view->priv->model, &iter, path);
	}

      validated_area = validate_row (tree_view, tree, node, &iter, path) ||
                       validated_area;

      if (!tree_view->priv->fixed_height_check)
        {
	  gint height;

	  height = ROW_HEIGHT (tree_view, GTK_RBNODE_GET_HEIGHT (node));
	  if (prev_height < 0)
	    prev_height = height;
	  else if (prev_height != height)
	    fixed_height = FALSE;
	}

      i++;
    }
  while (i < GTK_ROTATOR_NUM_ROWS_PER_IDLE);

  if (!tree_view->priv->fixed_height_check)
   {
     if (fixed_height)
       _gtk_rbtree_set_fixed_height (tree_view->priv->tree, prev_height, FALSE);

     tree_view->priv->fixed_height_check = 1;
   }
  
 done:
  if (validated_area)
    {
      GtkRequisition requisition;
      /* We temporarily guess a size, under the assumption that it will be the
       * same when we get our next size_allocate.  If we don't do this, we'll be
       * in an inconsistent state when we call top_row_to_dy. */

      gtk_widget_size_request (GTK_WIDGET (tree_view), &requisition);
      tree_view->priv->hadjustment->upper = MAX (tree_view->priv->hadjustment->upper, (gfloat)requisition.width);
      tree_view->priv->vadjustment->upper = MAX (tree_view->priv->vadjustment->upper, (gfloat)requisition.height);
      gtk_adjustment_changed (tree_view->priv->hadjustment);
      gtk_adjustment_changed (tree_view->priv->vadjustment);

      if (queue_resize)
        gtk_widget_queue_resize (GTK_WIDGET (tree_view));
    }

  if (path) gtk_tree_path_free (path);

  return retval;
}

static gboolean
validate_rows (Rotator *tree_view)
{
  gboolean retval;
  
  retval = do_validate_rows (tree_view, TRUE);
  
  if (! retval && tree_view->priv->validate_rows_timer)
    {
      g_source_remove (tree_view->priv->validate_rows_timer);
      tree_view->priv->validate_rows_timer = 0;
    }

  return retval;
}

static gboolean
validate_rows_handler (Rotator *tree_view)
{
  gboolean retval;

  retval = do_validate_rows (tree_view, TRUE);
  if (! retval && tree_view->priv->validate_rows_timer)
    {
      g_source_remove (tree_view->priv->validate_rows_timer);
      tree_view->priv->validate_rows_timer = 0;
    }

  return retval;
}

static gboolean
do_presize_handler (Rotator *tree_view)
{
  if (tree_view->priv->mark_rows_col_dirty)
    {
      if (tree_view->priv->tree)
	_gtk_rbtree_column_invalid (tree_view->priv->tree);
      tree_view->priv->mark_rows_col_dirty = FALSE;
    }
  validate_visible_area (tree_view);
  tree_view->priv->presize_handler_timer = 0;

  if (tree_view->priv->fixed_height_mode)
    {
      GtkRequisition requisition;

      gtk_widget_size_request (GTK_WIDGET (tree_view), &requisition);

      tree_view->priv->hadjustment->upper = MAX (tree_view->priv->hadjustment->upper, (gfloat)requisition.width);
      tree_view->priv->vadjustment->upper = MAX (tree_view->priv->vadjustment->upper, (gfloat)requisition.height);
      gtk_adjustment_changed (tree_view->priv->hadjustment);
      gtk_adjustment_changed (tree_view->priv->vadjustment);
      gtk_widget_queue_resize (GTK_WIDGET (tree_view));
    }
		   
  return FALSE;
}

static gboolean
presize_handler_callback (gpointer data)
{
  do_presize_handler (GTK_ROTATOR (data));
		   
  return FALSE;
}

static void
install_presize_handler (Rotator *tree_view)
{
  if (! GTK_WIDGET_REALIZED (tree_view))
    return;

  if (! tree_view->priv->presize_handler_timer)
    {
      tree_view->priv->presize_handler_timer =
	gdk_threads_add_idle_full (GTK_PRIORITY_RESIZE - 2, presize_handler_callback, tree_view, NULL);
    }
  if (! tree_view->priv->validate_rows_timer)
    {
      tree_view->priv->validate_rows_timer =
	gdk_threads_add_idle_full (GTK_ROTATOR_PRIORITY_VALIDATE, (GSourceFunc) validate_rows_handler, tree_view, NULL);
    }
}

static gboolean
scroll_sync_handler (Rotator *tree_view)
{
  if (tree_view->priv->height <= tree_view->priv->vadjustment->page_size)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (tree_view->priv->vadjustment), 0);
  else if (gtk_tree_row_reference_valid (tree_view->priv->top_row))
    rotator_top_row_to_dy (tree_view);

  tree_view->priv->scroll_sync_timer = 0;

  return FALSE;
}

static void
install_scroll_sync_handler (Rotator *tree_view)
{
  if (! GTK_WIDGET_REALIZED (tree_view))
    return;

  if (!tree_view->priv->scroll_sync_timer)
    {
      tree_view->priv->scroll_sync_timer =
	gdk_threads_add_idle_full (GTK_ROTATOR_PRIORITY_SCROLL_SYNC, (GSourceFunc) scroll_sync_handler, tree_view, NULL);
    }
}

static void
rotator_set_top_row (Rotator *tree_view,
			   GtkTreePath *path,
			   gint         offset)
{
  gtk_tree_row_reference_free (tree_view->priv->top_row);

  if (!path)
    {
      tree_view->priv->top_row = NULL;
      tree_view->priv->top_row_dy = 0;
    }
  else
    {
      tree_view->priv->top_row = gtk_tree_row_reference_new_proxy (G_OBJECT (tree_view), tree_view->priv->model, path);
      tree_view->priv->top_row_dy = offset;
    }
}

static void
rotator_top_row_to_dy (Rotator *tree_view)
{
  GtkTreePath *path;
  GtkRBTree *tree;
  GtkRBNode *node;
  int new_dy;

  if (tree_view->priv->top_row)
    path = gtk_tree_row_reference_get_path (tree_view->priv->top_row);
  else
    path = NULL;

  if (!path)
    tree = NULL;
  else
    _rotator_find_node (tree_view, path, &tree, &node);

  if (path)
    gtk_tree_path_free (path);

  if (tree == NULL)
    {
      /* keep dy and set new toprow */
      gtk_tree_row_reference_free (tree_view->priv->top_row);
      tree_view->priv->top_row = NULL;
      tree_view->priv->top_row_dy = 0;
      /* DO NOT install the idle handler */
      return;
    }

  if (ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node))
      < tree_view->priv->top_row_dy)
    {
      /* new top row -- do NOT install the idle handler */
      return;
    }

  new_dy = _gtk_rbtree_node_find_offset (tree, node);
  new_dy += tree_view->priv->top_row_dy;

  if (new_dy + tree_view->priv->vadjustment->page_size > tree_view->priv->height)
    new_dy = tree_view->priv->height - tree_view->priv->vadjustment->page_size;

  new_dy = MAX (0, new_dy);

  tree_view->priv->in_top_row_to_dy = TRUE;
  gtk_adjustment_set_value (tree_view->priv->vadjustment, (gdouble)new_dy);
  tree_view->priv->in_top_row_to_dy = FALSE;
}


void
_rotator_install_mark_rows_col_dirty (Rotator *tree_view)
{
  tree_view->priv->mark_rows_col_dirty = TRUE;

  install_presize_handler (tree_view);
}

/*
 * This function works synchronously (due to the while (validate_rows...)
 * loop).
 *
 * There was a check for column_type != GTK_ROTATOR_COLUMN_AUTOSIZE
 * here. You now need to check that yourself.
 */
void
_rotator_column_autosize (Rotator *tree_view,
			        RotatorColumn *column)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (GTK_IS_ROTATOR_COLUMN (column));

  _rotator_column_cell_set_dirty (column, FALSE);

  do_presize_handler (tree_view);
  while (validate_rows (tree_view));

  gtk_widget_queue_resize (GTK_WIDGET (tree_view));
}

/* Drag-and-drop */

static void
set_source_row (GdkDragContext *context,
                GtkTreeModel   *model,
                GtkTreePath    *source_row)
{
  g_object_set_data_full (G_OBJECT (context),
                          I_("gtk-tree-view-source-row"),
                          source_row ? gtk_tree_row_reference_new (model, source_row) : NULL,
                          (GDestroyNotify) (source_row ? gtk_tree_row_reference_free : NULL));
}

static GtkTreePath*
get_source_row (GdkDragContext *context)
{
  GtkTreeRowReference *ref =
    g_object_get_data (G_OBJECT (context), "gtk-tree-view-source-row");

  if (ref)
    return gtk_tree_row_reference_get_path (ref);
  else
    return NULL;
}

typedef struct
{
  GtkTreeRowReference *dest_row;
  guint                path_down_mode   : 1;
  guint                empty_view_drop  : 1;
  guint                drop_append_mode : 1;
}
DestRow;

static void
dest_row_free (gpointer data)
{
  DestRow *dr = (DestRow *)data;

  gtk_tree_row_reference_free (dr->dest_row);
  g_slice_free (DestRow, dr);
}

static void
set_dest_row (GdkDragContext *context,
              GtkTreeModel   *model,
              GtkTreePath    *dest_row,
              gboolean        path_down_mode,
              gboolean        empty_view_drop,
              gboolean        drop_append_mode)
{
  DestRow *dr;

  if (!dest_row)
    {
      g_object_set_data_full (G_OBJECT (context), I_("gtk-tree-view-dest-row"),
                              NULL, NULL);
      return;
    }

  dr = g_slice_new (DestRow);

  dr->dest_row = gtk_tree_row_reference_new (model, dest_row);
  dr->path_down_mode = path_down_mode != FALSE;
  dr->empty_view_drop = empty_view_drop != FALSE;
  dr->drop_append_mode = drop_append_mode != FALSE;

  g_object_set_data_full (G_OBJECT (context), I_("gtk-tree-view-dest-row"),
                          dr, (GDestroyNotify) dest_row_free);
}

static GtkTreePath*
get_dest_row (GdkDragContext *context,
              gboolean       *path_down_mode)
{
  DestRow *dr =
    g_object_get_data (G_OBJECT (context), "gtk-tree-view-dest-row");

  if (dr)
    {
      GtkTreePath *path = NULL;

      if (path_down_mode)
        *path_down_mode = dr->path_down_mode;

      if (dr->dest_row)
        path = gtk_tree_row_reference_get_path (dr->dest_row);
      else if (dr->empty_view_drop)
        path = gtk_tree_path_new_from_indices (0, -1);
      else
        path = NULL;

      if (path && dr->drop_append_mode)
        gtk_tree_path_next (path);

      return path;
    }
  else
    return NULL;
}

/* Get/set whether drag_motion requested the drag data and
 * drag_data_received should thus not actually insert the data,
 * since the data doesn't result from a drop.
 */
static void
set_status_pending (GdkDragContext *context,
                    GdkDragAction   suggested_action)
{
  g_object_set_data (G_OBJECT (context),
                     I_("gtk-tree-view-status-pending"),
                     GINT_TO_POINTER (suggested_action));
}

static GdkDragAction
get_status_pending (GdkDragContext *context)
{
  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (context),
                                             "gtk-tree-view-status-pending"));
}

static TreeViewDragInfo*
get_info (Rotator *tree_view)
{
  return g_object_get_data (G_OBJECT (tree_view), "gtk-tree-view-drag-info");
}

static void
destroy_info (TreeViewDragInfo *di)
{
  g_slice_free (TreeViewDragInfo, di);
}

static TreeViewDragInfo*
ensure_info (Rotator *tree_view)
{
  TreeViewDragInfo *di;

  di = get_info (tree_view);

  if (di == NULL)
    {
      di = g_slice_new0 (TreeViewDragInfo);

      g_object_set_data_full (G_OBJECT (tree_view),
                              I_("gtk-tree-view-drag-info"),
                              di,
                              (GDestroyNotify) destroy_info);
    }

  return di;
}

static void
remove_info (Rotator *tree_view)
{
  g_object_set_data (G_OBJECT (tree_view), I_("gtk-tree-view-drag-info"), NULL);
}

#if 0
static gint
drag_scan_timeout (gpointer data)
{
  Rotator *tree_view;
  gint x, y;
  GdkModifierType state;
  GtkTreePath *path = NULL;
  RotatorColumn *column = NULL;
  GdkRectangle visible_rect;

  GDK_THREADS_ENTER ();

  tree_view = GTK_ROTATOR (data);

  gdk_window_get_pointer (tree_view->priv->bin_window,
                          &x, &y, &state);

  rotator_get_visible_rect (tree_view, &visible_rect);

  /* See if we are near the edge. */
  if ((x - visible_rect.x) < SCROLL_EDGE_SIZE ||
      (visible_rect.x + visible_rect.width - x) < SCROLL_EDGE_SIZE ||
      (y - visible_rect.y) < SCROLL_EDGE_SIZE ||
      (visible_rect.y + visible_rect.height - y) < SCROLL_EDGE_SIZE)
    {
      rotator_get_path_at_pos (tree_view,
                                     tree_view->priv->bin_window,
                                     x, y,
                                     &path,
                                     &column,
                                     NULL,
                                     NULL);

      if (path != NULL)
        {
          rotator_scroll_to_cell (tree_view,
                                        path,
                                        column,
					TRUE,
                                        0.5, 0.5);

          gtk_tree_path_free (path);
        }
    }

  GDK_THREADS_LEAVE ();

  return TRUE;
}
#endif /* 0 */

static void
add_scroll_timeout (Rotator *tree_view)
{
}

static void
remove_scroll_timeout (Rotator *tree_view)
{
  if (tree_view->priv->scroll_timeout != 0)
    {
      g_source_remove (tree_view->priv->scroll_timeout);
      tree_view->priv->scroll_timeout = 0;
    }
}

static gboolean
check_model_dnd (GtkTreeModel *model,
                 GType         required_iface,
                 const gchar  *signal)
{
  if (model == NULL || !G_TYPE_CHECK_INSTANCE_TYPE ((model), required_iface))
    {
      g_warning ("You must override the default '%s' handler "
                 "on Rotator when using models that don't support "
                 "the %s interface and enabling drag-and-drop. The simplest way to do this "
                 "is to connect to '%s' and call "
                 "g_signal_stop_emission_by_name() in your signal handler to prevent "
                 "the default handler from running. Look at the source code "
                 "for the default handler in gtktreeview.c to get an idea what "
                 "your handler should do. (gtktreeview.c is in the GTK source "
                 "code.) If you're using GTK from a language other than C, "
                 "there may be a more natural way to override default handlers, e.g. via derivation.",
                 signal, g_type_name (required_iface), signal);
      return FALSE;
    }
  else
    return TRUE;
}

static void
remove_open_timeout (Rotator *tree_view)
{
  if (tree_view->priv->open_dest_timeout != 0)
    {
      g_source_remove (tree_view->priv->open_dest_timeout);
      tree_view->priv->open_dest_timeout = 0;
    }
}


static gint
open_row_timeout (gpointer data)
{
  Rotator *tree_view = data;
  GtkTreePath *dest_path = NULL;
  RotatorDropPosition pos;
  gboolean result = FALSE;

  rotator_get_drag_dest_row (tree_view,
                                   &dest_path,
                                   &pos);

  if (dest_path &&
      (pos == GTK_ROTATOR_DROP_INTO_OR_AFTER ||
       pos == GTK_ROTATOR_DROP_INTO_OR_BEFORE))
    {
      rotator_expand_row (tree_view, dest_path, FALSE);
      tree_view->priv->open_dest_timeout = 0;

      gtk_tree_path_free (dest_path);
    }
  else
    {
      if (dest_path)
        gtk_tree_path_free (dest_path);

      result = TRUE;
    }

  return result;
}

/* Returns TRUE if event should not be propagated to parent widgets */
static gboolean
set_destination_row (Rotator* tree_view, GdkDragContext *context, /* coordinates relative to the widget */ gint            x, gint            y, GdkDragAction  *suggested_action, GdkAtom        *target)
{
  GtkTreePath *path = NULL;
  RotatorDropPosition pos;
  RotatorDropPosition old_pos;
  TreeViewDragInfo *di;
  GtkWidget *widget;
  GtkTreePath *old_dest_path = NULL;
  gboolean can_drop = FALSE;

  *suggested_action = 0;
  *target = GDK_NONE;

  widget = GTK_WIDGET (tree_view);

  di = get_info (tree_view);

  if (di == NULL || y - ROTATOR_HEADER_HEIGHT (tree_view) < 0)
    {
      /* someone unset us as a drag dest, note that if
       * we return FALSE drag_leave isn't called
       */

      rotator_set_drag_dest_row (tree_view,
                                       NULL,
                                       GTK_ROTATOR_DROP_BEFORE);

      remove_scroll_timeout (GTK_ROTATOR (widget));
      remove_open_timeout (GTK_ROTATOR (widget));

      return FALSE; /* no longer a drop site */
    }

  *target = gtk_drag_dest_find_target (widget, context,
                                       gtk_drag_dest_get_target_list (widget));
  if (*target == GDK_NONE)
    {
      return FALSE;
    }

  if (!rotator_get_dest_row_at_pos (tree_view,
                                          x, y,
                                          &path,
                                          &pos))
    {
      gint n_children;
      GtkTreeModel *model;

      remove_open_timeout (tree_view);

      /* the row got dropped on empty space, let's setup a special case
       */

      if (path)
	gtk_tree_path_free (path);

      model = rotator_get_model (tree_view);

      n_children = gtk_tree_model_iter_n_children (model, NULL);
      if (n_children)
        {
          pos = GTK_ROTATOR_DROP_AFTER;
          path = gtk_tree_path_new_from_indices (n_children - 1, -1);
        }
      else
        {
          pos = GTK_ROTATOR_DROP_BEFORE;
          path = gtk_tree_path_new_from_indices (0, -1);
        }

      can_drop = TRUE;

      goto out;
    }

  g_assert (path);

  /* If we left the current row's "open" zone, unset the timeout for
   * opening the row
   */
  rotator_get_drag_dest_row (tree_view,
                                   &old_dest_path,
                                   &old_pos);

  if (old_dest_path &&
      (gtk_tree_path_compare (path, old_dest_path) != 0 ||
       !(pos == GTK_ROTATOR_DROP_INTO_OR_AFTER ||
         pos == GTK_ROTATOR_DROP_INTO_OR_BEFORE)))
    remove_open_timeout (tree_view);

  if (old_dest_path)
    gtk_tree_path_free (old_dest_path);

  if (TRUE /* FIXME if the location droppable predicate */)
    {
      can_drop = TRUE;
    }

out:
  if (can_drop)
    {
      GtkWidget *source_widget;

      *suggested_action = context->suggested_action;
      source_widget = gtk_drag_get_source_widget (context);

      if (source_widget == widget)
        {
          /* Default to MOVE, unless the user has
           * pressed ctrl or shift to affect available actions
           */
          if ((context->actions & GDK_ACTION_MOVE) != 0)
            *suggested_action = GDK_ACTION_MOVE;
        }

      rotator_set_drag_dest_row (GTK_ROTATOR (widget),
                                       path, pos);
    }
  else
    {
      /* can't drop here */
      remove_open_timeout (tree_view);

      rotator_set_drag_dest_row (GTK_ROTATOR (widget),
                                       NULL,
                                       GTK_ROTATOR_DROP_BEFORE);
    }

  if (path)
    gtk_tree_path_free (path);

  return TRUE;
}

static GtkTreePath*
get_logical_dest_row (Rotator *tree_view,
                      gboolean    *path_down_mode,
                      gboolean    *drop_append_mode)
{
  /* adjust path to point to the row the drop goes in front of */
  GtkTreePath *path = NULL;
  RotatorDropPosition pos;

  g_return_val_if_fail (path_down_mode != NULL, NULL);
  g_return_val_if_fail (drop_append_mode != NULL, NULL);

  *path_down_mode = FALSE;
  *drop_append_mode = 0;

  rotator_get_drag_dest_row (tree_view, &path, &pos);

  if (path == NULL)
    return NULL;

  if (pos == GTK_ROTATOR_DROP_BEFORE)
    ; /* do nothing */
  else if (pos == GTK_ROTATOR_DROP_INTO_OR_BEFORE ||
           pos == GTK_ROTATOR_DROP_INTO_OR_AFTER)
    *path_down_mode = TRUE;
  else
    {
      GtkTreeIter iter;
      GtkTreeModel *model = rotator_get_model (tree_view);

      g_assert (pos == GTK_ROTATOR_DROP_AFTER);

      if (!gtk_tree_model_get_iter (model, &iter, path) ||
          !gtk_tree_model_iter_next (model, &iter))
        *drop_append_mode = 1;
      else
        {
          *drop_append_mode = 0;
          gtk_tree_path_next (path);
        }
    }

  return path;
}

static gboolean
rotator_maybe_begin_dragging_row (Rotator* tree_view, GdkEventMotion* event)
{
  GtkWidget *widget = GTK_WIDGET (tree_view);
  GdkDragContext *context;
  TreeViewDragInfo *di;
  GtkTreePath *path = NULL;
  gint button;
  gint cell_x, cell_y;
  GtkTreeModel *model;
  gboolean retval = FALSE;

  di = get_info (tree_view);

  if (di == NULL || !di->source_set)
    goto out;

  if (tree_view->priv->pressed_button < 0)
    goto out;

  if (!gtk_drag_check_threshold (widget,
                                 tree_view->priv->press_start_x,
                                 tree_view->priv->press_start_y,
                                 event->x, event->y))
    goto out;

  model = rotator_get_model (tree_view);

  if (model == NULL)
    goto out;

  button = tree_view->priv->pressed_button;
  tree_view->priv->pressed_button = -1;

  rotator_get_path_at_pos (tree_view,
                                 tree_view->priv->press_start_x,
                                 tree_view->priv->press_start_y,
                                 &path,
                                 NULL,
                                 &cell_x,
                                 &cell_y);

  if (path == NULL)
    goto out;

  if (!GTK_IS_TREE_DRAG_SOURCE (model) ||
      !gtk_tree_drag_source_row_draggable (GTK_TREE_DRAG_SOURCE (model),
					   path))
    goto out;

  if (!(GDK_BUTTON1_MASK << (button - 1) & di->start_button_mask))
    goto out;

  /* Now we can begin the drag */

  retval = TRUE;

  context = gtk_drag_begin (widget,
                            gtk_drag_source_get_target_list (widget),
                            di->source_actions,
                            button,
                            (GdkEvent*)event);

  set_source_row (context, model, path);

 out:
  if (path)
    gtk_tree_path_free (path);

  return retval;
}


static void
rotator_drag_begin (GtkWidget      *widget,
                          GdkDragContext *context)
{
  Rotator *tree_view;
  GtkTreePath *path = NULL;
  gint cell_x, cell_y;
  GdkPixmap *row_pix;
  TreeViewDragInfo *di;

  tree_view = GTK_ROTATOR (widget);

  /* if the user uses a custom DND source impl, we don't set the icon here */
  di = get_info (tree_view);

  if (di == NULL || !di->source_set)
    return;

  rotator_get_path_at_pos (tree_view,
                                 tree_view->priv->press_start_x,
                                 tree_view->priv->press_start_y,
                                 &path,
                                 NULL,
                                 &cell_x,
                                 &cell_y);

  g_return_if_fail (path != NULL);

  row_pix = rotator_create_row_drag_icon (tree_view,
                                                path);

  gtk_drag_set_icon_pixmap (context,
                            gdk_drawable_get_colormap (row_pix),
                            row_pix,
                            NULL,
                            /* the + 1 is for the black border in the icon */
                            tree_view->priv->press_start_x + 1,
                            cell_y + 1);

  g_object_unref (row_pix);
  gtk_tree_path_free (path);
}

static void
rotator_drag_end (GtkWidget      *widget,
                        GdkDragContext *context)
{
  /* do nothing */
}

/* Default signal implementations for the drag signals */
static void
rotator_drag_data_get (GtkWidget        *widget,
                             GdkDragContext   *context,
                             GtkSelectionData *selection_data,
                             guint             info,
                             guint             time)
{
  Rotator *tree_view;
  GtkTreeModel *model;
  TreeViewDragInfo *di;
  GtkTreePath *source_row;

  tree_view = GTK_ROTATOR (widget);

  model = rotator_get_model (tree_view);

  if (model == NULL)
    return;

  di = get_info (GTK_ROTATOR (widget));

  if (di == NULL)
    return;

  source_row = get_source_row (context);

  if (source_row == NULL)
    return;

  /* We can implement the GTK_TREE_MODEL_ROW target generically for
   * any model; for DragSource models there are some other targets
   * we also support.
   */

  if (GTK_IS_TREE_DRAG_SOURCE (model) &&
      gtk_tree_drag_source_drag_data_get (GTK_TREE_DRAG_SOURCE (model),
                                          source_row,
                                          selection_data))
    goto done;

  /* If drag_data_get does nothing, try providing row data. */
  if (selection_data->target == gdk_atom_intern_static_string ("GTK_TREE_MODEL_ROW"))
    {
      gtk_tree_set_row_drag_data (selection_data,
				  model,
				  source_row);
    }

 done:
  gtk_tree_path_free (source_row);
}


static void
rotator_drag_data_delete (GtkWidget      *widget,
                                GdkDragContext *context)
{
  TreeViewDragInfo *di;
  GtkTreeModel *model;
  Rotator *tree_view;
  GtkTreePath *source_row;

  tree_view = GTK_ROTATOR (widget);
  model = rotator_get_model (tree_view);

  if (!check_model_dnd (model, GTK_TYPE_TREE_DRAG_SOURCE, "drag_data_delete"))
    return;

  di = get_info (tree_view);

  if (di == NULL)
    return;

  source_row = get_source_row (context);

  if (source_row == NULL)
    return;

  gtk_tree_drag_source_drag_data_delete (GTK_TREE_DRAG_SOURCE (model),
                                         source_row);

  gtk_tree_path_free (source_row);

  set_source_row (context, NULL, NULL);
}

static void
rotator_drag_leave (GtkWidget      *widget,
                          GdkDragContext *context,
                          guint             time)
{
  /* unset any highlight row */
  rotator_set_drag_dest_row (GTK_ROTATOR (widget),
                                   NULL,
                                   GTK_ROTATOR_DROP_BEFORE);

  remove_scroll_timeout (GTK_ROTATOR (widget));
  remove_open_timeout (GTK_ROTATOR (widget));
}


static gboolean
rotator_drag_motion (GtkWidget* widget, GdkDragContext* context, /* coordinates relative to the widget */ gint x, gint y, guint time)
{
  gboolean empty;
  GtkTreePath *path = NULL;
  RotatorDropPosition pos;
  Rotator *tree_view;
  GdkDragAction suggested_action = 0;
  GdkAtom target;

  tree_view = GTK_ROTATOR (widget);

  if (!set_destination_row (tree_view, context, x, y, &suggested_action, &target))
    return FALSE;

  rotator_get_drag_dest_row (tree_view, &path, &pos);

  /* we only know this *after* set_desination_row */
  empty = tree_view->priv->empty_view_drop;

  if (path == NULL && !empty)
    {
      /* Can't drop here. */
      gdk_drag_status (context, 0, time);
    }

      if (target == gdk_atom_intern_static_string ("GTK_TREE_MODEL_ROW"))
        {
          /* Request data so we can use the source row when
           * determining whether to accept the drop
           */
          set_status_pending (context, suggested_action);
          gtk_drag_get_data (widget, context, target, time);
        }
      else
        {
          set_status_pending (context, 0);
          gdk_drag_status (context, suggested_action, time);
        }
    }

  if (path)
    gtk_tree_path_free (path);

  return TRUE;
}


static gboolean
rotator_drag_drop (GtkWidget        *widget,
                         GdkDragContext   *context,
			 /* coordinates relative to the widget */
                         gint              x,
                         gint              y,
                         guint             time)
{
  Rotator *tree_view;
  GtkTreePath *path;
  GdkDragAction suggested_action = 0;
  GdkAtom target = GDK_NONE;
  TreeViewDragInfo *di;
  GtkTreeModel *model;
  gboolean path_down_mode;
  gboolean drop_append_mode;

  tree_view = GTK_ROTATOR (widget);

  model = rotator_get_model (tree_view);

  remove_scroll_timeout (GTK_ROTATOR (widget));
  remove_open_timeout (GTK_ROTATOR (widget));

  di = get_info (tree_view);

  if (di == NULL)
    return FALSE;

  if (!check_model_dnd (model, GTK_TYPE_TREE_DRAG_DEST, "drag_drop"))
    return FALSE;

  if (!set_destination_row (tree_view, context, x, y, &suggested_action, &target))
    return FALSE;

  path = get_logical_dest_row (tree_view, &path_down_mode, &drop_append_mode);

  if (target != GDK_NONE && path != NULL)
    {
      /* in case a motion had requested drag data, change things so we
       * treat drag data receives as a drop.
       */
      set_status_pending (context, 0);
      set_dest_row (context, model, path,
                    path_down_mode, tree_view->priv->empty_view_drop,
                    drop_append_mode);
    }

  if (path)
    gtk_tree_path_free (path);

  /* Unset this thing */
  rotator_set_drag_dest_row (GTK_ROTATOR (widget),
                                   NULL,
                                   GTK_ROTATOR_DROP_BEFORE);

  if (target != GDK_NONE)
    {
      gtk_drag_get_data (widget, context, target, time);
      return TRUE;
    }
  else
    return FALSE;
}

static void
rotator_drag_data_received (GtkWidget        *widget,
                                  GdkDragContext   *context,
				  /* coordinates relative to the widget */
                                  gint              x,
                                  gint              y,
                                  GtkSelectionData *selection_data,
                                  guint             info,
                                  guint             time)
{
  GtkTreePath *path;
  TreeViewDragInfo *di;
  gboolean accepted = FALSE;
  GtkTreeModel *model;
  Rotator *tree_view;
  GtkTreePath *dest_row;
  GdkDragAction suggested_action;
  gboolean path_down_mode;
  gboolean drop_append_mode;

  tree_view = GTK_ROTATOR (widget);

  model = rotator_get_model (tree_view);

  if (!check_model_dnd (model, GTK_TYPE_TREE_DRAG_DEST, "drag_data_received"))
    return;

  di = get_info (tree_view);

  if (di == NULL)
    return;

  suggested_action = get_status_pending (context);

  if (suggested_action)
    {
      /* We are getting this data due to a request in drag_motion,
       * rather than due to a request in drag_drop, so we are just
       * supposed to call drag_status, not actually paste in the
       * data.
       */
      path = get_logical_dest_row (tree_view, &path_down_mode,
                                   &drop_append_mode);

      if (path == NULL)
        suggested_action = 0;
      else if (path_down_mode)
        gtk_tree_path_down (path);

      if (suggested_action)
        {
	  if (!gtk_tree_drag_dest_row_drop_possible (GTK_TREE_DRAG_DEST (model),
						     path,
						     selection_data))
            {
              if (path_down_mode)
                {
                  path_down_mode = FALSE;
                  gtk_tree_path_up (path);

                  if (!gtk_tree_drag_dest_row_drop_possible (GTK_TREE_DRAG_DEST (model),
                                                             path,
                                                             selection_data))
                    suggested_action = 0;
                }
              else
	        suggested_action = 0;
            }
        }

      gdk_drag_status (context, suggested_action, time);

      if (path)
        gtk_tree_path_free (path);

      /* If you can't drop, remove user drop indicator until the next motion */
      if (suggested_action == 0)
        rotator_set_drag_dest_row (GTK_ROTATOR (widget),
                                         NULL,
                                         GTK_ROTATOR_DROP_BEFORE);

      return;
    }

  dest_row = get_dest_row (context, &path_down_mode);

  if (dest_row == NULL)
    return;

  if (selection_data->length >= 0)
    {
      if (path_down_mode)
        {
          gtk_tree_path_down (dest_row);
          if (!gtk_tree_drag_dest_row_drop_possible (GTK_TREE_DRAG_DEST (model),
                                                     dest_row, selection_data))
            gtk_tree_path_up (dest_row);
        }
    }

  if (selection_data->length >= 0)
    {
      if (gtk_tree_drag_dest_drag_data_received (GTK_TREE_DRAG_DEST (model),
                                                 dest_row,
                                                 selection_data))
        accepted = TRUE;
    }

  gtk_drag_finish (context,
                   accepted,
                   (context->action == GDK_ACTION_MOVE),
                   time);

  if (gtk_tree_path_get_depth (dest_row) == 1
      && gtk_tree_path_get_indices (dest_row)[0] == 0)
    {
      /* special special case drag to "0", scroll to first item */
      if (!tree_view->priv->scroll_to_path)
        rotator_scroll_to_cell (tree_view, dest_row, NULL, FALSE, 0.0, 0.0);
    }

  gtk_tree_path_free (dest_row);

  /* drop dest_row */
  set_dest_row (context, NULL, NULL, FALSE, FALSE, FALSE);
}



/* GtkContainer Methods
 */


static void
rotator_remove (GtkContainer *container,
		      GtkWidget    *widget)
{
  Rotator *tree_view = GTK_ROTATOR (container);
  RotatorChild *child = NULL;
  GList *tmp_list;

  tmp_list = tree_view->priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      if (child->widget == widget)
	{
	  gtk_widget_unparent (widget);

	  tree_view->priv->children = g_list_remove_link (tree_view->priv->children, tmp_list);
	  g_list_free_1 (tmp_list);
	  g_slice_free (RotatorChild, child);
	  return;
	}

      tmp_list = tmp_list->next;
    }

  tmp_list = tree_view->priv->columns;

  while (tmp_list)
    {
      RotatorColumn *column;
      column = tmp_list->data;
      if (column->button == widget)
	{
	  gtk_widget_unparent (widget);
	  return;
	}
      tmp_list = tmp_list->next;
    }
}

static void
rotator_forall (GtkContainer *container,
		      gboolean      include_internals,
		      GtkCallback   callback,
		      gpointer      callback_data)
{
  Rotator *tree_view = GTK_ROTATOR (container);
  RotatorChild *child = NULL;
  RotatorColumn *column;
  GList *tmp_list;

  tmp_list = tree_view->priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      tmp_list = tmp_list->next;

      (* callback) (child->widget, callback_data);
    }
  if (include_internals == FALSE)
    return;

  for (tmp_list = tree_view->priv->columns; tmp_list; tmp_list = tmp_list->next)
    {
      column = tmp_list->data;

      if (column->button)
	(* callback) (column->button, callback_data);
    }
}

/* Returns TRUE if the treeview contains no "special" (editable or activatable)
 * cells. If so we draw one big row-spanning focus rectangle.
 */
static gboolean
rotator_has_special_cell (Rotator *tree_view)
{
  GList *list;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      if (!((RotatorColumn *)list->data)->visible)
	continue;
      if (_rotator_column_count_special_cells (list->data))
	return TRUE;
    }

  return FALSE;
}

static void
column_sizing_notify (GObject    *object,
                      GParamSpec *pspec,
                      gpointer    data)
{
  RotatorColumn *c = GTK_ROTATOR_COLUMN (object);

  if (rotator_column_get_sizing (c) != GTK_ROTATOR_COLUMN_FIXED)
    /* disable fixed height mode */
    g_object_set (data, "fixed-height-mode", FALSE, NULL);
}

/**
 * rotator_set_fixed_height_mode:
 * @tree_view: a #Rotator 
 * @enable: %TRUE to enable fixed height mode
 * 
 * Enables or disables the fixed height mode of @tree_view. 
 * Fixed height mode speeds up #Rotator by assuming that all 
 * rows have the same height. 
 * Only enable this option if all rows are the same height and all
 * columns are of type %GTK_ROTATOR_COLUMN_FIXED.
 *
 * Since: 2.6 
 **/
void
rotator_set_fixed_height_mode (Rotator *tree_view,
                                     gboolean     enable)
{
  GList *l;
  
  enable = enable != FALSE;

  if (enable == tree_view->priv->fixed_height_mode)
    return;

  if (!enable)
    {
      tree_view->priv->fixed_height_mode = 0;
      tree_view->priv->fixed_height = -1;

      /* force a revalidation */
      install_presize_handler (tree_view);
    }
  else 
    {
      /* make sure all columns are of type FIXED */
      for (l = tree_view->priv->columns; l; l = l->next)
	{
	  RotatorColumn *c = l->data;
	  
	  g_return_if_fail (rotator_column_get_sizing (c) == GTK_ROTATOR_COLUMN_FIXED);
	}
      
      /* yes, we really have to do this is in a separate loop */
      for (l = tree_view->priv->columns; l; l = l->next)
	g_signal_connect (l->data, "notify::sizing",
			  G_CALLBACK (column_sizing_notify), tree_view);
      
      tree_view->priv->fixed_height_mode = 1;
      tree_view->priv->fixed_height = -1;
      
      if (tree_view->priv->tree)
	initialize_fixed_height_mode (tree_view);
    }

  g_object_notify (G_OBJECT (tree_view), "fixed-height-mode");
}

/**
 * rotator_get_fixed_height_mode:
 * @tree_view: a #Rotator
 * 
 * Returns whether fixed height mode is turned on for @tree_view.
 * 
 * Return value: %TRUE if @tree_view is in fixed height mode
 * 
 * Since: 2.6
 **/
gboolean
rotator_get_fixed_height_mode (Rotator *tree_view)
{
  return tree_view->priv->fixed_height_mode;
}

/* This function returns in 'path' the first focusable path, if the given path
 * is already focusable, it's the returned one.
 */
static gboolean
search_first_focusable_path (Rotator  *tree_view,
			     GtkTreePath **path,
			     gboolean      search_forward,
			     GtkRBTree   **new_tree,
			     GtkRBNode   **new_node)
{
  GtkRBTree *tree = NULL;
  GtkRBNode *node = NULL;

  if (!path || !*path)
    return FALSE;

  _rotator_find_node (tree_view, *path, &tree, &node);

  if (!tree || !node)
    return FALSE;

  while (node && row_is_separator (tree_view, NULL, *path))
    {
      if (search_forward)
	_gtk_rbtree_next_full (tree, node, &tree, &node);
      else
	_gtk_rbtree_prev_full (tree, node, &tree, &node);

      if (*path)
	gtk_tree_path_free (*path);

      if (node)
	*path = _rotator_find_path (tree_view, tree, node);
      else
	*path = NULL;
    }

  if (new_tree)
    *new_tree = tree;

  if (new_node)
    *new_node = node;

  return (*path != NULL);
}

static gint
rotator_focus (GtkWidget        *widget, GtkDirectionType  direction)
{
  Rotator *tree_view = GTK_ROTATOR (widget);
  GtkContainer *container = GTK_CONTAINER (widget);
  GtkWidget *focus_child;

  if (!GTK_WIDGET_IS_SENSITIVE (container) || !GTK_WIDGET_CAN_FOCUS (widget))
    return FALSE;

  focus_child = container->focus_child;

  rotator_stop_editing (GTK_ROTATOR (widget), FALSE);
  /* Case 1.  Headers currently have focus. */
  if (focus_child)
    {
      switch (direction)
	{
	case GTK_DIR_LEFT:
	case GTK_DIR_RIGHT:
	  rotator_header_focus (tree_view, direction, TRUE);
	  return TRUE;
	case GTK_DIR_TAB_BACKWARD:
	case GTK_DIR_UP:
	  return FALSE;
	case GTK_DIR_TAB_FORWARD:
	case GTK_DIR_DOWN:
	  gtk_widget_grab_focus (widget);
	  return TRUE;
	default:
	  g_assert_not_reached ();
	  return FALSE;
	}
    }

  /* Case 2. We don't have focus at all. */
  if (!GTK_WIDGET_HAS_FOCUS (container))
    {
      if (!rotator_header_focus (tree_view, direction, FALSE))
	gtk_widget_grab_focus (widget);
      return TRUE;
    }

  /* Case 3. We have focus already. */
  if (direction == GTK_DIR_TAB_BACKWARD)
    return (rotator_header_focus (tree_view, direction, FALSE));
  else if (direction == GTK_DIR_TAB_FORWARD)
    return FALSE;

  /* Other directions caught by the keybindings */
  gtk_widget_grab_focus (widget);
  return TRUE;
}

static void
rotator_grab_focus (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (rotator_parent_class)->grab_focus (widget);

  rotator_focus_to_cursor (GTK_ROTATOR (widget));
}

static void
rotator_style_set (GtkWidget *widget,
			 GtkStyle *previous_style)
{
  Rotator *tree_view = GTK_ROTATOR (widget);
  GList *list;
  RotatorColumn *column;

  gtk_widget_style_get (widget,
			"expander-size", &tree_view->priv->expander_size,
			NULL);
  tree_view->priv->expander_size += EXPANDER_EXTRA_PADDING;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      column = list->data;
      _rotator_column_cell_set_dirty (column, TRUE);
    }

  tree_view->priv->fixed_height = -1;
  _gtk_rbtree_mark_invalid (tree_view->priv->tree);

  gtk_widget_queue_resize (widget);
}


static void
rotator_set_focus_child (GtkContainer *container,
			       GtkWidget    *child)
{
  Rotator *tree_view = GTK_ROTATOR (container);
  GList *list;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      if (GTK_ROTATOR_COLUMN (list->data)->button == child)
	{
	  tree_view->priv->focus_column = GTK_ROTATOR_COLUMN (list->data);
	  break;
	}
    }

  GTK_CONTAINER_CLASS (rotator_parent_class)->set_focus_child (container, child);
}

static void
rotator_set_adjustments (Rotator   *tree_view,
			       GtkAdjustment *hadj,
			       GtkAdjustment *vadj)
{
  gboolean need_adjust = FALSE;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (hadj)
    g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));
  else
    hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  if (vadj)
    g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));
  else
    vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (tree_view->priv->hadjustment && (tree_view->priv->hadjustment != hadj))
    {
      g_signal_handlers_disconnect_by_func (tree_view->priv->hadjustment,
					    rotator_adjustment_changed,
					    tree_view);
      g_object_unref (tree_view->priv->hadjustment);
    }

  if (tree_view->priv->vadjustment && (tree_view->priv->vadjustment != vadj))
    {
      g_signal_handlers_disconnect_by_func (tree_view->priv->vadjustment,
					    rotator_adjustment_changed,
					    tree_view);
      g_object_unref (tree_view->priv->vadjustment);
    }

  if (tree_view->priv->hadjustment != hadj)
    {
      tree_view->priv->hadjustment = hadj;
      g_object_ref_sink (tree_view->priv->hadjustment);

      g_signal_connect (tree_view->priv->hadjustment, "value-changed",
			G_CALLBACK (rotator_adjustment_changed),
			tree_view);
      need_adjust = TRUE;
    }

  if (tree_view->priv->vadjustment != vadj)
    {
      tree_view->priv->vadjustment = vadj;
      g_object_ref_sink (tree_view->priv->vadjustment);

      g_signal_connect (tree_view->priv->vadjustment, "value-changed",
			G_CALLBACK (rotator_adjustment_changed),
			tree_view);
      need_adjust = TRUE;
    }

  if (need_adjust)
    rotator_adjustment_changed (NULL, tree_view);
}
#endif


static gboolean
rotator_real_move_cursor (Rotator* tree_view, GtkMovementStep step, gint count)
{
	PF0;

#if 0
  GdkModifierType state;

  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);
  g_return_val_if_fail (step == GTK_MOVEMENT_LOGICAL_POSITIONS ||
			step == GTK_MOVEMENT_VISUAL_POSITIONS ||
			step == GTK_MOVEMENT_DISPLAY_LINES ||
			step == GTK_MOVEMENT_PAGES ||
			step == GTK_MOVEMENT_BUFFER_ENDS, FALSE);

  if (tree_view->priv->tree == NULL)
    return FALSE;
  if (!GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (tree_view)))
    return FALSE;

  rotator_stop_editing (tree_view, FALSE);
  GTK_ROTATOR_SET_FLAG (tree_view, GTK_ROTATOR_DRAW_KEYFOCUS);
  gtk_widget_grab_focus (GTK_WIDGET (tree_view));

  if (gtk_get_current_event_state (&state))
    {
      if ((state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
        tree_view->priv->ctrl_pressed = TRUE;
      if ((state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK)
        tree_view->priv->shift_pressed = TRUE;
    }
  /* else we assume not pressed */

  switch (step)
    {
      /* currently we make no distinction.  When we go bi-di, we need to */
    case GTK_MOVEMENT_LOGICAL_POSITIONS:
    case GTK_MOVEMENT_VISUAL_POSITIONS:
      rotator_move_cursor_left_right (tree_view, count);
      break;
    case GTK_MOVEMENT_DISPLAY_LINES:
      rotator_move_cursor_up_down (tree_view, count);
      break;
    case GTK_MOVEMENT_PAGES:
      rotator_move_cursor_page_up_down (tree_view, count);
      break;
    case GTK_MOVEMENT_BUFFER_ENDS:
      rotator_move_cursor_start_end (tree_view, count);
      break;
    default:
      g_assert_not_reached ();
    }

  tree_view->priv->ctrl_pressed = FALSE;
  tree_view->priv->shift_pressed = FALSE;
#endif

  return TRUE;
}


#if 0
static void
rotator_put (Rotator *tree_view,
		   GtkWidget   *child_widget,
		   /* in bin_window coordinates */
		   gint         x,
		   gint         y,
		   gint         width,
		   gint         height)
{
  RotatorChild *child;
  
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (GTK_IS_WIDGET (child_widget));

  child = g_slice_new (RotatorChild);

  child->widget = child_widget;
  child->x = x;
  child->y = y;
  child->width = width;
  child->height = height;

  tree_view->priv->children = g_list_append (tree_view->priv->children, child);

  if (GTK_WIDGET_REALIZED (tree_view))
    gtk_widget_set_parent_window (child->widget, tree_view->priv->bin_window);
  
  gtk_widget_set_parent (child_widget, GTK_WIDGET (tree_view));
}

void
_rotator_child_move_resize (Rotator *tree_view,
				  GtkWidget   *widget,
				  /* in tree coordinates */
				  gint         x,
				  gint         y,
				  gint         width,
				  gint         height)
{
  RotatorChild *child = NULL;
  GList *list;
  GdkRectangle allocation;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  for (list = tree_view->priv->children; list; list = list->next)
    {
      if (((RotatorChild *)list->data)->widget == widget)
	{
	  child = list->data;
	  break;
	}
    }
  if (child == NULL)
    return;

  allocation.x = child->x = x;
  allocation.y = child->y = y;
  allocation.width = child->width = width;
  allocation.height = child->height = height;

  if (GTK_WIDGET_REALIZED (widget))
    gtk_widget_size_allocate (widget, &allocation);
}
#endif


/* TreeModel Callbacks
 */

static void
rotator_row_changed (GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer data)
{
	Rotator* tree_view = (Rotator*)data;
	RotatorPrivate* _view = tree_view->priv;
	g_return_if_fail (path || iter);

	gboolean free_path = false;
	if(!path){
		path = gtk_tree_model_get_path(model, iter);
		free_path = true;
	}
	else if (!iter) gtk_tree_model_get_iter(model, iter, path);

	gint* i = gtk_tree_path_get_indices(path);
	g_return_if_fail(i);
	//dbg(0, "i=%i", i[0]);

	if(g_list_length(_view->pending_rows) > 1) dbg(0, "list unexpectedly long");
	GList* l = _view->pending_rows;
	for(;l;l=l->next){
		GtkTreeRowReference* row = l->data;
    	GtkTreePath* row_path = gtk_tree_row_reference_get_path(row);
		gint* i0 = gtk_tree_path_get_indices(row_path);
		if(i0 && i0[0] == i[0]){
			_view->pending_rows = g_list_remove(_view->pending_rows, row);
			gtk_tree_row_reference_free(row);

			_add_by_iter(tree_view, iter);

			break;
		}
    	gtk_tree_path_free(row_path);
	}

	gtk_widget_queue_draw((GtkWidget*)tree_view);

	if (free_path) gtk_tree_path_free(path);
}


static void
rotator_row_inserted (GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer data)
{
	PF0;
	Rotator* tree_view = (Rotator*)data;
	RotatorPrivate* _r = tree_view->priv;
	gboolean free_path = false;

	g_return_if_fail (path || iter);

	if(!_r->wfc) return; //cannot add until the canvas is fully created.

	if (!path) {
		path = gtk_tree_model_get_path (model, iter);
		free_path = true;
	}
	else if (!iter)
		gtk_tree_model_get_iter (model, iter, path);

	//gint* indices = gtk_tree_path_get_indices (path);

	//this is an optional performance thing to let the model know the item is being displayed.
	gtk_tree_model_ref_node (tree_view->priv->model, iter);

	GtkTreeRowReference* row_ref = gtk_tree_row_reference_new(model, path);
	tree_view->priv->pending_rows = g_list_prepend(tree_view->priv->pending_rows, row_ref);

	gtk_widget_queue_draw((GtkWidget*)tree_view);

	if (free_path) gtk_tree_path_free (path);
}


#if 0
static void
rotator_row_has_child_toggled (GtkTreeModel *model, GtkTreePath  *path, GtkTreeIter  *iter, gpointer      data)
{
  Rotator *tree_view = (Rotator *)data;
  GtkTreeIter real_iter;
  gboolean has_child;
  GtkRBTree *tree;
  GtkRBNode *node;
  gboolean free_path = FALSE;

  g_return_if_fail (path != NULL || iter != NULL);

  if (iter)
    real_iter = *iter;

  if (path == NULL)
    {
      path = gtk_tree_model_get_path (model, iter);
      free_path = TRUE;
    }
  else if (iter == NULL)
    gtk_tree_model_get_iter (model, &real_iter, path);

  if (_rotator_find_node (tree_view,
				path,
				&tree,
				&node))
    /* We aren't actually showing the node */
    goto done;

  if (tree == NULL)
    goto done;

  has_child = gtk_tree_model_iter_has_child (model, &real_iter);
  /* Sanity check.
   */
  if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_PARENT) == has_child)
    goto done;

  if (has_child)
    GTK_RBNODE_SET_FLAG (node, GTK_RBNODE_IS_PARENT);
  else
    GTK_RBNODE_UNSET_FLAG (node, GTK_RBNODE_IS_PARENT);

  if (has_child && GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_IS_LIST))
    {
      GTK_ROTATOR_UNSET_FLAG (tree_view, GTK_ROTATOR_IS_LIST);
      if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_SHOW_EXPANDERS))
	{
	  GList *list;

	  for (list = tree_view->priv->columns; list; list = list->next)
	    if (GTK_ROTATOR_COLUMN (list->data)->visible)
	      {
		GTK_ROTATOR_COLUMN (list->data)->dirty = TRUE;
		_rotator_column_cell_set_dirty (GTK_ROTATOR_COLUMN (list->data), TRUE);
		break;
	      }
	}
      gtk_widget_queue_resize (GTK_WIDGET (tree_view));
    }
  else
    {
      _rotator_queue_draw_node (tree_view, tree, node, NULL);
    }

 done:
  if (free_path)
    gtk_tree_path_free (path);
}

static void
count_children_helper (GtkRBTree *tree, GtkRBNode *node, gpointer   data)
{
  if (node->children)
    _gtk_rbtree_traverse (node->children, node->children->root, G_POST_ORDER, count_children_helper, data);
  (*((gint *)data))++;
}

static void
check_selection_helper (GtkRBTree *tree, GtkRBNode *node, gpointer   data)
{
  gint *value = (gint *)data;

  *value = GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_SELECTED);

  if (node->children && !*value)
    _gtk_rbtree_traverse (node->children, node->children->root, G_POST_ORDER, check_selection_helper, data);
}
#endif


static void
rotator_row_deleted (GtkTreeModel* model, GtkTreePath* path, gpointer data)
{
	// path is not guaranteed to be valid, so we have no reliable way to know which row has been deleted,
	// though in many cases it is valid.

	Rotator* tree_view = (Rotator*)data;
	RotatorPrivate* _r = tree_view->priv;

	g_return_if_fail (path);

	/*
	gtk_tree_row_reference_deleted (G_OBJECT (data), path);

	if (! gtk_tree_row_reference_valid (tree_view->priv->top_row)) {
      gtk_tree_row_reference_free (tree_view->priv->top_row);
      tree_view->priv->top_row = NULL;
    }

	gint selection_changed = FALSE;
	if (selection_changed) g_signal_emit_by_name (tree_view->priv->selection, "changed");
	*/

	Sample* sample = samplecat_list_store_get_sample_by_path(path);
	if(sample){
		dbg(0, "sample=%s", sample->name);

		GList* l = _r->actors;
		for(;l;l=l->next){
			SampleActor* sa = l->data;
			dbg(0, "  %s -- %s", sample->name, sa->actor->waveform->filename);
			if(sa->sample == sample){
				dbg(0, "   found!");
				_r->actors = g_list_remove(_r->actors, sa);
				agl_actor__remove_child((AGlActor*)_r->scene, (AGlActor*)sa->actor);
				g_free(sa);
				break;
			}
		}

		sample_unref(sample);
	}else{
		//the path was invalid - reload the whole list!
		GList* l = _r->actors;
		for(;l;l=l->next){
			SampleActor* sa = l->data;
			agl_actor__remove_child((AGlActor*)_r->scene, (AGlActor*)sa->actor);
			g_free(sa);
		}
		g_list_free0(_r->actors);

		_add_all(tree_view);
	}
}


#if 0
static void
rotator_rows_reordered (GtkTreeModel *model,
			      GtkTreePath  *parent,
			      GtkTreeIter  *iter,
			      gint         *new_order,
			      gpointer      data)
{
  Rotator *tree_view = GTK_ROTATOR (data);
  GtkRBTree *tree;
  GtkRBNode *node;
  gint len;

  len = gtk_tree_model_iter_n_children (model, iter);

  if (len < 2)
    return;

  gtk_tree_row_reference_reordered (G_OBJECT (data),
				    parent,
				    iter,
				    new_order);

  if (_rotator_find_node (tree_view,
				parent,
				&tree,
				&node))
    return;

  /* We need to special case the parent path */
  if (tree == NULL)
    tree = tree_view->priv->tree;
  else
    tree = node->children;

  if (tree == NULL)
    return;

  if (tree_view->priv->edited_column)
    rotator_stop_editing (tree_view, TRUE);

  /* we need to be unprelighted */
  ensure_unprelighted (tree_view);

  /* clear the timeout */
  cancel_arrow_animation (tree_view);
  
  _gtk_rbtree_reorder (tree, new_order, len);

  gtk_widget_queue_draw (GTK_WIDGET (tree_view));

}


/* Internal tree functions
 */


static void
rotator_get_background_xrange (Rotator       *tree_view,
                                     GtkRBTree         *tree,
                                     RotatorColumn *column,
                                     gint              *x1,
                                     gint              *x2)
{
  RotatorColumn *tmp_column = NULL;
  gint total_width;
  GList *list;
  gboolean rtl;

  if (x1)
    *x1 = 0;

  if (x2)
    *x2 = 0;

  rtl = (gtk_widget_get_direction (GTK_WIDGET (tree_view)) == GTK_TEXT_DIR_RTL);

  total_width = 0;
  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
       list;
       list = (rtl ? list->prev : list->next))
    {
      tmp_column = list->data;

      if (tmp_column == column)
        break;

      if (tmp_column->visible)
        total_width += tmp_column->width;
    }

  if (tmp_column != column)
    {
      g_warning (G_STRLOC": passed-in column isn't in the tree");
      return;
    }

  if (x1)
    *x1 = total_width;

  if (x2)
    {
      if (column->visible)
        *x2 = total_width + column->width;
      else
        *x2 = total_width; /* width of 0 */
    }
}
static void
rotator_get_arrow_xrange (Rotator *tree_view,
				GtkRBTree   *tree,
                                gint        *x1,
                                gint        *x2)
{
  gint x_offset = 0;
  GList *list;
  RotatorColumn *tmp_column = NULL;
  gint total_width;
  gboolean indent_expanders;
  gboolean rtl;

  rtl = (gtk_widget_get_direction (GTK_WIDGET (tree_view)) == GTK_TEXT_DIR_RTL);

  total_width = 0;
  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
       list;
       list = (rtl ? list->prev : list->next))
    {
      tmp_column = list->data;

      if (rotator_is_expander_column (tree_view, tmp_column))
        {
	  if (rtl)
	    x_offset = total_width + tmp_column->width - tree_view->priv->expander_size;
	  else
	    x_offset = total_width;
          break;
        }

      if (tmp_column->visible)
        total_width += tmp_column->width;
    }

  gtk_widget_style_get (GTK_WIDGET (tree_view),
			"indent-expanders", &indent_expanders,
			NULL);

  if (indent_expanders)
    {
      if (rtl)
	x_offset -= tree_view->priv->expander_size * _gtk_rbtree_get_depth (tree);
      else
	x_offset += tree_view->priv->expander_size * _gtk_rbtree_get_depth (tree);
    }

  *x1 = x_offset;
  
  if (tmp_column && tmp_column->visible)
    /* +1 because x2 isn't included in the range. */
    *x2 = *x1 + tree_view->priv->expander_size + 1;
  else
    *x2 = *x1;
}

static void
rotator_build_tree (Rotator *tree_view,
			  GtkRBTree   *tree,
			  GtkTreeIter *iter,
			  gint         depth,
			  gboolean     recurse)
{
  GtkRBNode *temp = NULL;
  GtkTreePath *path = NULL;
  gboolean is_list = GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_IS_LIST);

  do
    {
      gtk_tree_model_ref_node (tree_view->priv->model, iter);
      temp = _gtk_rbtree_insert_after (tree, temp, 0, FALSE);

      if (tree_view->priv->fixed_height > 0)
        {
          if (GTK_RBNODE_FLAG_SET (temp, GTK_RBNODE_INVALID))
	    {
              _gtk_rbtree_node_set_height (tree, temp, tree_view->priv->fixed_height);
	      _gtk_rbtree_node_mark_valid (tree, temp);
	    }
        }

      if (is_list)
        continue;

      if (recurse)
	{
	  GtkTreeIter child;

	  if (!path)
	    path = gtk_tree_model_get_path (tree_view->priv->model, iter);
	  else
	    gtk_tree_path_next (path);

	  if (gtk_tree_model_iter_children (tree_view->priv->model, &child, iter))
	    {
	      gboolean expand;

	      g_signal_emit (tree_view, tree_view_signals[TEST_EXPAND_ROW], 0, iter, path, &expand);

	      if (gtk_tree_model_iter_has_child (tree_view->priv->model, iter)
		  && !expand)
	        {
	          temp->children = _gtk_rbtree_new ();
	          temp->children->parent_tree = tree;
	          temp->children->parent_node = temp;
	          rotator_build_tree (tree_view, temp->children, &child, depth + 1, recurse);
		}
	    }
	}

      if (gtk_tree_model_iter_has_child (tree_view->priv->model, iter))
	{
	  if ((temp->flags&GTK_RBNODE_IS_PARENT) != GTK_RBNODE_IS_PARENT)
	    temp->flags ^= GTK_RBNODE_IS_PARENT;
	}
    }
  while (gtk_tree_model_iter_next (tree_view->priv->model, iter));

  if (path)
    gtk_tree_path_free (path);
}

/* If height is non-NULL, then we set it to be the new height.  if it's all
 * dirty, then height is -1.  We know we'll remeasure dirty rows, anyways.
 */
static gboolean
rotator_discover_dirty_iter (Rotator *tree_view,
				   GtkTreeIter *iter,
				   gint         depth,
				   gint        *height,
				   GtkRBNode   *node)
{
  RotatorColumn *column;
  GList *list;
  gboolean retval = FALSE;
  gint tmpheight;
  gint horizontal_separator;

  gtk_widget_style_get (GTK_WIDGET (tree_view),
			"horizontal-separator", &horizontal_separator,
			NULL);

  if (height)
    *height = -1;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      gint width;
      column = list->data;
      if (column->dirty == TRUE)
	continue;
      if (height == NULL && column->column_type == GTK_ROTATOR_COLUMN_FIXED)
	continue;
      if (!column->visible)
	continue;

      rotator_column_cell_set_cell_data (column, tree_view->priv->model, iter,
					       GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_PARENT),
					       node->children?TRUE:FALSE);

      if (height)
	{
	  rotator_column_cell_get_size (column,
					      NULL, NULL, NULL,
					      &width, &tmpheight);
	  *height = MAX (*height, tmpheight);
	}
      else
	{
	  rotator_column_cell_get_size (column,
					      NULL, NULL, NULL,
					      &width, NULL);
	}

      if (rotator_is_expander_column (tree_view, column))
	{
	  int tmp = 0;

	  tmp = horizontal_separator + width + (depth - 1) * tree_view->priv->level_indentation;
	  if (ROTATOR_DRAW_EXPANDERS (tree_view))
	    tmp += depth * tree_view->priv->expander_size;

	  if (tmp > column->requested_width)
	    {
	      _rotator_column_cell_set_dirty (column, TRUE);
	      retval = TRUE;
	    }
	}
      else
	{
	  if (horizontal_separator + width > column->requested_width)
	    {
	      _rotator_column_cell_set_dirty (column, TRUE);
	      retval = TRUE;
	    }
	}
    }

  return retval;
}

static void
rotator_discover_dirty (Rotator *tree_view,
			      GtkRBTree   *tree,
			      GtkTreeIter *iter,
			      gint         depth)
{
  GtkRBNode *temp = tree->root;
  RotatorColumn *column;
  GList *list;
  GtkTreeIter child;
  gboolean is_all_dirty;

  ROTATOR_INTERNAL_ASSERT_VOID (tree != NULL);

  while (temp->left != tree->nil)
    temp = temp->left;

  do
    {
      ROTATOR_INTERNAL_ASSERT_VOID (temp != NULL);
      is_all_dirty = TRUE;
      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  column = list->data;
	  if (column->dirty == FALSE)
	    {
	      is_all_dirty = FALSE;
	      break;
	    }
	}

      if (is_all_dirty)
	return;

      rotator_discover_dirty_iter (tree_view,
					 iter,
					 depth,
					 NULL,
					 temp);
      if (gtk_tree_model_iter_children (tree_view->priv->model, &child, iter) &&
	  temp->children != NULL)
	rotator_discover_dirty (tree_view, temp->children, &child, depth + 1);
      temp = _gtk_rbtree_next (tree, temp);
    }
  while (gtk_tree_model_iter_next (tree_view->priv->model, iter));
}


/* Make sure the node is visible vertically */
static void
rotator_clamp_node_visible (Rotator *tree_view, GtkRBTree   *tree, GtkRBNode   *node)
{
  gint node_dy, height;
  GtkTreePath *path = NULL;

  if (!GTK_WIDGET_REALIZED (tree_view)) return;

  /* just return if the node is visible, avoiding a costly expose */
  node_dy = _gtk_rbtree_node_find_offset (tree, node);
  height = ROW_HEIGHT (tree_view, GTK_RBNODE_GET_HEIGHT (node));
  if (! GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_INVALID)
      && node_dy >= tree_view->priv->vadjustment->value
      && node_dy + height <= (tree_view->priv->vadjustment->value
                              + tree_view->priv->vadjustment->page_size))
    return;

  path = _rotator_find_path (tree_view, tree, node);
  if (path)
    {
      /* We process updates because we want to clear old selected items when we scroll.
       * if this is removed, we get a "selection streak" at the bottom. */
      gdk_window_process_updates (tree_view->priv->bin_window, TRUE);
      rotator_scroll_to_cell (tree_view, path, NULL, FALSE, 0.0, 0.0);
      gtk_tree_path_free (path);
    }
}

static void
rotator_clamp_column_visible (Rotator       *tree_view, RotatorColumn *column, gboolean           focus_to_cell)
{
  gint x, width;

  if (column == NULL)
    return;

  x = column->button->allocation.x;
  width = column->button->allocation.width;

  if (width > tree_view->priv->hadjustment->page_size)
    {
      /* The column is larger than the horizontal page size.  If the
       * column has cells which can be focussed individually, then we make
       * sure the cell which gets focus is fully visible (if even the
       * focus cell is bigger than the page size, we make sure the
       * left-hand side of the cell is visible).
       *
       * If the column does not have those so-called special cells, we
       * make sure the left-hand side of the column is visible.
       */

      if (focus_to_cell && rotator_has_special_cell (tree_view))
        {
	  GtkTreePath *cursor_path;
	  GdkRectangle background_area, cell_area, focus_area;

	  cursor_path = gtk_tree_row_reference_get_path (tree_view->priv->cursor);

	  rotator_get_cell_area (tree_view,
				       cursor_path, column, &cell_area);
	  rotator_get_background_area (tree_view,
					     cursor_path, column,
					     &background_area);

	  gtk_tree_path_free (cursor_path);

	  _rotator_column_get_focus_area (column,
						&background_area,
						&cell_area,
						&focus_area);

	  x = focus_area.x;
	  width = focus_area.width;

	  if (width < tree_view->priv->hadjustment->page_size)
	    {
	      if ((tree_view->priv->hadjustment->value + tree_view->priv->hadjustment->page_size) < (x + width))
		gtk_adjustment_set_value (tree_view->priv->hadjustment,
					  x + width - tree_view->priv->hadjustment->page_size);
	      else if (tree_view->priv->hadjustment->value > x)
		gtk_adjustment_set_value (tree_view->priv->hadjustment, x);
	    }
	}

      gtk_adjustment_set_value (tree_view->priv->hadjustment,
				CLAMP (x,
				       tree_view->priv->hadjustment->lower,
				       tree_view->priv->hadjustment->upper
				       - tree_view->priv->hadjustment->page_size));
    }
  else
    {
      if ((tree_view->priv->hadjustment->value + tree_view->priv->hadjustment->page_size) < (x + width))
	  gtk_adjustment_set_value (tree_view->priv->hadjustment,
				    x + width - tree_view->priv->hadjustment->page_size);
      else if (tree_view->priv->hadjustment->value > x)
	gtk_adjustment_set_value (tree_view->priv->hadjustment, x);
  }
}

/* This function could be more efficient.  I'll optimize it if profiling seems
 * to imply that it is important */
GtkTreePath *
_rotator_find_path (Rotator *tree_view,
			  GtkRBTree   *tree,
			  GtkRBNode   *node)
{
  GtkTreePath *path;
  GtkRBTree *tmp_tree;
  GtkRBNode *tmp_node, *last;
  gint count;

  path = gtk_tree_path_new ();

  g_return_val_if_fail (node != NULL, path);
  g_return_val_if_fail (node != tree->nil, path);

  count = 1 + node->left->count;

  last = node;
  tmp_node = node->parent;
  tmp_tree = tree;
  while (tmp_tree)
    {
      while (tmp_node != tmp_tree->nil)
	{
	  if (tmp_node->right == last)
	    count += 1 + tmp_node->left->count;
	  last = tmp_node;
	  tmp_node = tmp_node->parent;
	}
      gtk_tree_path_prepend_index (path, count - 1);
      last = tmp_tree->parent_node;
      tmp_tree = tmp_tree->parent_tree;
      if (last)
	{
	  count = 1 + last->left->count;
	  tmp_node = last->parent;
	}
    }
  return path;
}
#endif

#if 0
/* Returns TRUE if we ran out of tree before finding the path.  If the path is
 * invalid (ie. points to a node that's not in the tree), *tree and *node are
 * both set to NULL.
 */
gboolean
_rotator_find_node (Rotator  *tree_view,
			  GtkTreePath  *path,
			  GtkRBTree   **tree,
			  GtkRBNode   **node)
{
  GtkRBNode *tmpnode = NULL;
  GtkRBTree *tmptree = tree_view->priv->tree;
  gint *indices = gtk_tree_path_get_indices (path);
  gint depth = gtk_tree_path_get_depth (path);
  gint i = 0;

  *node = NULL;
  *tree = NULL;

  if (depth == 0 || tmptree == NULL)
    return FALSE;
  do
    {
      tmpnode = _gtk_rbtree_find_count (tmptree, indices[i] + 1);
      ++i;
      if (tmpnode == NULL)
	{
	  *tree = NULL;
	  *node = NULL;
	  return FALSE;
	}
      if (i >= depth)
	{
	  *tree = tmptree;
	  *node = tmpnode;
	  return FALSE;
	}
      *tree = tmptree;
      *node = tmpnode;
      tmptree = tmpnode->children;
      if (tmptree == NULL)
	return TRUE;
    }
  while (1);
}

static gboolean
rotator_is_expander_column (Rotator       *tree_view,
				  RotatorColumn *column)
{
  GList *list;

  if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_IS_LIST))
    return FALSE;

  if (tree_view->priv->expander_column != NULL)
    {
      if (tree_view->priv->expander_column == column)
	return TRUE;
      return FALSE;
    }
  else
    {
      for (list = tree_view->priv->columns;
	   list;
	   list = list->next)
	if (((RotatorColumn *)list->data)->visible)
	  break;
      if (list && list->data == column)
	return TRUE;
    }
  return FALSE;
}

static void
rotator_add_move_binding (GtkBindingSet* binding_set, guint keyval, guint modmask, gboolean add_shifted_binding, GtkMovementStep step, gint count)
{
  
  gtk_binding_entry_add_signal (binding_set, keyval, modmask, "move-cursor", 2, G_TYPE_ENUM, step, G_TYPE_INT, count);

  if (add_shifted_binding)
    gtk_binding_entry_add_signal (binding_set, keyval, GDK_SHIFT_MASK, "move-cursor", 2, G_TYPE_ENUM, step, G_TYPE_INT, count);

  if ((modmask & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
   return;

  gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                "move-cursor", 2,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count);

  gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK,
                                "move-cursor", 2,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count);
}

static gint
rotator_unref_tree_helper (GtkTreeModel *model, GtkTreeIter  *iter, GtkRBTree    *tree, GtkRBNode    *node)
{
  gint retval = FALSE;
  do
    {
      g_return_val_if_fail (node != NULL, FALSE);

      if (node->children)
	{
	  GtkTreeIter child;
	  GtkRBTree *new_tree;
	  GtkRBNode *new_node;

	  new_tree = node->children;
	  new_node = new_tree->root;

	  while (new_node && new_node->left != new_tree->nil)
	    new_node = new_node->left;

	  if (!gtk_tree_model_iter_children (model, &child, iter))
	    return FALSE;

	  retval = retval || rotator_unref_tree_helper (model, &child, new_tree, new_node);
	}

      if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_SELECTED))
	retval = TRUE;
      gtk_tree_model_unref_node (model, iter);
      node = _gtk_rbtree_next (tree, node);
    }
  while (gtk_tree_model_iter_next (model, iter));

  return retval;
}

static gint
rotator_unref_and_check_selection_tree (Rotator *tree_view,
					      GtkRBTree   *tree)
{
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkRBNode *node;
  gint retval;

  if (!tree)
    return FALSE;

  node = tree->root;
  while (node && node->left != tree->nil)
    node = node->left;

  g_return_val_if_fail (node != NULL, FALSE);
  path = _rotator_find_path (tree_view, tree, node);
  gtk_tree_model_get_iter (GTK_TREE_MODEL (tree_view->priv->model),
			   &iter, path);
  retval = rotator_unref_tree_helper (GTK_TREE_MODEL (tree_view->priv->model), &iter, tree, node);
  gtk_tree_path_free (path);

  return retval;
}

static void
rotator_set_column_drag_info (Rotator       *tree_view,
				    RotatorColumn *column)
{
  RotatorColumn *left_column;
  RotatorColumn *cur_column = NULL;
  RotatorColumnReorder *reorder;
  gboolean rtl;
  GList *tmp_list;
  gint left;

  /* We want to precalculate the motion list such that we know what column slots
   * are available.
   */
  left_column = NULL;
  rtl = (gtk_widget_get_direction (GTK_WIDGET (tree_view)) == GTK_TEXT_DIR_RTL);

  /* First, identify all possible drop spots */
  if (rtl)
    tmp_list = g_list_last (tree_view->priv->columns);
  else
    tmp_list = g_list_first (tree_view->priv->columns);

  while (tmp_list)
    {
      cur_column = GTK_ROTATOR_COLUMN (tmp_list->data);
      tmp_list = rtl?g_list_previous (tmp_list):g_list_next (tmp_list);

      if (cur_column->visible == FALSE)
	continue;

      /* If it's not the column moving and func tells us to skip over the column, we continue. */
      if (left_column != column && cur_column != column &&
	  tree_view->priv->column_drop_func &&
	  ! tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	{
	  left_column = cur_column;
	  continue;
	}
      reorder = g_slice_new0 (RotatorColumnReorder);
      reorder->left_column = left_column;
      left_column = reorder->right_column = cur_column;

      tree_view->priv->column_drag_info = g_list_append (tree_view->priv->column_drag_info, reorder);
    }

  /* Add the last one */
  if (tree_view->priv->column_drop_func == NULL ||
      ((left_column != column) &&
       tree_view->priv->column_drop_func (tree_view, column, left_column, NULL, tree_view->priv->column_drop_func_data)))
    {
      reorder = g_slice_new0 (RotatorColumnReorder);
      reorder->left_column = left_column;
      reorder->right_column = NULL;
      tree_view->priv->column_drag_info = g_list_append (tree_view->priv->column_drag_info, reorder);
    }

  /* We quickly check to see if it even makes sense to reorder columns. */
  /* If there is nothing that can be moved, then we return */

  if (tree_view->priv->column_drag_info == NULL)
    return;

  /* We know there are always 2 slots possbile, as you can always return column. */
  /* If that's all there is, return */
  if (tree_view->priv->column_drag_info->next == NULL || 
      (tree_view->priv->column_drag_info->next->next == NULL &&
       ((RotatorColumnReorder *)tree_view->priv->column_drag_info->data)->right_column == column &&
       ((RotatorColumnReorder *)tree_view->priv->column_drag_info->next->data)->left_column == column))
    {
      for (tmp_list = tree_view->priv->column_drag_info; tmp_list; tmp_list = tmp_list->next)
	g_slice_free (RotatorColumnReorder, tmp_list->data);
      g_list_free (tree_view->priv->column_drag_info);
      tree_view->priv->column_drag_info = NULL;
      return;
    }
  /* We fill in the ranges for the columns, now that we've isolated them */
  left = - ROTATOR_COLUMN_DRAG_DEAD_MULTIPLIER (tree_view);

  for (tmp_list = tree_view->priv->column_drag_info; tmp_list; tmp_list = tmp_list->next)
    {
      reorder = (RotatorColumnReorder *) tmp_list->data;

      reorder->left_align = left;
      if (tmp_list->next != NULL)
	{
	  g_assert (tmp_list->next->data);
	  left = reorder->right_align = (reorder->right_column->button->allocation.x +
					 reorder->right_column->button->allocation.width +
					 ((RotatorColumnReorder *)tmp_list->next->data)->left_column->button->allocation.x)/2;
	}
      else
	{
	  gint width;

	  gdk_drawable_get_size (tree_view->priv->header_window, &width, NULL);
	  reorder->right_align = width + ROTATOR_COLUMN_DRAG_DEAD_MULTIPLIER (tree_view);
	}
    }
}

void
_rotator_column_start_drag (Rotator       *tree_view,
				  RotatorColumn *column)
{
  GdkEvent *send_event;
  GtkAllocation allocation;
  gint x, y, width, height;
  GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (tree_view));
  GdkDisplay *display = gdk_screen_get_display (screen);

  g_return_if_fail (tree_view->priv->column_drag_info == NULL);
  g_return_if_fail (tree_view->priv->cur_reorder == NULL);

  rotator_set_column_drag_info (tree_view, column);

  if (tree_view->priv->column_drag_info == NULL)
    return;

  if (tree_view->priv->drag_window == NULL)
    {
      GdkWindowAttr attributes;
      guint attributes_mask;

      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes.x = column->button->allocation.x;
      attributes.y = 0;
      attributes.width = column->button->allocation.width;
      attributes.height = column->button->allocation.height;
      attributes.visual = gtk_widget_get_visual (GTK_WIDGET (tree_view));
      attributes.colormap = gtk_widget_get_colormap (GTK_WIDGET (tree_view));
      attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK | GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK;
      attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

      tree_view->priv->drag_window = gdk_window_new (tree_view->priv->bin_window,
						     &attributes,
						     attributes_mask);
      gdk_window_set_user_data (tree_view->priv->drag_window, GTK_WIDGET (tree_view));
    }

  gdk_display_pointer_ungrab (display, GDK_CURRENT_TIME);
  gdk_display_keyboard_ungrab (display, GDK_CURRENT_TIME);

  gtk_grab_remove (column->button);

  send_event = gdk_event_new (GDK_LEAVE_NOTIFY);
  send_event->crossing.send_event = TRUE;
  send_event->crossing.window = g_object_ref (GTK_BUTTON (column->button)->event_window);
  send_event->crossing.subwindow = NULL;
  send_event->crossing.detail = GDK_NOTIFY_ANCESTOR;
  send_event->crossing.time = GDK_CURRENT_TIME;

  gtk_propagate_event (column->button, send_event);
  gdk_event_free (send_event);

  send_event = gdk_event_new (GDK_BUTTON_RELEASE);
  send_event->button.window = g_object_ref (gdk_screen_get_root_window (screen));
  send_event->button.send_event = TRUE;
  send_event->button.time = GDK_CURRENT_TIME;
  send_event->button.x = -1;
  send_event->button.y = -1;
  send_event->button.axes = NULL;
  send_event->button.state = 0;
  send_event->button.button = 1;
  send_event->button.device = gdk_display_get_core_pointer (display);
  send_event->button.x_root = 0;
  send_event->button.y_root = 0;

  gtk_propagate_event (column->button, send_event);
  gdk_event_free (send_event);

  /* Kids, don't try this at home */
  g_object_ref (column->button);
  gtk_container_remove (GTK_CONTAINER (tree_view), column->button);
  gtk_widget_set_parent_window (column->button, tree_view->priv->drag_window);
  gtk_widget_set_parent (column->button, GTK_WIDGET (tree_view));
  g_object_unref (column->button);

  tree_view->priv->drag_column_x = column->button->allocation.x;
  allocation = column->button->allocation;
  allocation.x = 0;
  gtk_widget_size_allocate (column->button, &allocation);
  gtk_widget_set_parent_window (column->button, tree_view->priv->drag_window);

  tree_view->priv->drag_column = column;
  gdk_window_show (tree_view->priv->drag_window);

  gdk_window_get_origin (tree_view->priv->header_window, &x, &y);
  gdk_drawable_get_size (tree_view->priv->header_window, &width, &height);

  gtk_widget_grab_focus (GTK_WIDGET (tree_view));
  while (gtk_events_pending ())
    gtk_main_iteration ();

  GTK_ROTATOR_SET_FLAG (tree_view, GTK_ROTATOR_IN_COLUMN_DRAG);
  gdk_pointer_grab (tree_view->priv->drag_window,
		    FALSE,
		    GDK_POINTER_MOTION_MASK|GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, GDK_CURRENT_TIME);
  gdk_keyboard_grab (tree_view->priv->drag_window,
		     FALSE,
		     GDK_CURRENT_TIME);
}

static void
rotator_queue_draw_arrow (Rotator        *tree_view,
				GtkRBTree          *tree,
				GtkRBNode          *node,
				const GdkRectangle *clip_rect)
{
  GdkRectangle rect;

  if (!GTK_WIDGET_REALIZED (tree_view))
    return;

  rect.x = 0;
  rect.width = MAX (tree_view->priv->expander_size, MAX (tree_view->priv->width, GTK_WIDGET (tree_view)->allocation.width));

  rect.y = BACKGROUND_FIRST_PIXEL (tree_view, tree, node);
  rect.height = ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node));

  if (clip_rect)
    {
      GdkRectangle new_rect;

      gdk_rectangle_intersect (clip_rect, &rect, &new_rect);

      gdk_window_invalidate_rect (tree_view->priv->bin_window, &new_rect, TRUE);
    }
  else
    {
      gdk_window_invalidate_rect (tree_view->priv->bin_window, &rect, TRUE);
    }
}

void
_rotator_queue_draw_node (Rotator        *tree_view,
				GtkRBTree          *tree,
				GtkRBNode          *node,
				const GdkRectangle *clip_rect)
{
  GdkRectangle rect;

  if (!GTK_WIDGET_REALIZED (tree_view))
    return;

  rect.x = 0;
  rect.width = MAX (tree_view->priv->width, GTK_WIDGET (tree_view)->allocation.width);

  rect.y = BACKGROUND_FIRST_PIXEL (tree_view, tree, node);
  rect.height = ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node));

  if (clip_rect)
    {
      GdkRectangle new_rect;

      gdk_rectangle_intersect (clip_rect, &rect, &new_rect);

      gdk_window_invalidate_rect (tree_view->priv->bin_window, &new_rect, TRUE);
    }
  else
    {
      gdk_window_invalidate_rect (tree_view->priv->bin_window, &rect, TRUE);
    }
}

static void
rotator_queue_draw_path (Rotator        *tree_view,
                               GtkTreePath        *path,
                               const GdkRectangle *clip_rect)
{
  GtkRBTree *tree = NULL;
  GtkRBNode *node = NULL;

  _rotator_find_node (tree_view, path, &tree, &node);

  if (tree)
    _rotator_queue_draw_node (tree_view, tree, node, clip_rect);
}

/* x and y are the mouse position
 */
static void
rotator_draw_arrow (Rotator *tree_view,
                          GtkRBTree   *tree,
			  GtkRBNode   *node,
			  /* in bin_window coordinates */
			  gint         x,
			  gint         y)
{
  GdkRectangle area;
  GtkStateType state;
  GtkWidget *widget;
  gint x_offset = 0;
  gint x2;
  gint vertical_separator;
  gint expander_size;
  GtkExpanderStyle expander_style;

  gtk_widget_style_get (GTK_WIDGET (tree_view),
			"vertical-separator", &vertical_separator,
			NULL);
  expander_size = tree_view->priv->expander_size - EXPANDER_EXTRA_PADDING;

  if (! GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_PARENT))
    return;

  widget = GTK_WIDGET (tree_view);

  rotator_get_arrow_xrange (tree_view, tree, &x_offset, &x2);

  area.x = x_offset;
  area.y = CELL_FIRST_PIXEL (tree_view, tree, node, vertical_separator);
  area.width = expander_size + 2;
  area.height = MAX (CELL_HEIGHT (node, vertical_separator), (expander_size - vertical_separator));

  if (GTK_WIDGET_STATE (tree_view) == GTK_STATE_INSENSITIVE)
    {
      state = GTK_STATE_INSENSITIVE;
    }
  else if (node == tree_view->priv->button_pressed_node)
    {
      if (x >= area.x && x <= (area.x + area.width) &&
	  y >= area.y && y <= (area.y + area.height))
        state = GTK_STATE_ACTIVE;
      else
        state = GTK_STATE_NORMAL;
    }
  else
    {
      if (node == tree_view->priv->prelight_node &&
	  GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_ARROW_PRELIT))
	state = GTK_STATE_PRELIGHT;
      else
	state = GTK_STATE_NORMAL;
    }

  if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_SEMI_EXPANDED))
    expander_style = GTK_EXPANDER_SEMI_EXPANDED;
  else if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_SEMI_COLLAPSED))
    expander_style = GTK_EXPANDER_SEMI_COLLAPSED;
  else if (node->children != NULL)
    expander_style = GTK_EXPANDER_EXPANDED;
  else
    expander_style = GTK_EXPANDER_COLLAPSED;

  gtk_paint_expander (widget->style,
                      tree_view->priv->bin_window,
                      state,
                      &area,
                      widget,
                      "treeview",
		      area.x + area.width / 2,
		      area.y + area.height / 2,
		      expander_style);
}

static void
rotator_focus_to_cursor (Rotator *tree_view)
{
  GtkTreePath *cursor_path;

  if ((tree_view->priv->tree == NULL) ||
      (! GTK_WIDGET_REALIZED (tree_view)))
    return;

  cursor_path = NULL;
  if (tree_view->priv->cursor)
    cursor_path = gtk_tree_row_reference_get_path (tree_view->priv->cursor);

  if (cursor_path == NULL)
    {
      /* Consult the selection before defaulting to the
       * first focusable element
       */
      GList *selected_rows;
      GtkTreeModel *model;
      GtkTreeSelection *selection;

      selection = rotator_get_selection (tree_view);
      selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);

      if (selected_rows)
	{
          cursor_path = gtk_tree_path_copy((const GtkTreePath *)(selected_rows->data));
	  g_list_foreach (selected_rows, (GFunc)gtk_tree_path_free, NULL);
	  g_list_free (selected_rows);
        }
      else
	{
	  cursor_path = gtk_tree_path_new_first ();
	  search_first_focusable_path (tree_view, &cursor_path,
				       TRUE, NULL, NULL);
	}

      gtk_tree_row_reference_free (tree_view->priv->cursor);
      tree_view->priv->cursor = NULL;

      if (cursor_path)
	{
	  if (tree_view->priv->selection->type == GTK_SELECTION_MULTIPLE)
	    rotator_real_set_cursor (tree_view, cursor_path, FALSE, FALSE);
	  else
	    rotator_real_set_cursor (tree_view, cursor_path, TRUE, FALSE);
	}
    }

  if (cursor_path)
    {
      GTK_ROTATOR_SET_FLAG (tree_view, GTK_ROTATOR_DRAW_KEYFOCUS);

      rotator_queue_draw_path (tree_view, cursor_path, NULL);
      gtk_tree_path_free (cursor_path);

      if (tree_view->priv->focus_column == NULL)
	{
	  GList *list;
	  for (list = tree_view->priv->columns; list; list = list->next)
	    {
	      if (GTK_ROTATOR_COLUMN (list->data)->visible)
		{
		  tree_view->priv->focus_column = GTK_ROTATOR_COLUMN (list->data);
		  break;
		}
	    }
	}
    }
}

static void
rotator_move_cursor_up_down (Rotator *tree_view,
				   gint         count)
{
  gint selection_count;
  GtkRBTree *cursor_tree = NULL;
  GtkRBNode *cursor_node = NULL;
  GtkRBTree *new_cursor_tree = NULL;
  GtkRBNode *new_cursor_node = NULL;
  GtkTreePath *cursor_path = NULL;
  gboolean grab_focus = TRUE;
  gboolean selectable;

  if (! GTK_WIDGET_HAS_FOCUS (tree_view))
    return;

  cursor_path = NULL;
  if (!gtk_tree_row_reference_valid (tree_view->priv->cursor))
    /* FIXME: we lost the cursor; should we get the first? */
    return;

  cursor_path = gtk_tree_row_reference_get_path (tree_view->priv->cursor);
  _rotator_find_node (tree_view, cursor_path,
			    &cursor_tree, &cursor_node);

  if (cursor_tree == NULL)
    /* FIXME: we lost the cursor; should we get the first? */
    return;

  selection_count = gtk_tree_selection_count_selected_rows (tree_view->priv->selection);
  selectable = _gtk_tree_selection_row_is_selectable (tree_view->priv->selection,
						      cursor_node,
						      cursor_path);

  if (selection_count == 0
      && tree_view->priv->selection->type != GTK_SELECTION_NONE
      && !tree_view->priv->ctrl_pressed
      && selectable)
    {
      /* Don't move the cursor, but just select the current node */
      new_cursor_tree = cursor_tree;
      new_cursor_node = cursor_node;
    }
  else
    {
      if (count == -1)
	_gtk_rbtree_prev_full (cursor_tree, cursor_node,
			       &new_cursor_tree, &new_cursor_node);
      else
	_gtk_rbtree_next_full (cursor_tree, cursor_node,
			       &new_cursor_tree, &new_cursor_node);
    }

  gtk_tree_path_free (cursor_path);

  if (new_cursor_node)
    {
      cursor_path = _rotator_find_path (tree_view,
					      new_cursor_tree, new_cursor_node);

      search_first_focusable_path (tree_view, &cursor_path,
				   (count != -1),
				   &new_cursor_tree,
				   &new_cursor_node);

      if (cursor_path)
	gtk_tree_path_free (cursor_path);
    }

  /*
   * If the list has only one item and multi-selection is set then select
   * the row (if not yet selected).
   */
  if (tree_view->priv->selection->type == GTK_SELECTION_MULTIPLE &&
      new_cursor_node == NULL)
    {
      if (count == -1)
        _gtk_rbtree_next_full (cursor_tree, cursor_node,
    			       &new_cursor_tree, &new_cursor_node);
      else
        _gtk_rbtree_prev_full (cursor_tree, cursor_node,
			       &new_cursor_tree, &new_cursor_node);

      if (new_cursor_node == NULL
	  && !GTK_RBNODE_FLAG_SET (cursor_node, GTK_RBNODE_IS_SELECTED))
        {
          new_cursor_node = cursor_node;
          new_cursor_tree = cursor_tree;
        }
      else
        {
          new_cursor_node = NULL;
        }
    }

  if (new_cursor_node)
    {
      cursor_path = _rotator_find_path (tree_view, new_cursor_tree, new_cursor_node);
      rotator_real_set_cursor (tree_view, cursor_path, TRUE, TRUE);
      gtk_tree_path_free (cursor_path);
    }
  else
    {
      rotator_clamp_node_visible (tree_view, cursor_tree, cursor_node);

      if (!tree_view->priv->shift_pressed)
        {
          if (! gtk_widget_keynav_failed (GTK_WIDGET (tree_view),
                                          count < 0 ?
                                          GTK_DIR_UP : GTK_DIR_DOWN))
            {
              GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (tree_view));

              if (toplevel)
                gtk_widget_child_focus (toplevel,
                                        count < 0 ?
                                        GTK_DIR_TAB_BACKWARD :
                                        GTK_DIR_TAB_FORWARD);

              grab_focus = FALSE;
            }
        }
      else
        {
          gtk_widget_error_bell (GTK_WIDGET (tree_view));
        }
    }

  if (grab_focus)
    gtk_widget_grab_focus (GTK_WIDGET (tree_view));
}

static void
rotator_move_cursor_page_up_down (Rotator *tree_view,
					gint         count)
{
  GtkRBTree *cursor_tree = NULL;
  GtkRBNode *cursor_node = NULL;
  GtkTreePath *old_cursor_path = NULL;
  GtkTreePath *cursor_path = NULL;
  GtkRBTree *start_cursor_tree = NULL;
  GtkRBNode *start_cursor_node = NULL;
  gint y;
  gint window_y;
  gint vertical_separator;

  if (! GTK_WIDGET_HAS_FOCUS (tree_view))
    return;

  if (gtk_tree_row_reference_valid (tree_view->priv->cursor))
    old_cursor_path = gtk_tree_row_reference_get_path (tree_view->priv->cursor);
  else
    /* This is sorta weird.  Focus in should give us a cursor */
    return;

  gtk_widget_style_get (GTK_WIDGET (tree_view), "vertical-separator", &vertical_separator, NULL);
  _rotator_find_node (tree_view, old_cursor_path,
			    &cursor_tree, &cursor_node);

  if (cursor_tree == NULL)
    {
      /* FIXME: we lost the cursor.  Should we try to get one? */
      gtk_tree_path_free (old_cursor_path);
      return;
    }
  g_return_if_fail (cursor_node != NULL);

  y = _gtk_rbtree_node_find_offset (cursor_tree, cursor_node);
  window_y = RBTREE_Y_TO_TREE_WINDOW_Y (tree_view, y);
  y += tree_view->priv->cursor_offset;
  y += count * (int)tree_view->priv->vadjustment->page_increment;
  y = CLAMP (y, (gint)tree_view->priv->vadjustment->lower,  (gint)tree_view->priv->vadjustment->upper - vertical_separator);

  if (y >= tree_view->priv->height)
    y = tree_view->priv->height - 1;

  tree_view->priv->cursor_offset =
    _gtk_rbtree_find_offset (tree_view->priv->tree, y,
			     &cursor_tree, &cursor_node);

  if (tree_view->priv->cursor_offset > BACKGROUND_HEIGHT (cursor_node))
    {
      _gtk_rbtree_next_full (cursor_tree, cursor_node,
			     &cursor_tree, &cursor_node);
      tree_view->priv->cursor_offset -= BACKGROUND_HEIGHT (cursor_node);
    }

  y -= tree_view->priv->cursor_offset;
  cursor_path = _rotator_find_path (tree_view, cursor_tree, cursor_node);

  start_cursor_tree = cursor_tree;
  start_cursor_node = cursor_node;

  if (! search_first_focusable_path (tree_view, &cursor_path,
				     (count != -1),
				     &cursor_tree, &cursor_node))
    {
      /* It looks like we reached the end of the view without finding
       * a focusable row.  We will step backwards to find the last
       * focusable row.
       */
      cursor_tree = start_cursor_tree;
      cursor_node = start_cursor_node;
      cursor_path = _rotator_find_path (tree_view, cursor_tree, cursor_node);

      search_first_focusable_path (tree_view, &cursor_path,
				   (count == -1),
				   &cursor_tree, &cursor_node);
    }

  if (!cursor_path)
    goto cleanup;

  /* update y */
  y = _gtk_rbtree_node_find_offset (cursor_tree, cursor_node);

  rotator_real_set_cursor (tree_view, cursor_path, TRUE, FALSE);

  y -= window_y;
  rotator_scroll_to_point (tree_view, -1, y);
  rotator_clamp_node_visible (tree_view, cursor_tree, cursor_node);
  _rotator_queue_draw_node (tree_view, cursor_tree, cursor_node, NULL);

  if (!gtk_tree_path_compare (old_cursor_path, cursor_path))
    gtk_widget_error_bell (GTK_WIDGET (tree_view));

  gtk_widget_grab_focus (GTK_WIDGET (tree_view));

cleanup:
  gtk_tree_path_free (old_cursor_path);
  gtk_tree_path_free (cursor_path);
}

static void
rotator_move_cursor_left_right (Rotator *tree_view,
				      gint         count)
{
  GtkRBTree *cursor_tree = NULL;
  GtkRBNode *cursor_node = NULL;
  GtkTreePath *cursor_path = NULL;
  RotatorColumn *column;
  GtkTreeIter iter;
  GList *list;
  gboolean found_column = FALSE;
  gboolean rtl;

  rtl = (gtk_widget_get_direction (GTK_WIDGET (tree_view)) == GTK_TEXT_DIR_RTL);

  if (! GTK_WIDGET_HAS_FOCUS (tree_view))
    return;

  if (gtk_tree_row_reference_valid (tree_view->priv->cursor))
    cursor_path = gtk_tree_row_reference_get_path (tree_view->priv->cursor);
  else
    return;

  _rotator_find_node (tree_view, cursor_path, &cursor_tree, &cursor_node);
  if (cursor_tree == NULL)
    return;
  if (gtk_tree_model_get_iter (tree_view->priv->model, &iter, cursor_path) == FALSE)
    {
      gtk_tree_path_free (cursor_path);
      return;
    }
  gtk_tree_path_free (cursor_path);

  list = rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns);
  if (tree_view->priv->focus_column)
    {
      for (; list; list = (rtl ? list->prev : list->next))
	{
	  if (list->data == tree_view->priv->focus_column)
	    break;
	}
    }

  while (list)
    {
      gboolean left, right;

      column = list->data;
      if (column->visible == FALSE)
	goto loop_end;

      rotator_column_cell_set_cell_data (column,
					       tree_view->priv->model,
					       &iter,
					       GTK_RBNODE_FLAG_SET (cursor_node, GTK_RBNODE_IS_PARENT),
					       cursor_node->children?TRUE:FALSE);

      if (rtl)
        {
	  right = list->prev ? TRUE : FALSE;
	  left = list->next ? TRUE : FALSE;
	}
      else
        {
	  left = list->prev ? TRUE : FALSE;
	  right = list->next ? TRUE : FALSE;
        }

      if (_rotator_column_cell_focus (column, count, left, right))
	{
	  tree_view->priv->focus_column = column;
	  found_column = TRUE;
	  break;
	}
    loop_end:
      if (count == 1)
	list = rtl ? list->prev : list->next;
      else
	list = rtl ? list->next : list->prev;
    }

  if (found_column)
    {
      if (!rotator_has_special_cell (tree_view))
	_rotator_queue_draw_node (tree_view,
				        cursor_tree,
				        cursor_node,
				        NULL);
      g_signal_emit (tree_view, tree_view_signals[CURSOR_CHANGED], 0);
      gtk_widget_grab_focus (GTK_WIDGET (tree_view));
    }
  else
    {
      gtk_widget_error_bell (GTK_WIDGET (tree_view));
    }

  rotator_clamp_column_visible (tree_view,
				      tree_view->priv->focus_column, TRUE);
}

static void
rotator_move_cursor_start_end (Rotator *tree_view,
				     gint         count)
{
  GtkRBTree *cursor_tree;
  GtkRBNode *cursor_node;
  GtkTreePath *path;
  GtkTreePath *old_path;

  if (! GTK_WIDGET_HAS_FOCUS (tree_view))
    return;

  g_return_if_fail (tree_view->priv->tree != NULL);

  rotator_get_cursor (tree_view, &old_path, NULL);

  cursor_tree = tree_view->priv->tree;
  cursor_node = cursor_tree->root;

  if (count == -1)
    {
      while (cursor_node && cursor_node->left != cursor_tree->nil)
	cursor_node = cursor_node->left;

      /* Now go forward to find the first focusable row. */
      path = _rotator_find_path (tree_view, cursor_tree, cursor_node);
      search_first_focusable_path (tree_view, &path,
				   TRUE, &cursor_tree, &cursor_node);
    }
  else
    {
      do
	{
	  while (cursor_node && cursor_node->right != cursor_tree->nil)
	    cursor_node = cursor_node->right;
	  if (cursor_node->children == NULL)
	    break;

	  cursor_tree = cursor_node->children;
	  cursor_node = cursor_tree->root;
	}
      while (1);

      /* Now go backwards to find last focusable row. */
      path = _rotator_find_path (tree_view, cursor_tree, cursor_node);
      search_first_focusable_path (tree_view, &path,
				   FALSE, &cursor_tree, &cursor_node);
    }

  if (!path)
    goto cleanup;

  if (gtk_tree_path_compare (old_path, path))
    {
      rotator_real_set_cursor (tree_view, path, TRUE, TRUE);
      gtk_widget_grab_focus (GTK_WIDGET (tree_view));
    }
  else
    {
      gtk_widget_error_bell (GTK_WIDGET (tree_view));
    }

cleanup:
  gtk_tree_path_free (old_path);
  gtk_tree_path_free (path);
}

static gboolean
rotator_real_select_all (Rotator *tree_view)
{
  if (! GTK_WIDGET_HAS_FOCUS (tree_view))
    return FALSE;

  if (tree_view->priv->selection->type != GTK_SELECTION_MULTIPLE)
    return FALSE;

  gtk_tree_selection_select_all (tree_view->priv->selection);

  return TRUE;
}

static gboolean
rotator_real_unselect_all (Rotator *tree_view)
{
  if (! GTK_WIDGET_HAS_FOCUS (tree_view))
    return FALSE;

  if (tree_view->priv->selection->type != GTK_SELECTION_MULTIPLE)
    return FALSE;

  gtk_tree_selection_unselect_all (tree_view->priv->selection);

  return TRUE;
}
#endif

static gboolean
rotator_real_select_cursor_row (Rotator* tree_view, gboolean start_editing)
{
	PF0;
#if 0
  GtkRBTree *new_tree = NULL;
  GtkRBNode *new_node = NULL;
  GtkRBTree *cursor_tree = NULL;
  GtkRBNode *cursor_node = NULL;
  GtkTreePath *cursor_path = NULL;
  GtkTreeSelectMode mode = 0;

  if (! GTK_WIDGET_HAS_FOCUS (tree_view))
    return FALSE;

  if (tree_view->priv->cursor)
    cursor_path = gtk_tree_row_reference_get_path (tree_view->priv->cursor);

  if (cursor_path == NULL)
    return FALSE;

  _rotator_find_node (tree_view, cursor_path,
			    &cursor_tree, &cursor_node);

  if (cursor_tree == NULL)
    {
      gtk_tree_path_free (cursor_path);
      return FALSE;
    }

  if (!tree_view->priv->shift_pressed && start_editing &&
      tree_view->priv->focus_column)
    {
      if (rotator_start_editing (tree_view, cursor_path))
	{
	  gtk_tree_path_free (cursor_path);
	  return TRUE;
	}
    }

  if (tree_view->priv->ctrl_pressed)
    mode |= GTK_TREE_SELECT_MODE_TOGGLE;
  if (tree_view->priv->shift_pressed)
    mode |= GTK_TREE_SELECT_MODE_EXTEND;

  _gtk_tree_selection_internal_select_node (tree_view->priv->selection,
					    cursor_node,
					    cursor_tree,
					    cursor_path,
                                            mode,
					    FALSE);

  /* We bail out if the original (tree, node) don't exist anymore after
   * handling the selection-changed callback.  We do return TRUE because
   * the key press has been handled at this point.
   */
  _rotator_find_node (tree_view, cursor_path, &new_tree, &new_node);

  if (cursor_tree != new_tree || cursor_node != new_node)
    return FALSE;

  rotator_clamp_node_visible (tree_view, cursor_tree, cursor_node);

  gtk_widget_grab_focus (GTK_WIDGET (tree_view));
  _rotator_queue_draw_node (tree_view, cursor_tree, cursor_node, NULL);

  if (!tree_view->priv->shift_pressed)
    rotator_row_activated (tree_view, cursor_path,
                                 tree_view->priv->focus_column);
    
  gtk_tree_path_free (cursor_path);
#endif

  return TRUE;
}


#if 0
static gboolean
rotator_real_toggle_cursor_row (Rotator *tree_view)
{
  GtkRBTree *new_tree = NULL;
  GtkRBNode *new_node = NULL;
  GtkRBTree *cursor_tree = NULL;
  GtkRBNode *cursor_node = NULL;
  GtkTreePath *cursor_path = NULL;

  if (! GTK_WIDGET_HAS_FOCUS (tree_view))
    return FALSE;

  cursor_path = NULL;
  if (tree_view->priv->cursor)
    cursor_path = gtk_tree_row_reference_get_path (tree_view->priv->cursor);

  if (cursor_path == NULL)
    return FALSE;

  _rotator_find_node (tree_view, cursor_path,
			    &cursor_tree, &cursor_node);
  if (cursor_tree == NULL)
    {
      gtk_tree_path_free (cursor_path);
      return FALSE;
    }

  _gtk_tree_selection_internal_select_node (tree_view->priv->selection,
					    cursor_node,
					    cursor_tree,
					    cursor_path,
                                            GTK_TREE_SELECT_MODE_TOGGLE,
					    FALSE);

  /* We bail out if the original (tree, node) don't exist anymore after
   * handling the selection-changed callback.  We do return TRUE because
   * the key press has been handled at this point.
   */
  _rotator_find_node (tree_view, cursor_path, &new_tree, &new_node);

  if (cursor_tree != new_tree || cursor_node != new_node)
    return FALSE;

  rotator_clamp_node_visible (tree_view, cursor_tree, cursor_node);

  gtk_widget_grab_focus (GTK_WIDGET (tree_view));
  rotator_queue_draw_path (tree_view, cursor_path, NULL);
  gtk_tree_path_free (cursor_path);

  return TRUE;
}

static gboolean
rotator_real_select_cursor_parent (Rotator *tree_view)
{
  GtkRBTree *cursor_tree = NULL;
  GtkRBNode *cursor_node = NULL;
  GtkTreePath *cursor_path = NULL;
  GdkModifierType state;

  if (! GTK_WIDGET_HAS_FOCUS (tree_view))
    return FALSE;

  cursor_path = NULL;
  if (tree_view->priv->cursor)
    cursor_path = gtk_tree_row_reference_get_path (tree_view->priv->cursor);

  if (cursor_path == NULL)
    return FALSE;

  _rotator_find_node (tree_view, cursor_path,
			    &cursor_tree, &cursor_node);
  if (cursor_tree == NULL)
    {
      gtk_tree_path_free (cursor_path);
      return FALSE;
    }

  if (gtk_get_current_event_state (&state))
    {
      if ((state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
        tree_view->priv->ctrl_pressed = TRUE;
    }

  if (cursor_tree->parent_node)
    {
      rotator_queue_draw_path (tree_view, cursor_path, NULL);
      cursor_node = cursor_tree->parent_node;
      cursor_tree = cursor_tree->parent_tree;

      gtk_tree_path_up (cursor_path);

      rotator_real_set_cursor (tree_view, cursor_path, TRUE, FALSE);
    }

  rotator_clamp_node_visible (tree_view, cursor_tree, cursor_node);

  gtk_widget_grab_focus (GTK_WIDGET (tree_view));
  rotator_queue_draw_path (tree_view, cursor_path, NULL);
  gtk_tree_path_free (cursor_path);

  tree_view->priv->ctrl_pressed = FALSE;

  return TRUE;
}
static gboolean
rotator_search_entry_flush_timeout (Rotator *tree_view)
{
  rotator_search_dialog_hide (tree_view->priv->search_window, tree_view);
  tree_view->priv->typeselect_flush_timeout = 0;

  return FALSE;
}

/* Cut and paste from gtkwindow.c */
static void
send_focus_change (GtkWidget *widget,
		   gboolean   in)
{
  GdkEvent *fevent = gdk_event_new (GDK_FOCUS_CHANGE);

  g_object_ref (widget);
   
 if (in)
    GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);
  else
    GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

  fevent->focus_change.type = GDK_FOCUS_CHANGE;
  fevent->focus_change.window = g_object_ref (widget->window);
  fevent->focus_change.in = in;
  
  gtk_widget_event (widget, fevent);
  
  g_object_notify (G_OBJECT (widget), "has-focus");

  g_object_unref (widget);
  gdk_event_free (fevent);
}

static void
rotator_ensure_interactive_directory (Rotator *tree_view)
{
  GtkWidget *frame, *vbox, *toplevel;
  GdkScreen *screen;

  if (tree_view->priv->search_custom_entry_set)
    return;

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (tree_view));
  screen = gtk_widget_get_screen (GTK_WIDGET (tree_view));

   if (tree_view->priv->search_window != NULL)
     {
       if (GTK_WINDOW (toplevel)->group)
	 gtk_window_group_add_window (GTK_WINDOW (toplevel)->group,
				      GTK_WINDOW (tree_view->priv->search_window));
       else if (GTK_WINDOW (tree_view->priv->search_window)->group)
	 gtk_window_group_remove_window (GTK_WINDOW (tree_view->priv->search_window)->group,
					 GTK_WINDOW (tree_view->priv->search_window));
       gtk_window_set_screen (GTK_WINDOW (tree_view->priv->search_window), screen);
       return;
     }
   
  tree_view->priv->search_window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_screen (GTK_WINDOW (tree_view->priv->search_window), screen);

  if (GTK_WINDOW (toplevel)->group)
    gtk_window_group_add_window (GTK_WINDOW (toplevel)->group,
				 GTK_WINDOW (tree_view->priv->search_window));

  gtk_window_set_type_hint (GTK_WINDOW (tree_view->priv->search_window),
			    GDK_WINDOW_TYPE_HINT_UTILITY);
  gtk_window_set_modal (GTK_WINDOW (tree_view->priv->search_window), TRUE);
  g_signal_connect (tree_view->priv->search_window, "delete-event",
		    G_CALLBACK (rotator_search_delete_event),
		    tree_view);
  g_signal_connect (tree_view->priv->search_window, "key-press-event",
		    G_CALLBACK (rotator_search_key_press_event),
		    tree_view);
  g_signal_connect (tree_view->priv->search_window, "scroll-event",
		    G_CALLBACK (rotator_search_scroll_event),
		    tree_view);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_widget_show (frame);
  gtk_container_add (GTK_CONTAINER (tree_view->priv->search_window), frame);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 3);

  /* add entry */
  tree_view->priv->search_entry = gtk_entry_new ();
  gtk_widget_show (tree_view->priv->search_entry);
  g_signal_connect (tree_view->priv->search_entry, "populate-popup",
		    G_CALLBACK (rotator_search_disable_popdown),
		    tree_view);
  g_signal_connect (tree_view->priv->search_entry,
		    "activate", G_CALLBACK (rotator_search_activate),
		    tree_view);
  g_signal_connect (GTK_ENTRY (tree_view->priv->search_entry)->im_context,
		    "preedit-changed",
		    G_CALLBACK (rotator_search_preedit_changed),
		    tree_view);
  gtk_container_add (GTK_CONTAINER (vbox),
		     tree_view->priv->search_entry);

  gtk_widget_realize (tree_view->priv->search_entry);
}

/* Pops up the interactive search entry.  If keybinding is TRUE then the user
 * started this by typing the start_interactive_search keybinding.  Otherwise, it came from 
 */
static gboolean
rotator_real_start_interactive_search (Rotator *tree_view,
					     gboolean     keybinding)
{
  /* We only start interactive search if we have focus or the columns
   * have focus.  If one of our children have focus, we don't want to
   * start the search.
   */
  GList *list;
  gboolean found_focus = FALSE;
  GtkWidgetClass *entry_parent_class;
  
  if (!tree_view->priv->enable_search && !keybinding)
    return FALSE;

  if (tree_view->priv->search_custom_entry_set)
    return FALSE;

  if (tree_view->priv->search_window != NULL &&
      GTK_WIDGET_VISIBLE (tree_view->priv->search_window))
    return TRUE;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      RotatorColumn *column;

      column = list->data;
      if (! column->visible)
	continue;

      if (GTK_WIDGET_HAS_FOCUS (column->button))
	{
	  found_focus = TRUE;
	  break;
	}
    }
  
  if (GTK_WIDGET_HAS_FOCUS (tree_view))
    found_focus = TRUE;

  if (!found_focus)
    return FALSE;

  if (tree_view->priv->search_column < 0)
    return FALSE;

  rotator_ensure_interactive_directory (tree_view);

  if (keybinding)
    gtk_entry_set_text (GTK_ENTRY (tree_view->priv->search_entry), "");

  /* done, show it */
  tree_view->priv->search_position_func (tree_view, tree_view->priv->search_window, tree_view->priv->search_position_user_data);
  gtk_widget_show (tree_view->priv->search_window);
  if (tree_view->priv->search_entry_changed_id == 0)
    {
      tree_view->priv->search_entry_changed_id =
	g_signal_connect (tree_view->priv->search_entry, "changed",
			  G_CALLBACK (rotator_search_init),
			  tree_view);
    }

  tree_view->priv->typeselect_flush_timeout =
    gdk_threads_add_timeout (GTK_ROTATOR_SEARCH_DIALOG_TIMEOUT,
		   (GSourceFunc) rotator_search_entry_flush_timeout,
		   tree_view);

  /* Grab focus will select all the text.  We don't want that to happen, so we
   * call the parent instance and bypass the selection change.  This is probably
   * really non-kosher. */
  entry_parent_class = g_type_class_peek_parent (GTK_ENTRY_GET_CLASS (tree_view->priv->search_entry));
  (entry_parent_class->grab_focus) (tree_view->priv->search_entry);

  /* send focus-in event */
  send_focus_change (tree_view->priv->search_entry, TRUE);

  /* search first matching iter */
  rotator_search_init (tree_view->priv->search_entry, tree_view);

  return TRUE;
}

static gboolean
rotator_start_interactive_search (Rotator *tree_view)
{
  return rotator_real_start_interactive_search (tree_view, TRUE);
}

/* this function returns the new width of the column being resized given
 * the column and x position of the cursor; the x cursor position is passed
 * in as a pointer and automagicly corrected if it's beyond min/max limits
 */
static gint
rotator_new_column_width (Rotator *tree_view, gint       i, gint      *x)
{
  RotatorColumn *column;
  gint width;
  gboolean rtl;

  /* first translate the x position from widget->window
   * to clist->clist_window
   */
  rtl = (gtk_widget_get_direction (GTK_WIDGET (tree_view)) == GTK_TEXT_DIR_RTL);
  column = g_list_nth (tree_view->priv->columns, i)->data;
  width = rtl ? (column->button->allocation.x + column->button->allocation.width - *x) : (*x - column->button->allocation.x);
 
  /* Clamp down the value */
  if (column->min_width == -1)
    width = MAX (column->button->requisition.width,
		 width);
  else
    width = MAX (column->min_width,
		 width);
  if (column->max_width != -1)
    width = MIN (width, column->max_width);

  *x = rtl ? (column->button->allocation.x + column->button->allocation.width - width) : (column->button->allocation.x + width);
 
  return width;
}


/* FIXME this adjust_allocation is a big cut-and-paste from
 * GtkCList, needs to be some "official" way to do this
 * factored out.
 */
typedef struct
{
  GdkWindow *window;
  int dx;
  int dy;
} ScrollData;

/* The window to which widget->window is relative */
#define ALLOCATION_WINDOW(widget)		\
   (GTK_WIDGET_NO_WINDOW (widget) ?		\
    (widget)->window :                          \
     gdk_window_get_parent ((widget)->window))

static void
adjust_allocation_recurse (GtkWidget *widget,
			   gpointer   data)
{
  ScrollData *scroll_data = data;

  /* Need to really size allocate instead of just poking
   * into widget->allocation if the widget is not realized.
   * FIXME someone figure out why this was.
   */
  if (!GTK_WIDGET_REALIZED (widget))
    {
      if (GTK_WIDGET_VISIBLE (widget))
	{
	  GdkRectangle tmp_rectangle = widget->allocation;
	  tmp_rectangle.x += scroll_data->dx;
          tmp_rectangle.y += scroll_data->dy;
          
	  gtk_widget_size_allocate (widget, &tmp_rectangle);
	}
    }
  else
    {
      if (ALLOCATION_WINDOW (widget) == scroll_data->window)
	{
	  widget->allocation.x += scroll_data->dx;
          widget->allocation.y += scroll_data->dy;
          
	  if (GTK_IS_CONTAINER (widget))
	    gtk_container_forall (GTK_CONTAINER (widget),
				  adjust_allocation_recurse,
				  data);
	}
    }
}

static void
adjust_allocation (GtkWidget *widget,
		   int        dx,
                   int        dy)
{
  ScrollData scroll_data;

  if (GTK_WIDGET_REALIZED (widget))
    scroll_data.window = ALLOCATION_WINDOW (widget);
  else
    scroll_data.window = NULL;
    
  scroll_data.dx = dx;
  scroll_data.dy = dy;
  
  adjust_allocation_recurse (widget, &scroll_data);
}

/* Callbacks */
static void
rotator_adjustment_changed (GtkAdjustment *adjustment,
				  Rotator   *tree_view)
{
  if (GTK_WIDGET_REALIZED (tree_view))
    {
      gint dy;
	
      gdk_window_move (tree_view->priv->bin_window,
		       - tree_view->priv->hadjustment->value,
		       ROTATOR_HEADER_HEIGHT (tree_view));
      gdk_window_move (tree_view->priv->header_window,
		       - tree_view->priv->hadjustment->value,
		       0);
      dy = tree_view->priv->dy - (int) tree_view->priv->vadjustment->value;
      if (dy && tree_view->priv->edited_column)
	{
	  if (GTK_IS_WIDGET (tree_view->priv->edited_column->editable_widget))
	    {
	      GList *list;
	      GtkWidget *widget;
	      RotatorChild *child = NULL;

	      widget = GTK_WIDGET (tree_view->priv->edited_column->editable_widget);
	      adjust_allocation (widget, 0, dy); 
	      
	      for (list = tree_view->priv->children; list; list = list->next)
		{
		  child = (RotatorChild *)list->data;
		  if (child->widget == widget)
		    {
		      child->y += dy;
		      break;
		    }
		}
	    }
	}
      gdk_window_scroll (tree_view->priv->bin_window, 0, dy);

      if (tree_view->priv->dy != (int) tree_view->priv->vadjustment->value)
        {
          /* update our dy and top_row */
          tree_view->priv->dy = (int) tree_view->priv->vadjustment->value;
	}
    }
}
#endif



/* Public methods
 */

/**
 * rotator_new:
 *
 * Creates a new #Rotator widget.
 *
 * Return value: A newly created #Rotator widget.
 **/
GtkWidget *
rotator_new (void)
{
  return g_object_new (GTK_TYPE_ROTATOR, NULL);
}

/**
 * rotator_new_with_model:
 * @model: the model.
 *
 * Creates a new #Rotator widget with the model initialized to @model.
 *
 * Return value: A newly created #Rotator widget.
 **/
GtkWidget *
rotator_new_with_model (GtkTreeModel* model)
{
	return g_object_new (GTK_TYPE_ROTATOR, "model", model, NULL);
}


#if 0
/* Public Accessors
 */

/**
 * rotator_get_model:
 * @tree_view: a #Rotator
 *
 * Returns the model the #Rotator is based on.  Returns %NULL if the
 * model is unset.
 *
 * Return value: A #GtkTreeModel, or %NULL if none is currently being used.
 **/
GtkTreeModel *
rotator_get_model (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), NULL);

  return tree_view->priv->model;
}
#endif

/**
 * rotator_set_model:
 * @tree_view: A #GtkTreeNode.
 * @model: The model.
 *
 * Sets the model for a #Rotator.  If the @tree_view already has a model
 * set, it will remove it before setting the new model.  If @model is %NULL, 
 * then it will unset the old model.
 **/
void
rotator_set_model (Rotator* tree_view, GtkTreeModel* model)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (model) g_return_if_fail (GTK_IS_TREE_MODEL(model));

  if (model == tree_view->priv->model) return;

#if 0
  if (tree_view->priv->scroll_to_path)
    {
      gtk_tree_row_reference_free (tree_view->priv->scroll_to_path);
      tree_view->priv->scroll_to_path = NULL;
    }
#endif


  if (tree_view->priv->model) {
#if 0
      GList *tmplist = tree_view->priv->columns;

      rotator_unref_and_check_selection_tree (tree_view, tree_view->priv->tree);
      rotator_stop_editing (tree_view, TRUE);

      remove_expand_collapse_timeout (tree_view);

#endif
      g_signal_handlers_disconnect_by_func (tree_view->priv->model, rotator_row_changed, tree_view);
      g_signal_handlers_disconnect_by_func (tree_view->priv->model, rotator_row_inserted, tree_view);
      g_signal_handlers_disconnect_by_func (tree_view->priv->model, rotator_row_deleted, tree_view);
#if 0
      g_signal_handlers_disconnect_by_func (tree_view->priv->model, rotator_row_has_child_toggled, tree_view);
      g_signal_handlers_disconnect_by_func (tree_view->priv->model, rotator_rows_reordered, tree_view);

      for (; tmplist; tmplist = tmplist->next) _rotator_column_unset_model (tmplist->data, tree_view->priv->model);

      if (tree_view->priv->tree) rotator_free_rbtree (tree_view);

      gtk_tree_row_reference_free (tree_view->priv->drag_dest_row);
      tree_view->priv->drag_dest_row = NULL;
      gtk_tree_row_reference_free (tree_view->priv->cursor);
      tree_view->priv->cursor = NULL;
      gtk_tree_row_reference_free (tree_view->priv->anchor);
      tree_view->priv->anchor = NULL;
      gtk_tree_row_reference_free (tree_view->priv->top_row);
      tree_view->priv->top_row = NULL;
      gtk_tree_row_reference_free (tree_view->priv->last_button_press);
      tree_view->priv->last_button_press = NULL;
      gtk_tree_row_reference_free (tree_view->priv->last_button_press_2);
      tree_view->priv->last_button_press_2 = NULL;
      gtk_tree_row_reference_free (tree_view->priv->scroll_to_path);
      tree_view->priv->scroll_to_path = NULL;

      tree_view->priv->scroll_to_column = NULL;

      g_object_unref (tree_view->priv->model);

      tree_view->priv->search_column = -1;
      tree_view->priv->fixed_height_check = 0;
      tree_view->priv->fixed_height = -1;
      tree_view->priv->dy = tree_view->priv->top_row_dy = 0;
#endif
    }

  tree_view->priv->model = model;

#if 0
  if (tree_view->priv->model)
    {
      gint i;
      GtkTreePath *path;
      GtkTreeIter iter;
      GtkTreeModelFlags flags;

      if (tree_view->priv->search_column == -1)
	{
	  for (i = 0; i < gtk_tree_model_get_n_columns (model); i++)
	    {
	      GType type = gtk_tree_model_get_column_type (model, i);

	      if (g_value_type_transformable (type, G_TYPE_STRING))
		{
		  tree_view->priv->search_column = i;
		  break;
		}
	    }
	}

#endif
      g_object_ref (tree_view->priv->model);
      g_signal_connect (tree_view->priv->model, "row-changed", G_CALLBACK (rotator_row_changed), tree_view);
      g_signal_connect (tree_view->priv->model, "row-inserted", G_CALLBACK (rotator_row_inserted), tree_view);
      g_signal_connect (tree_view->priv->model, "row-deleted", G_CALLBACK (rotator_row_deleted), tree_view);
#if 0
      g_signal_connect (tree_view->priv->model, "row-has-child-toggled", G_CALLBACK (rotator_row_has_child_toggled), tree_view);
      g_signal_connect (tree_view->priv->model, "rows-reordered", G_CALLBACK (rotator_rows_reordered), tree_view);

      flags = gtk_tree_model_get_flags (tree_view->priv->model);
      if ((flags & GTK_TREE_MODEL_LIST_ONLY) == GTK_TREE_MODEL_LIST_ONLY)
        GTK_ROTATOR_SET_FLAG (tree_view, GTK_ROTATOR_IS_LIST);
      else
        GTK_ROTATOR_UNSET_FLAG (tree_view, GTK_ROTATOR_IS_LIST);

      path = gtk_tree_path_new_first ();
      if (gtk_tree_model_get_iter (tree_view->priv->model, &iter, path))
	{
	  tree_view->priv->tree = _gtk_rbtree_new ();
	  rotator_build_tree (tree_view, tree_view->priv->tree, &iter, 1, FALSE);
	}
      gtk_tree_path_free (path);

      /*  FIXME: do I need to do this? rotator_create_buttons (tree_view); */
      install_presize_handler (tree_view);
    }

  g_object_notify (G_OBJECT (tree_view), "model");

  if (tree_view->priv->selection)
  _gtk_tree_selection_emit_changed (tree_view->priv->selection);

  if (GTK_WIDGET_REALIZED (tree_view))
    gtk_widget_queue_resize (GTK_WIDGET (tree_view));
#endif
}

#if 0
/**
 * rotator_get_selection:
 * @tree_view: A #Rotator.
 *
 * Gets the #GtkTreeSelection associated with @tree_view.
 *
 * Return value: A #GtkTreeSelection object.
 **/
GtkTreeSelection *
rotator_get_selection (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), NULL);

  return tree_view->priv->selection;
}

/**
 * rotator_get_hadjustment:
 * @tree_view: A #Rotator
 *
 * Gets the #GtkAdjustment currently being used for the horizontal aspect.
 *
 * Return value: A #GtkAdjustment object, or %NULL if none is currently being
 * used.
 **/
GtkAdjustment *
rotator_get_hadjustment (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), NULL);

  if (tree_view->priv->hadjustment == NULL)
    rotator_set_hadjustment (tree_view, NULL);

  return tree_view->priv->hadjustment;
}

/**
 * rotator_set_hadjustment:
 * @tree_view: A #Rotator
 * @adjustment: The #GtkAdjustment to set, or %NULL
 *
 * Sets the #GtkAdjustment for the current horizontal aspect.
 **/
void
rotator_set_hadjustment (Rotator   *tree_view,
			       GtkAdjustment *adjustment)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  rotator_set_adjustments (tree_view,
				 adjustment,
				 tree_view->priv->vadjustment);

  g_object_notify (G_OBJECT (tree_view), "hadjustment");
}

/**
 * rotator_get_vadjustment:
 * @tree_view: A #Rotator
 *
 * Gets the #GtkAdjustment currently being used for the vertical aspect.
 *
 * Return value: A #GtkAdjustment object, or %NULL if none is currently being
 * used.
 **/
GtkAdjustment *
rotator_get_vadjustment (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), NULL);

  if (tree_view->priv->vadjustment == NULL)
    rotator_set_vadjustment (tree_view, NULL);

  return tree_view->priv->vadjustment;
}

/**
 * rotator_set_vadjustment:
 * @tree_view: A #Rotator
 * @adjustment: The #GtkAdjustment to set, or %NULL
 *
 * Sets the #GtkAdjustment for the current vertical aspect.
 **/
void
rotator_set_vadjustment (Rotator   *tree_view,
			       GtkAdjustment *adjustment)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  rotator_set_adjustments (tree_view,
				 tree_view->priv->hadjustment,
				 adjustment);

  g_object_notify (G_OBJECT (tree_view), "vadjustment");
}

/* Column and header operations */

/**
 * rotator_get_headers_visible:
 * @tree_view: A #Rotator.
 *
 * Returns %TRUE if the headers on the @tree_view are visible.
 *
 * Return value: Whether the headers are visible or not.
 **/
gboolean
rotator_get_headers_visible (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);

  return GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_HEADERS_VISIBLE);
}

/**
 * rotator_set_headers_visible:
 * @tree_view: A #Rotator.
 * @headers_visible: %TRUE if the headers are visible
 *
 * Sets the visibility state of the headers.
 **/
void
rotator_set_headers_visible (Rotator *tree_view,
				   gboolean     headers_visible)
{
  gint x, y;
  GList *list;
  RotatorColumn *column;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  headers_visible = !! headers_visible;

  if (GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_HEADERS_VISIBLE) == headers_visible)
    return;

  if (headers_visible)
    GTK_ROTATOR_SET_FLAG (tree_view, GTK_ROTATOR_HEADERS_VISIBLE);
  else
    GTK_ROTATOR_UNSET_FLAG (tree_view, GTK_ROTATOR_HEADERS_VISIBLE);

  if (GTK_WIDGET_REALIZED (tree_view))
    {
      gdk_window_get_position (tree_view->priv->bin_window, &x, &y);
      if (headers_visible)
	{
	  gdk_window_move_resize (tree_view->priv->bin_window, x, y  + ROTATOR_HEADER_HEIGHT (tree_view), tree_view->priv->width, GTK_WIDGET (tree_view)->allocation.height -  + ROTATOR_HEADER_HEIGHT (tree_view));

          if (GTK_WIDGET_MAPPED (tree_view))
            rotator_map_buttons (tree_view);
 	}
      else
	{
	  gdk_window_move_resize (tree_view->priv->bin_window, x, y, tree_view->priv->width, tree_view->priv->height);

	  for (list = tree_view->priv->columns; list; list = list->next)
	    {
	      column = list->data;
	      gtk_widget_unmap (column->button);
	    }
	  gdk_window_hide (tree_view->priv->header_window);
	}
    }

  tree_view->priv->vadjustment->page_size = GTK_WIDGET (tree_view)->allocation.height - ROTATOR_HEADER_HEIGHT (tree_view);
  tree_view->priv->vadjustment->page_increment = (GTK_WIDGET (tree_view)->allocation.height - ROTATOR_HEADER_HEIGHT (tree_view)) / 2;
  tree_view->priv->vadjustment->lower = 0;
  tree_view->priv->vadjustment->upper = tree_view->priv->height;
  gtk_adjustment_changed (tree_view->priv->vadjustment);

  gtk_widget_queue_resize (GTK_WIDGET (tree_view));

  g_object_notify (G_OBJECT (tree_view), "headers-visible");
}

/**
 * rotator_columns_autosize:
 * @tree_view: A #Rotator.
 *
 * Resizes all columns to their optimal width. Only works after the
 * treeview has been realized.
 **/
void
rotator_columns_autosize (Rotator *tree_view)
{
  gboolean dirty = FALSE;
  GList *list;
  RotatorColumn *column;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      column = list->data;
      if (column->column_type == GTK_ROTATOR_COLUMN_AUTOSIZE)
	continue;
      _rotator_column_cell_set_dirty (column, TRUE);
      dirty = TRUE;
    }

  if (dirty)
    gtk_widget_queue_resize (GTK_WIDGET (tree_view));
}

/**
 * rotator_set_headers_clickable:
 * @tree_view: A #Rotator.
 * @setting: %TRUE if the columns are clickable.
 *
 * Allow the column title buttons to be clicked.
 **/
void
rotator_set_headers_clickable (Rotator *tree_view,
				     gboolean   setting)
{
  GList *list;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  for (list = tree_view->priv->columns; list; list = list->next)
    rotator_column_set_clickable (GTK_ROTATOR_COLUMN (list->data), setting);

  g_object_notify (G_OBJECT (tree_view), "headers-clickable");
}


/**
 * rotator_get_headers_clickable:
 * @tree_view: A #Rotator.
 *
 * Returns whether all header columns are clickable.
 *
 * Return value: %TRUE if all header columns are clickable, otherwise %FALSE
 *
 * Since: 2.10
 **/
gboolean 
rotator_get_headers_clickable (Rotator *tree_view)
{
  GList *list;
  
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);

  for (list = tree_view->priv->columns; list; list = list->next)
    if (!GTK_ROTATOR_COLUMN (list->data)->clickable)
      return FALSE;

  return TRUE;
}

/**
 * rotator_set_rules_hint
 * @tree_view: a #Rotator
 * @setting: %TRUE if the tree requires reading across rows
 *
 * This function tells GTK+ that the user interface for your
 * application requires users to read across tree rows and associate
 * cells with one another. By default, GTK+ will then render the tree
 * with alternating row colors. Do <emphasis>not</emphasis> use it
 * just because you prefer the appearance of the ruled tree; that's a
 * question for the theme. Some themes will draw tree rows in
 * alternating colors even when rules are turned off, and users who
 * prefer that appearance all the time can choose those themes. You
 * should call this function only as a <emphasis>semantic</emphasis>
 * hint to the theme engine that your tree makes alternating colors
 * useful from a functional standpoint (since it has lots of columns,
 * generally).
 *
 **/
void
rotator_set_rules_hint (Rotator  *tree_view,
                              gboolean      setting)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  setting = setting != FALSE;

  if (tree_view->priv->has_rules != setting)
    {
      tree_view->priv->has_rules = setting;
      gtk_widget_queue_draw (GTK_WIDGET (tree_view));
    }

  g_object_notify (G_OBJECT (tree_view), "rules-hint");
}

/**
 * rotator_get_rules_hint
 * @tree_view: a #Rotator
 *
 * Gets the setting set by rotator_set_rules_hint().
 *
 * Return value: %TRUE if rules are useful for the user of this tree
 **/
gboolean
rotator_get_rules_hint (Rotator  *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);

  return tree_view->priv->has_rules;
}

/* Public Column functions
 */

/**
 * rotator_append_column:
 * @tree_view: A #Rotator.
 * @column: The #RotatorColumn to add.
 *
 * Appends @column to the list of columns. If @tree_view has "fixed_height"
 * mode enabled, then @column must have its "sizing" property set to be
 * GTK_ROTATOR_COLUMN_FIXED.
 *
 * Return value: The number of columns in @tree_view after appending.
 **/
gint
rotator_append_column (Rotator       *tree_view,
			     RotatorColumn *column)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), -1);
  g_return_val_if_fail (GTK_IS_ROTATOR_COLUMN (column), -1);
  g_return_val_if_fail (column->tree_view == NULL, -1);

  return rotator_insert_column (tree_view, column, -1);
}


/**
 * rotator_remove_column:
 * @tree_view: A #Rotator.
 * @column: The #RotatorColumn to remove.
 *
 * Removes @column from @tree_view.
 *
 * Return value: The number of columns in @tree_view after removing.
 **/
gint
rotator_remove_column (Rotator       *tree_view,
                             RotatorColumn *column)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), -1);
  g_return_val_if_fail (GTK_IS_ROTATOR_COLUMN (column), -1);
  g_return_val_if_fail (column->tree_view == GTK_WIDGET (tree_view), -1);

  if (tree_view->priv->focus_column == column)
    tree_view->priv->focus_column = NULL;

  if (tree_view->priv->edited_column == column)
    {
      rotator_stop_editing (tree_view, TRUE);

      /* no need to, but just to be sure ... */
      tree_view->priv->edited_column = NULL;
    }

  g_signal_handlers_disconnect_by_func (column,
                                        G_CALLBACK (column_sizing_notify),
                                        tree_view);

  _rotator_column_unset_tree_view (column);

  tree_view->priv->columns = g_list_remove (tree_view->priv->columns, column);
  tree_view->priv->n_columns--;

  if (GTK_WIDGET_REALIZED (tree_view))
    {
      GList *list;

      _rotator_column_unrealize_button (column);
      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  RotatorColumn *tmp_column;

	  tmp_column = GTK_ROTATOR_COLUMN (list->data);
	  if (tmp_column->visible)
	    _rotator_column_cell_set_dirty (tmp_column, TRUE);
	}

      if (tree_view->priv->n_columns == 0 &&
	  rotator_get_headers_visible (tree_view))
	gdk_window_hide (tree_view->priv->header_window);

      gtk_widget_queue_resize (GTK_WIDGET (tree_view));
    }

  g_object_unref (column);
  g_signal_emit (tree_view, tree_view_signals[COLUMNS_CHANGED], 0);

  return tree_view->priv->n_columns;
}

/**
 * rotator_insert_column:
 * @tree_view: A #Rotator.
 * @column: The #RotatorColumn to be inserted.
 * @position: The position to insert @column in.
 *
 * This inserts the @column into the @tree_view at @position.  If @position is
 * -1, then the column is inserted at the end. If @tree_view has
 * "fixed_height" mode enabled, then @column must have its "sizing" property
 * set to be GTK_ROTATOR_COLUMN_FIXED.
 *
 * Return value: The number of columns in @tree_view after insertion.
 **/
gint
rotator_insert_column (Rotator       *tree_view,
                             RotatorColumn *column,
                             gint               position)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), -1);
  g_return_val_if_fail (GTK_IS_ROTATOR_COLUMN (column), -1);
  g_return_val_if_fail (column->tree_view == NULL, -1);

  if (tree_view->priv->fixed_height_mode)
    g_return_val_if_fail (rotator_column_get_sizing (column)
                          == GTK_ROTATOR_COLUMN_FIXED, -1);

  g_object_ref_sink (column);

  if (tree_view->priv->n_columns == 0 &&
      GTK_WIDGET_REALIZED (tree_view) &&
      rotator_get_headers_visible (tree_view))
    {
      gdk_window_show (tree_view->priv->header_window);
    }

  g_signal_connect (column, "notify::sizing",
                    G_CALLBACK (column_sizing_notify), tree_view);

  tree_view->priv->columns = g_list_insert (tree_view->priv->columns,
					    column, position);
  tree_view->priv->n_columns++;

  _rotator_column_set_tree_view (column, tree_view);

  if (GTK_WIDGET_REALIZED (tree_view))
    {
      GList *list;

      _rotator_column_realize_button (column);

      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  column = GTK_ROTATOR_COLUMN (list->data);
	  if (column->visible)
	    _rotator_column_cell_set_dirty (column, TRUE);
	}
      gtk_widget_queue_resize (GTK_WIDGET (tree_view));
    }

  g_signal_emit (tree_view, tree_view_signals[COLUMNS_CHANGED], 0);

  return tree_view->priv->n_columns;
}

/**
 * rotator_insert_column_with_attributes:
 * @tree_view: A #Rotator
 * @position: The position to insert the new column in.
 * @title: The title to set the header to.
 * @cell: The #GtkCellRenderer.
 * @Varargs: A %NULL-terminated list of attributes.
 *
 * Creates a new #RotatorColumn and inserts it into the @tree_view at
 * @position.  If @position is -1, then the newly created column is inserted at
 * the end.  The column is initialized with the attributes given. If @tree_view
 * has "fixed_height" mode enabled, then the new column will have its sizing
 * property set to be GTK_ROTATOR_COLUMN_FIXED.
 *
 * Return value: The number of columns in @tree_view after insertion.
 **/
gint
rotator_insert_column_with_attributes (Rotator     *tree_view,
					     gint             position,
					     const gchar     *title,
					     GtkCellRenderer *cell,
					     ...)
{
  RotatorColumn *column;
  gchar *attribute;
  va_list args;
  gint column_id;

  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), -1);

  column = rotator_column_new ();
  if (tree_view->priv->fixed_height_mode)
    rotator_column_set_sizing (column, GTK_ROTATOR_COLUMN_FIXED);

  rotator_column_set_title (column, title);
  rotator_column_pack_start (column, cell, TRUE);

  va_start (args, cell);

  attribute = va_arg (args, gchar *);

  while (attribute != NULL)
    {
      column_id = va_arg (args, gint);
      rotator_column_add_attribute (column, cell, attribute, column_id);
      attribute = va_arg (args, gchar *);
    }

  va_end (args);

  rotator_insert_column (tree_view, column, position);

  return tree_view->priv->n_columns;
}

/**
 * rotator_insert_column_with_data_func:
 * @tree_view: a #Rotator
 * @position: Position to insert, -1 for append
 * @title: column title
 * @cell: cell renderer for column
 * @func: function to set attributes of cell renderer
 * @data: data for @func
 * @dnotify: destroy notifier for @data
 *
 * Convenience function that inserts a new column into the #Rotator
 * with the given cell renderer and a #GtkCellDataFunc to set cell renderer
 * attributes (normally using data from the model). See also
 * rotator_column_set_cell_data_func(), gtk_tree_view_column_pack_start().
 * If @tree_view has "fixed_height" mode enabled, then the new column will have its
 * "sizing" property set to be GTK_ROTATOR_COLUMN_FIXED.
 *
 * Return value: number of columns in the tree view post-insert
 **/
gint
rotator_insert_column_with_data_func  (Rotator               *tree_view,
                                             gint                       position,
                                             const gchar               *title,
                                             GtkCellRenderer           *cell,
                                             GtkTreeCellDataFunc        func,
                                             gpointer                   data,
                                             GDestroyNotify             dnotify)
{
  RotatorColumn *column;

  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), -1);

  column = rotator_column_new ();
  if (tree_view->priv->fixed_height_mode)
    rotator_column_set_sizing (column, GTK_ROTATOR_COLUMN_FIXED);

  rotator_column_set_title (column, title);
  rotator_column_pack_start (column, cell, TRUE);
  rotator_column_set_cell_data_func (column, cell, func, data, dnotify);

  rotator_insert_column (tree_view, column, position);

  return tree_view->priv->n_columns;
}

/**
 * rotator_get_column:
 * @tree_view: A #Rotator.
 * @n: The position of the column, counting from 0.
 *
 * Gets the #RotatorColumn at the given position in the #tree_view.
 *
 * Return value: The #RotatorColumn, or %NULL if the position is outside the
 * range of columns.
 **/
RotatorColumn *
rotator_get_column (Rotator *tree_view,
			  gint         n)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), NULL);

  if (n < 0 || n >= tree_view->priv->n_columns)
    return NULL;

  if (tree_view->priv->columns == NULL)
    return NULL;

  return GTK_ROTATOR_COLUMN (g_list_nth (tree_view->priv->columns, n)->data);
}

/**
 * rotator_get_columns:
 * @tree_view: A #Rotator
 *
 * Returns a #GList of all the #RotatorColumn s currently in @tree_view.
 * The returned list must be freed with g_list_free ().
 *
 * Return value: A list of #RotatorColumn s
 **/
GList *
rotator_get_columns (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), NULL);

  return g_list_copy (tree_view->priv->columns);
}

/**
 * rotator_move_column_after:
 * @tree_view: A #Rotator
 * @column: The #RotatorColumn to be moved.
 * @base_column: The #RotatorColumn to be moved relative to, or %NULL.
 *
 * Moves @column to be after to @base_column.  If @base_column is %NULL, then
 * @column is placed in the first position.
 **/
void
rotator_move_column_after (Rotator       *tree_view,
				 RotatorColumn *column,
				 RotatorColumn *base_column)
{
  GList *column_list_el, *base_el = NULL;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  column_list_el = g_list_find (tree_view->priv->columns, column);
  g_return_if_fail (column_list_el != NULL);

  if (base_column)
    {
      base_el = g_list_find (tree_view->priv->columns, base_column);
      g_return_if_fail (base_el != NULL);
    }

  if (column_list_el->prev == base_el)
    return;

  tree_view->priv->columns = g_list_remove_link (tree_view->priv->columns, column_list_el);
  if (base_el == NULL)
    {
      column_list_el->prev = NULL;
      column_list_el->next = tree_view->priv->columns;
      if (column_list_el->next)
	column_list_el->next->prev = column_list_el;
      tree_view->priv->columns = column_list_el;
    }
  else
    {
      column_list_el->prev = base_el;
      column_list_el->next = base_el->next;
      if (column_list_el->next)
	column_list_el->next->prev = column_list_el;
      base_el->next = column_list_el;
    }

  if (GTK_WIDGET_REALIZED (tree_view))
    {
      gtk_widget_queue_resize (GTK_WIDGET (tree_view));
      rotator_size_allocate_columns (GTK_WIDGET (tree_view), NULL);
    }

  g_signal_emit (tree_view, tree_view_signals[COLUMNS_CHANGED], 0);
}

/**
 * rotator_set_expander_column:
 * @tree_view: A #Rotator
 * @column: %NULL, or the column to draw the expander arrow at.
 *
 * Sets the column to draw the expander arrow at. It must be in @tree_view.  
 * If @column is %NULL, then the expander arrow is always at the first 
 * visible column.
 *
 * If you do not want expander arrow to appear in your tree, set the 
 * expander column to a hidden column.
 **/
void
rotator_set_expander_column (Rotator       *tree_view,
                                   RotatorColumn *column)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  if (column != NULL)
    g_return_if_fail (GTK_IS_ROTATOR_COLUMN (column));

  if (tree_view->priv->expander_column != column)
    {
      GList *list;

      if (column)
	{
	  /* Confirm that column is in tree_view */
	  for (list = tree_view->priv->columns; list; list = list->next)
	    if (list->data == column)
	      break;
	  g_return_if_fail (list != NULL);
	}

      tree_view->priv->expander_column = column;
      g_object_notify (G_OBJECT (tree_view), "expander-column");
    }
}

/**
 * rotator_get_expander_column:
 * @tree_view: A #Rotator
 *
 * Returns the column that is the current expander column.  This
 * column has the expander arrow drawn next to it.
 *
 * Return value: The expander column.
 **/
RotatorColumn *
rotator_get_expander_column (Rotator *tree_view)
{
  GList *list;

  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), NULL);

  for (list = tree_view->priv->columns; list; list = list->next)
    if (rotator_is_expander_column (tree_view, GTK_ROTATOR_COLUMN (list->data)))
      return (RotatorColumn *) list->data;
  return NULL;
}


/**
 * rotator_set_column_drag_function:
 * @tree_view: A #Rotator.
 * @func: A function to determine which columns are reorderable, or %NULL.
 * @user_data: User data to be passed to @func, or %NULL
 * @destroy: Destroy notifier for @user_data, or %NULL
 *
 * Sets a user function for determining where a column may be dropped when
 * dragged.  This function is called on every column pair in turn at the
 * beginning of a column drag to determine where a drop can take place.  The
 * arguments passed to @func are: the @tree_view, the #RotatorColumn being
 * dragged, the two #RotatorColumn s determining the drop spot, and
 * @user_data.  If either of the #RotatorColumn arguments for the drop spot
 * are %NULL, then they indicate an edge.  If @func is set to be %NULL, then
 * @tree_view reverts to the default behavior of allowing all columns to be
 * dropped everywhere.
 **/
void
rotator_set_column_drag_function (Rotator               *tree_view,
					RotatorColumnDropFunc  func,
					gpointer                   user_data,
					GDestroyNotify             destroy)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (tree_view->priv->column_drop_func_data_destroy)
    tree_view->priv->column_drop_func_data_destroy (tree_view->priv->column_drop_func_data);

  tree_view->priv->column_drop_func = func;
  tree_view->priv->column_drop_func_data = user_data;
  tree_view->priv->column_drop_func_data_destroy = destroy;
}

/**
 * rotator_scroll_to_point:
 * @tree_view: a #Rotator
 * @tree_x: X coordinate of new top-left pixel of visible area, or -1
 * @tree_y: Y coordinate of new top-left pixel of visible area, or -1
 *
 * Scrolls the tree view such that the top-left corner of the visible
 * area is @tree_x, @tree_y, where @tree_x and @tree_y are specified
 * in tree coordinates.  The @tree_view must be realized before
 * this function is called.  If it isn't, you probably want to be
 * using rotator_scroll_to_cell().
 *
 * If either @tree_x or @tree_y are -1, then that direction isn't scrolled.
 **/
void
rotator_scroll_to_point (Rotator *tree_view,
                               gint         tree_x,
                               gint         tree_y)
{
  GtkAdjustment *hadj;
  GtkAdjustment *vadj;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (GTK_WIDGET_REALIZED (tree_view));

  hadj = tree_view->priv->hadjustment;
  vadj = tree_view->priv->vadjustment;

  if (tree_x != -1)
    gtk_adjustment_set_value (hadj, CLAMP (tree_x, hadj->lower, hadj->upper - hadj->page_size));
  if (tree_y != -1)
    gtk_adjustment_set_value (vadj, CLAMP (tree_y, vadj->lower, vadj->upper - vadj->page_size));
}

/**
 * rotator_scroll_to_cell:
 * @tree_view: A #Rotator.
 * @path: The path of the row to move to, or %NULL.
 * @column: The #RotatorColumn to move horizontally to, or %NULL.
 * @use_align: whether to use alignment arguments, or %FALSE.
 * @row_align: The vertical alignment of the row specified by @path.
 * @col_align: The horizontal alignment of the column specified by @column.
 *
 * Moves the alignments of @tree_view to the position specified by @column and
 * @path.  If @column is %NULL, then no horizontal scrolling occurs.  Likewise,
 * if @path is %NULL no vertical scrolling occurs.  At a minimum, one of @column
 * or @path need to be non-%NULL.  @row_align determines where the row is
 * placed, and @col_align determines where @column is placed.  Both are expected
 * to be between 0.0 and 1.0. 0.0 means left/top alignment, 1.0 means
 * right/bottom alignment, 0.5 means center.
 *
 * If @use_align is %FALSE, then the alignment arguments are ignored, and the
 * tree does the minimum amount of work to scroll the cell onto the screen.
 * This means that the cell will be scrolled to the edge closest to its current
 * position.  If the cell is currently visible on the screen, nothing is done.
 *
 * This function only works if the model is set, and @path is a valid row on the
 * model.  If the model changes before the @tree_view is realized, the centered
 * path will be modified to reflect this change.
 **/
void
rotator_scroll_to_cell (Rotator       *tree_view,
                              GtkTreePath       *path,
                              RotatorColumn *column,
			      gboolean           use_align,
                              gfloat             row_align,
                              gfloat             col_align)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (tree_view->priv->model != NULL);
  g_return_if_fail (tree_view->priv->tree != NULL);
  g_return_if_fail (row_align >= 0.0 && row_align <= 1.0);
  g_return_if_fail (col_align >= 0.0 && col_align <= 1.0);
  g_return_if_fail (path != NULL || column != NULL);

#if 0
  g_print ("rotator_scroll_to_cell:\npath: %s\ncolumn: %s\nuse_align: %d\nrow_align: %f\ncol_align: %f\n",
	   gtk_tree_path_to_string (path), column?"non-null":"null", use_align, row_align, col_align);
#endif
  row_align = CLAMP (row_align, 0.0, 1.0);
  col_align = CLAMP (col_align, 0.0, 1.0);


  /* Note: Despite the benefits that come from having one code path for the
   * scrolling code, we short-circuit validate_visible_area's immplementation as
   * it is much slower than just going to the point.
   */
  if (! GTK_WIDGET_VISIBLE (tree_view) ||
      ! GTK_WIDGET_REALIZED (tree_view) ||
      GTK_WIDGET_ALLOC_NEEDED (tree_view) || 
      GTK_RBNODE_FLAG_SET (tree_view->priv->tree->root, GTK_RBNODE_DESCENDANTS_INVALID))
    {
      if (tree_view->priv->scroll_to_path)
	gtk_tree_row_reference_free (tree_view->priv->scroll_to_path);

      tree_view->priv->scroll_to_path = NULL;
      tree_view->priv->scroll_to_column = NULL;

      if (path)
	tree_view->priv->scroll_to_path = gtk_tree_row_reference_new_proxy (G_OBJECT (tree_view), tree_view->priv->model, path);
      if (column)
	tree_view->priv->scroll_to_column = column;
      tree_view->priv->scroll_to_use_align = use_align;
      tree_view->priv->scroll_to_row_align = row_align;
      tree_view->priv->scroll_to_col_align = col_align;

      install_presize_handler (tree_view);
    }
  else
    {
      GdkRectangle cell_rect;
      GdkRectangle vis_rect;
      gint dest_x, dest_y;

      rotator_get_background_area (tree_view, path, column, &cell_rect);
      rotator_get_visible_rect (tree_view, &vis_rect);

      cell_rect.y = TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, cell_rect.y);

      dest_x = vis_rect.x;
      dest_y = vis_rect.y;

      if (column)
	{
	  if (use_align)
	    {
	      dest_x = cell_rect.x - ((vis_rect.width - cell_rect.width) * col_align);
	    }
	  else
	    {
	      if (cell_rect.x < vis_rect.x)
		dest_x = cell_rect.x;
	      if (cell_rect.x + cell_rect.width > vis_rect.x + vis_rect.width)
		dest_x = cell_rect.x + cell_rect.width - vis_rect.width;
	    }
	}

      if (path)
	{
	  if (use_align)
	    {
	      dest_y = cell_rect.y - ((vis_rect.height - cell_rect.height) * row_align);
	      dest_y = MAX (dest_y, 0);
	    }
	  else
	    {
	      if (cell_rect.y < vis_rect.y)
		dest_y = cell_rect.y;
	      if (cell_rect.y + cell_rect.height > vis_rect.y + vis_rect.height)
		dest_y = cell_rect.y + cell_rect.height - vis_rect.height;
	    }
	}

      rotator_scroll_to_point (tree_view, dest_x, dest_y);
    }
}

/**
 * rotator_row_activated:
 * @tree_view: A #Rotator
 * @path: The #GtkTreePath to be activated.
 * @column: The #RotatorColumn to be activated.
 *
 * Activates the cell determined by @path and @column.
 **/
void
rotator_row_activated (Rotator       *tree_view,
			     GtkTreePath       *path,
			     RotatorColumn *column)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  g_signal_emit (tree_view, tree_view_signals[ROW_ACTIVATED], 0, path, column);
}


static void
rotator_expand_all_emission_helper (GtkRBTree *tree,
                                          GtkRBNode *node,
                                          gpointer   data)
{
  Rotator *tree_view = data;

  if ((node->flags & GTK_RBNODE_IS_PARENT) == GTK_RBNODE_IS_PARENT &&
      node->children)
    {
      GtkTreePath *path;
      GtkTreeIter iter;

      path = _rotator_find_path (tree_view, tree, node);
      gtk_tree_model_get_iter (tree_view->priv->model, &iter, path);

      g_signal_emit (tree_view, tree_view_signals[ROW_EXPANDED], 0, &iter, path);

      gtk_tree_path_free (path);
    }

  if (node->children)
    _gtk_rbtree_traverse (node->children,
                          node->children->root,
                          G_PRE_ORDER,
                          rotator_expand_all_emission_helper,
                          tree_view);
}

/**
 * rotator_expand_all:
 * @tree_view: A #Rotator.
 *
 * Recursively expands all nodes in the @tree_view.
 **/
void
rotator_expand_all (Rotator *tree_view)
{
  GtkTreePath *path;
  GtkRBTree *tree;
  GtkRBNode *node;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (tree_view->priv->tree == NULL)
    return;

  path = gtk_tree_path_new_first ();
  _rotator_find_node (tree_view, path, &tree, &node);

  while (node)
    {
      rotator_real_expand_row (tree_view, path, tree, node, TRUE, FALSE);
      node = _gtk_rbtree_next (tree, node);
      gtk_tree_path_next (path);
  }

  gtk_tree_path_free (path);
}

/* Timeout to animate the expander during expands and collapses */
static gboolean
expand_collapse_timeout (gpointer data)
{
  return do_expand_collapse (data);
}

static void
add_expand_collapse_timeout (Rotator *tree_view,
                             GtkRBTree   *tree,
                             GtkRBNode   *node,
                             gboolean     expand)
{
  if (tree_view->priv->expand_collapse_timeout != 0)
    return;

  tree_view->priv->expand_collapse_timeout =
      gdk_threads_add_timeout (50, expand_collapse_timeout, tree_view);
  tree_view->priv->expanded_collapsed_tree = tree;
  tree_view->priv->expanded_collapsed_node = node;

  if (expand)
    GTK_RBNODE_SET_FLAG (node, GTK_RBNODE_IS_SEMI_COLLAPSED);
  else
    GTK_RBNODE_SET_FLAG (node, GTK_RBNODE_IS_SEMI_EXPANDED);
}

static void
remove_expand_collapse_timeout (Rotator *tree_view)
{
  if (tree_view->priv->expand_collapse_timeout)
    {
      g_source_remove (tree_view->priv->expand_collapse_timeout);
      tree_view->priv->expand_collapse_timeout = 0;
    }

  if (tree_view->priv->expanded_collapsed_node != NULL)
    {
      GTK_RBNODE_UNSET_FLAG (tree_view->priv->expanded_collapsed_node, GTK_RBNODE_IS_SEMI_EXPANDED);
      GTK_RBNODE_UNSET_FLAG (tree_view->priv->expanded_collapsed_node, GTK_RBNODE_IS_SEMI_COLLAPSED);

      tree_view->priv->expanded_collapsed_node = NULL;
    }
}

static void
cancel_arrow_animation (Rotator *tree_view)
{
  if (tree_view->priv->expand_collapse_timeout)
    {
      while (do_expand_collapse (tree_view));

      remove_expand_collapse_timeout (tree_view);
    }
}

static gboolean
do_expand_collapse (Rotator *tree_view)
{
  GtkRBNode *node;
  GtkRBTree *tree;
  gboolean expanding;
  gboolean redraw;

  redraw = FALSE;
  expanding = TRUE;

  node = tree_view->priv->expanded_collapsed_node;
  tree = tree_view->priv->expanded_collapsed_tree;

  if (node->children == NULL)
    expanding = FALSE;

  if (expanding)
    {
      if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_SEMI_COLLAPSED))
	{
	  GTK_RBNODE_UNSET_FLAG (node, GTK_RBNODE_IS_SEMI_COLLAPSED);
	  GTK_RBNODE_SET_FLAG (node, GTK_RBNODE_IS_SEMI_EXPANDED);

	  redraw = TRUE;

	}
      else if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_SEMI_EXPANDED))
	{
	  GTK_RBNODE_UNSET_FLAG (node, GTK_RBNODE_IS_SEMI_EXPANDED);

	  redraw = TRUE;
	}
    }
  else
    {
      if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_SEMI_EXPANDED))
	{
	  GTK_RBNODE_UNSET_FLAG (node, GTK_RBNODE_IS_SEMI_EXPANDED);
	  GTK_RBNODE_SET_FLAG (node, GTK_RBNODE_IS_SEMI_COLLAPSED);

	  redraw = TRUE;
	}
      else if (GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_SEMI_COLLAPSED))
	{
	  GTK_RBNODE_UNSET_FLAG (node, GTK_RBNODE_IS_SEMI_COLLAPSED);

	  redraw = TRUE;

	}
    }

  if (redraw)
    {
      rotator_queue_draw_arrow (tree_view, tree, node, NULL);

      return TRUE;
    }

  return FALSE;
}

/**
 * rotator_collapse_all:
 * @tree_view: A #Rotator.
 *
 * Recursively collapses all visible, expanded nodes in @tree_view.
 **/
void
rotator_collapse_all (Rotator *tree_view)
{
  GtkRBTree *tree;
  GtkRBNode *node;
  GtkTreePath *path;
  gint *indices;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (tree_view->priv->tree == NULL)
    return;

  path = gtk_tree_path_new ();
  gtk_tree_path_down (path);
  indices = gtk_tree_path_get_indices (path);

  tree = tree_view->priv->tree;
  node = tree->root;
  while (node && node->left != tree->nil)
    node = node->left;

  while (node)
    {
      if (node->children)
	rotator_real_collapse_row (tree_view, path, tree, node, FALSE);
      indices[0]++;
      node = _gtk_rbtree_next (tree, node);
    }

  gtk_tree_path_free (path);
}

/**
 * rotator_expand_to_path:
 * @tree_view: A #Rotator.
 * @path: path to a row.
 *
 * Expands the row at @path. This will also expand all parent rows of
 * @path as necessary.
 *
 * Since: 2.2
 **/
void
rotator_expand_to_path (Rotator *tree_view,
			      GtkTreePath *path)
{
  gint i, depth;
  gint *indices;
  GtkTreePath *tmp;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (path != NULL);

  depth = gtk_tree_path_get_depth (path);
  indices = gtk_tree_path_get_indices (path);

  tmp = gtk_tree_path_new ();
  g_return_if_fail (tmp != NULL);

  for (i = 0; i < depth; i++)
    {
      gtk_tree_path_append_index (tmp, indices[i]);
      rotator_expand_row (tree_view, tmp, FALSE);
    }

  gtk_tree_path_free (tmp);
}

/* FIXME the bool return values for expand_row and collapse_row are
 * not analagous; they should be TRUE if the row had children and
 * was not already in the requested state.
 */


static gboolean
rotator_real_expand_row (Rotator *tree_view,
			       GtkTreePath *path,
			       GtkRBTree   *tree,
			       GtkRBNode   *node,
			       gboolean     open_all,
			       gboolean     animate)
{
  GtkTreeIter iter;
  GtkTreeIter temp;
  gboolean expand;

  if (animate)
    g_object_get (gtk_widget_get_settings (GTK_WIDGET (tree_view)),
                  "gtk-enable-animations", &animate,
                  NULL);

  remove_auto_expand_timeout (tree_view);

  if (node->children && !open_all)
    return FALSE;

  if (! GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_PARENT))
    return FALSE;

  gtk_tree_model_get_iter (tree_view->priv->model, &iter, path);
  if (! gtk_tree_model_iter_has_child (tree_view->priv->model, &iter))
    return FALSE;


   if (node->children && open_all)
    {
      gboolean retval = FALSE;
      GtkTreePath *tmp_path = gtk_tree_path_copy (path);

      gtk_tree_path_append_index (tmp_path, 0);
      tree = node->children;
      node = tree->root;
      while (node->left != tree->nil)
	node = node->left;
      /* try to expand the children */
      do
        {
         gboolean t;
	 t = rotator_real_expand_row (tree_view, tmp_path, tree, node,
					    TRUE, animate);
         if (t)
           retval = TRUE;

         gtk_tree_path_next (tmp_path);
	 node = _gtk_rbtree_next (tree, node);
       }
      while (node != NULL);

      gtk_tree_path_free (tmp_path);

      return retval;
    }

  g_signal_emit (tree_view, tree_view_signals[TEST_EXPAND_ROW], 0, &iter, path, &expand);

  if (!gtk_tree_model_iter_has_child (tree_view->priv->model, &iter))
    return FALSE;

  if (expand)
    return FALSE;

  node->children = _gtk_rbtree_new ();
  node->children->parent_tree = tree;
  node->children->parent_node = node;

  gtk_tree_model_iter_children (tree_view->priv->model, &temp, &iter);

  rotator_build_tree (tree_view,
			    node->children,
			    &temp,
			    gtk_tree_path_get_depth (path) + 1,
			    open_all);

  remove_expand_collapse_timeout (tree_view);

  if (animate)
    add_expand_collapse_timeout (tree_view, tree, node, TRUE);

  install_presize_handler (tree_view);

  g_signal_emit (tree_view, tree_view_signals[ROW_EXPANDED], 0, &iter, path);
  if (open_all && node->children)
    {
      _gtk_rbtree_traverse (node->children,
                            node->children->root,
                            G_PRE_ORDER,
                            rotator_expand_all_emission_helper,
                            tree_view);
    }
  return TRUE;
}


/**
 * rotator_expand_row:
 * @tree_view: a #Rotator
 * @path: path to a row
 * @open_all: whether to recursively expand, or just expand immediate children
 *
 * Opens the row so its children are visible.
 *
 * Return value: %TRUE if the row existed and had children
 **/
gboolean
rotator_expand_row (Rotator *tree_view,
			  GtkTreePath *path,
			  gboolean     open_all)
{
  GtkRBTree *tree;
  GtkRBNode *node;

  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);
  g_return_val_if_fail (tree_view->priv->model != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  if (_rotator_find_node (tree_view,
				path,
				&tree,
				&node))
    return FALSE;

  if (tree != NULL)
    return rotator_real_expand_row (tree_view, path, tree, node, open_all, FALSE);
  else
    return FALSE;
}

static gboolean
rotator_real_collapse_row (Rotator *tree_view,
				 GtkTreePath *path,
				 GtkRBTree   *tree,
				 GtkRBNode   *node,
				 gboolean     animate)
{
  GtkTreeIter iter;
  GtkTreeIter children;
  gboolean collapse;
  gint x, y;
  GList *list;
  GdkWindow *child, *parent;

  if (animate)
    g_object_get (gtk_widget_get_settings (GTK_WIDGET (tree_view)),
                  "gtk-enable-animations", &animate,
                  NULL);

  remove_auto_expand_timeout (tree_view);

  if (node->children == NULL)
    return FALSE;

  gtk_tree_model_get_iter (tree_view->priv->model, &iter, path);

  g_signal_emit (tree_view, tree_view_signals[TEST_COLLAPSE_ROW], 0, &iter, path, &collapse);

  if (collapse)
    return FALSE;

  /* if the prelighted node is a child of us, we want to unprelight it.  We have
   * a chance to prelight the correct node below */

  if (tree_view->priv->prelight_tree)
    {
      GtkRBTree *parent_tree;
      GtkRBNode *parent_node;

      parent_tree = tree_view->priv->prelight_tree->parent_tree;
      parent_node = tree_view->priv->prelight_tree->parent_node;
      while (parent_tree)
	{
	  if (parent_tree == tree && parent_node == node)
	    {
	      ensure_unprelighted (tree_view);
	      break;
	    }
	  parent_node = parent_tree->parent_node;
	  parent_tree = parent_tree->parent_tree;
	}
    }

  ROTATOR_INTERNAL_ASSERT (gtk_tree_model_iter_children (tree_view->priv->model, &children, &iter), FALSE);

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      RotatorColumn *column = list->data;

      if (column->visible == FALSE)
	continue;
      if (rotator_column_get_sizing (column) == GTK_ROTATOR_COLUMN_AUTOSIZE)
	_rotator_column_cell_set_dirty (column, TRUE);
    }

  if (tree_view->priv->destroy_count_func)
    {
      GtkTreePath *child_path;
      gint child_count = 0;
      child_path = gtk_tree_path_copy (path);
      gtk_tree_path_down (child_path);
      if (node->children)
	_gtk_rbtree_traverse (node->children, node->children->root, G_POST_ORDER, count_children_helper, &child_count);
      tree_view->priv->destroy_count_func (tree_view, child_path, child_count, tree_view->priv->destroy_count_data);
      gtk_tree_path_free (child_path);
    }

  if (gtk_tree_row_reference_valid (tree_view->priv->cursor))
    {
      GtkTreePath *cursor_path = gtk_tree_row_reference_get_path (tree_view->priv->cursor);

      if (gtk_tree_path_is_ancestor (path, cursor_path))
	{
	  gtk_tree_row_reference_free (tree_view->priv->cursor);
	  tree_view->priv->cursor = gtk_tree_row_reference_new_proxy (G_OBJECT (tree_view),
								      tree_view->priv->model,
								      path);
	}
      gtk_tree_path_free (cursor_path);
    }

  if (gtk_tree_row_reference_valid (tree_view->priv->anchor))
    {
      GtkTreePath *anchor_path = gtk_tree_row_reference_get_path (tree_view->priv->anchor);
      if (gtk_tree_path_is_ancestor (path, anchor_path))
	{
	  gtk_tree_row_reference_free (tree_view->priv->anchor);
	  tree_view->priv->anchor = NULL;
	}
      gtk_tree_path_free (anchor_path);
    }

  if (gtk_tree_row_reference_valid (tree_view->priv->last_button_press))
    {
      GtkTreePath *lsc = gtk_tree_row_reference_get_path (tree_view->priv->last_button_press);
      if (gtk_tree_path_is_ancestor (path, lsc))
        {
	  gtk_tree_row_reference_free (tree_view->priv->last_button_press);
	  tree_view->priv->last_button_press = NULL;
	}
      gtk_tree_path_free (lsc);
    }

  if (gtk_tree_row_reference_valid (tree_view->priv->last_button_press_2))
    {
      GtkTreePath *lsc = gtk_tree_row_reference_get_path (tree_view->priv->last_button_press_2);
      if (gtk_tree_path_is_ancestor (path, lsc))
        {
	  gtk_tree_row_reference_free (tree_view->priv->last_button_press_2);
	  tree_view->priv->last_button_press_2 = NULL;
	}
      gtk_tree_path_free (lsc);
    }

  remove_expand_collapse_timeout (tree_view);

  if (rotator_unref_and_check_selection_tree (tree_view, node->children))
    {
      _gtk_rbtree_remove (node->children);
      g_signal_emit_by_name (tree_view->priv->selection, "changed");
    }
  else
    _gtk_rbtree_remove (node->children);
  
  if (animate)
    add_expand_collapse_timeout (tree_view, tree, node, FALSE);
  
  if (GTK_WIDGET_MAPPED (tree_view))
    {
      gtk_widget_queue_resize (GTK_WIDGET (tree_view));
    }

  g_signal_emit (tree_view, tree_view_signals[ROW_COLLAPSED], 0, &iter, path);

  if (GTK_WIDGET_MAPPED (tree_view))
    {
      /* now that we've collapsed all rows, we want to try to set the prelight
       * again. To do this, we fake a motion event and send it to ourselves. */

      child = tree_view->priv->bin_window;
      parent = gdk_window_get_parent (child);

      if (gdk_window_get_pointer (parent, &x, &y, NULL) == child)
	{
	  GdkEventMotion event;
	  gint child_x, child_y;

	  gdk_window_get_position (child, &child_x, &child_y);

	  event.window = tree_view->priv->bin_window;
	  event.x = x - child_x;
	  event.y = y - child_y;

	  /* despite the fact this isn't a real event, I'm almost positive it will
	   * never trigger a drag event.  maybe_drag is the only function that uses
	   * more than just event.x and event.y. */
	  rotator_motion_bin_window (GTK_WIDGET (tree_view), &event);
	}
    }

  return TRUE;
}

/**
 * rotator_collapse_row:
 * @tree_view: a #Rotator
 * @path: path to a row in the @tree_view
 *
 * Collapses a row (hides its child rows, if they exist).
 *
 * Return value: %TRUE if the row was collapsed.
 **/
gboolean
rotator_collapse_row (Rotator *tree_view,
			    GtkTreePath *path)
{
  GtkRBTree *tree;
  GtkRBNode *node;

  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);
  g_return_val_if_fail (tree_view->priv->tree != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  if (_rotator_find_node (tree_view,
				path,
				&tree,
				&node))
    return FALSE;

  if (tree == NULL || node->children == NULL)
    return FALSE;

  return rotator_real_collapse_row (tree_view, path, tree, node, FALSE);
}

static void
rotator_map_expanded_rows_helper (Rotator            *tree_view,
					GtkRBTree              *tree,
					GtkTreePath            *path,
					RotatorMappingFunc  func,
					gpointer                user_data)
{
  GtkRBNode *node;

  if (tree == NULL || tree->root == NULL)
    return;

  node = tree->root;

  while (node && node->left != tree->nil)
    node = node->left;

  while (node)
    {
      if (node->children)
	{
	  (* func) (tree_view, path, user_data);
	  gtk_tree_path_down (path);
	  rotator_map_expanded_rows_helper (tree_view, node->children, path, func, user_data);
	  gtk_tree_path_up (path);
	}
      gtk_tree_path_next (path);
      node = _gtk_rbtree_next (tree, node);
    }
}

/**
 * rotator_map_expanded_rows:
 * @tree_view: A #Rotator
 * @func: A function to be called
 * @data: User data to be passed to the function.
 *
 * Calls @func on all expanded rows.
 **/
void
rotator_map_expanded_rows (Rotator            *tree_view,
				 RotatorMappingFunc  func,
				 gpointer                user_data)
{
  GtkTreePath *path;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (func != NULL);

  path = gtk_tree_path_new_first ();

  rotator_map_expanded_rows_helper (tree_view,
					  tree_view->priv->tree,
					  path, func, user_data);

  gtk_tree_path_free (path);
}

/**
 * rotator_row_expanded:
 * @tree_view: A #Rotator.
 * @path: A #GtkTreePath to test expansion state.
 *
 * Returns %TRUE if the node pointed to by @path is expanded in @tree_view.
 *
 * Return value: %TRUE if #path is expanded.
 **/
gboolean
rotator_row_expanded (Rotator *tree_view,
			    GtkTreePath *path)
{
  GtkRBTree *tree;
  GtkRBNode *node;

  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  _rotator_find_node (tree_view, path, &tree, &node);

  if (node == NULL)
    return FALSE;

  return (node->children != NULL);
}

/**
 * rotator_get_reorderable:
 * @tree_view: a #Rotator
 *
 * Retrieves whether the user can reorder the tree via drag-and-drop. See
 * rotator_set_reorderable().
 *
 * Return value: %TRUE if the tree can be reordered.
 **/
gboolean
rotator_get_reorderable (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);

  return tree_view->priv->reorderable;
}

/**
 * rotator_set_reorderable:
 * @tree_view: A #Rotator.
 * @reorderable: %TRUE, if the tree can be reordered.
 *
 * This function is a convenience function to allow you to reorder
 * models that support the #GtkDragSourceIface and the
 * #GtkDragDestIface.  Both #GtkTreeStore and #GtkListStore support
 * these.  If @reorderable is %TRUE, then the user can reorder the
 * model by dragging and dropping rows. The developer can listen to
 * these changes by connecting to the model's row_inserted and
 * row_deleted signals. The reordering is implemented by setting up
 * the tree view as a drag source and destination. Therefore, drag and
 * drop can not be used in a reorderable view for any other purpose.
 *
 * This function does not give you any degree of control over the order -- any
 * reordering is allowed.  If more control is needed, you should probably
 * handle drag and drop manually.
 **/
void
rotator_set_reorderable (Rotator *tree_view,
			       gboolean     reorderable)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  reorderable = reorderable != FALSE;

  if (tree_view->priv->reorderable == reorderable)
    return;

  if (reorderable)
    {
      const GtkTargetEntry row_targets[] = {
        { "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, 0 }
      };

      rotator_enable_model_drag_source (tree_view,
					      GDK_BUTTON1_MASK,
					      row_targets,
					      G_N_ELEMENTS (row_targets),
					      GDK_ACTION_MOVE);
      rotator_enable_model_drag_dest (tree_view,
					    row_targets,
					    G_N_ELEMENTS (row_targets),
					    GDK_ACTION_MOVE);
    }
  else
    {
      rotator_unset_rows_drag_source (tree_view);
      rotator_unset_rows_drag_dest (tree_view);
    }

  tree_view->priv->reorderable = reorderable;

  g_object_notify (G_OBJECT (tree_view), "reorderable");
}

static void
rotator_real_set_cursor (Rotator     *tree_view,
			       GtkTreePath     *path,
			       gboolean         clear_and_select,
			       gboolean         clamp_node)
{
  GtkRBTree *tree = NULL;
  GtkRBNode *node = NULL;

  if (gtk_tree_row_reference_valid (tree_view->priv->cursor))
    {
      GtkTreePath *cursor_path;
      cursor_path = gtk_tree_row_reference_get_path (tree_view->priv->cursor);
      rotator_queue_draw_path (tree_view, cursor_path, NULL);
      gtk_tree_path_free (cursor_path);
    }

  gtk_tree_row_reference_free (tree_view->priv->cursor);
  tree_view->priv->cursor = NULL;

  /* One cannot set the cursor on a separator.   Also, if
   * _rotator_find_node returns TRUE, it ran out of tree
   * before finding the tree and node belonging to path.  The
   * path maps to a non-existing path and we will silently bail out.
   * We unset tree and node to avoid further processing.
   */
  if (!row_is_separator (tree_view, NULL, path)
      && _rotator_find_node (tree_view, path, &tree, &node) == FALSE)
    {
      tree_view->priv->cursor =
          gtk_tree_row_reference_new_proxy (G_OBJECT (tree_view),
                                            tree_view->priv->model,
                                            path);
    }
  else
    {
      tree = NULL;
      node = NULL;
    }

  if (tree != NULL)
    {
      GtkRBTree *new_tree = NULL;
      GtkRBNode *new_node = NULL;

      if (clear_and_select && !tree_view->priv->ctrl_pressed)
        {
          GtkTreeSelectMode mode = 0;

          if (tree_view->priv->ctrl_pressed)
            mode |= GTK_TREE_SELECT_MODE_TOGGLE;
          if (tree_view->priv->shift_pressed)
            mode |= GTK_TREE_SELECT_MODE_EXTEND;

          _gtk_tree_selection_internal_select_node (tree_view->priv->selection,
                                                    node, tree, path, mode,
                                                    FALSE);
        }

      /* We have to re-find tree and node here again, somebody might have
       * cleared the node or the whole tree in the GtkTreeSelection::changed
       * callback. If the nodes differ we bail out here.
       */
      _rotator_find_node (tree_view, path, &new_tree, &new_node);

      if (tree != new_tree || node != new_node)
        return;

      if (clamp_node)
        {
	  rotator_clamp_node_visible (tree_view, tree, node);
	  _rotator_queue_draw_node (tree_view, tree, node, NULL);
	}
    }

  g_signal_emit (tree_view, tree_view_signals[CURSOR_CHANGED], 0);
}

/**
 * rotator_get_cursor:
 * @tree_view: A #Rotator
 * @path: A pointer to be filled with the current cursor path, or %NULL
 * @focus_column: A pointer to be filled with the current focus column, or %NULL
 *
 * Fills in @path and @focus_column with the current path and focus column.  If
 * the cursor isn't currently set, then *@path will be %NULL.  If no column
 * currently has focus, then *@focus_column will be %NULL.
 *
 * The returned #GtkTreePath must be freed with gtk_tree_path_free() when
 * you are done with it.
 **/
void
rotator_get_cursor (Rotator        *tree_view,
			  GtkTreePath       **path,
			  RotatorColumn **focus_column)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (path)
    {
      if (gtk_tree_row_reference_valid (tree_view->priv->cursor))
	*path = gtk_tree_row_reference_get_path (tree_view->priv->cursor);
      else
	*path = NULL;
    }

  if (focus_column)
    {
      *focus_column = tree_view->priv->focus_column;
    }
}

/**
 * rotator_set_cursor:
 * @tree_view: A #Rotator
 * @path: A #GtkTreePath
 * @focus_column: A #RotatorColumn, or %NULL
 * @start_editing: %TRUE if the specified cell should start being edited.
 *
 * Sets the current keyboard focus to be at @path, and selects it.  This is
 * useful when you want to focus the user's attention on a particular row.  If
 * @focus_column is not %NULL, then focus is given to the column specified by 
 * it. Additionally, if @focus_column is specified, and @start_editing is 
 * %TRUE, then editing should be started in the specified cell.  
 * This function is often followed by @gtk_widget_grab_focus (@tree_view) 
 * in order to give keyboard focus to the widget.  Please note that editing 
 * can only happen when the widget is realized.
 **/
void
rotator_set_cursor (Rotator       *tree_view,
			  GtkTreePath       *path,
			  RotatorColumn *focus_column,
			  gboolean           start_editing)
{
  rotator_set_cursor_on_cell (tree_view, path, focus_column,
				    NULL, start_editing);
}

/**
 * rotator_set_cursor_on_cell:
 * @tree_view: A #Rotator
 * @path: A #GtkTreePath
 * @focus_column: A #RotatorColumn, or %NULL
 * @focus_cell: A #GtkCellRenderer, or %NULL
 * @start_editing: %TRUE if the specified cell should start being edited.
 *
 * Sets the current keyboard focus to be at @path, and selects it.  This is
 * useful when you want to focus the user's attention on a particular row.  If
 * @focus_column is not %NULL, then focus is given to the column specified by
 * it. If @focus_column and @focus_cell are not %NULL, and @focus_column
 * contains 2 or more editable or activatable cells, then focus is given to
 * the cell specified by @focus_cell. Additionally, if @focus_column is
 * specified, and @start_editing is %TRUE, then editing should be started in
 * the specified cell.  This function is often followed by
 * @gtk_widget_grab_focus (@tree_view) in order to give keyboard focus to the
 * widget.  Please note that editing can only happen when the widget is
 * realized.
 *
 * Since: 2.2
 **/
void
rotator_set_cursor_on_cell (Rotator       *tree_view,
				  GtkTreePath       *path,
				  RotatorColumn *focus_column,
				  GtkCellRenderer   *focus_cell,
				  gboolean           start_editing)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (tree_view->priv->tree != NULL);
  g_return_if_fail (path != NULL);
  if (focus_column)
    g_return_if_fail (GTK_IS_ROTATOR_COLUMN (focus_column));
  if (focus_cell)
    {
      g_return_if_fail (focus_column);
      g_return_if_fail (GTK_IS_CELL_RENDERER (focus_cell));
    }

  /* cancel the current editing, if it exists */
  if (tree_view->priv->edited_column &&
      tree_view->priv->edited_column->editable_widget)
    rotator_stop_editing (tree_view, TRUE);

  rotator_real_set_cursor (tree_view, path, TRUE, TRUE);

  if (focus_column && focus_column->visible)
    {
      GList *list;
      gboolean column_in_tree = FALSE;

      for (list = tree_view->priv->columns; list; list = list->next)
	if (list->data == focus_column)
	  {
	    column_in_tree = TRUE;
	    break;
	  }
      g_return_if_fail (column_in_tree);
      tree_view->priv->focus_column = focus_column;
      if (focus_cell)
	rotator_column_focus_cell (focus_column, focus_cell);
      if (start_editing)
	rotator_start_editing (tree_view, path);
    }
}

/**
 * rotator_get_bin_window:
 * @tree_view: A #Rotator
 * 
 * Returns the window that @tree_view renders to.  This is used primarily to
 * compare to <literal>event->window</literal> to confirm that the event on
 * @tree_view is on the right window.
 * 
 * Return value: A #GdkWindow, or %NULL when @tree_view hasn't been realized yet
 **/
GdkWindow *
rotator_get_bin_window (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), NULL);

  return tree_view->priv->bin_window;
}

/**
 * rotator_get_path_at_pos:
 * @tree_view: A #Rotator.
 * @x: The x position to be identified (relative to bin_window).
 * @y: The y position to be identified (relative to bin_window).
 * @path: A pointer to a #GtkTreePath pointer to be filled in, or %NULL
 * @column: A pointer to a #RotatorColumn pointer to be filled in, or %NULL
 * @cell_x: A pointer where the X coordinate relative to the cell can be placed, or %NULL
 * @cell_y: A pointer where the Y coordinate relative to the cell can be placed, or %NULL
 *
 * Finds the path at the point (@x, @y), relative to bin_window coordinates
 * (please see rotator_get_bin_window()).
 * That is, @x and @y are relative to an events coordinates. @x and @y must
 * come from an event on the @tree_view only where <literal>event->window ==
 * rotator_get_bin_window (<!-- -->)</literal>. It is primarily for
 * things like popup menus. If @path is non-%NULL, then it will be filled
 * with the #GtkTreePath at that point.  This path should be freed with
 * gtk_tree_path_free().  If @column is non-%NULL, then it will be filled
 * with the column at that point.  @cell_x and @cell_y return the coordinates
 * relative to the cell background (i.e. the @background_area passed to
 * gtk_cell_renderer_render()).  This function is only meaningful if
 * @tree_view is realized.
 *
 * For converting widget coordinates (eg. the ones you get from
 * GtkWidget::query-tooltip), please see
 * rotator_convert_widget_to_bin_window_coords().
 *
 * Return value: %TRUE if a row exists at that coordinate.
 **/
gboolean
rotator_get_path_at_pos (Rotator        *tree_view,
			       gint                x,
			       gint                y,
			       GtkTreePath       **path,
			       RotatorColumn **column,
                               gint               *cell_x,
                               gint               *cell_y)
{
  GtkRBTree *tree;
  GtkRBNode *node;
  gint y_offset;

  g_return_val_if_fail (tree_view != NULL, FALSE);
  g_return_val_if_fail (tree_view->priv->bin_window != NULL, FALSE);

  if (path)
    *path = NULL;
  if (column)
    *column = NULL;

  if (tree_view->priv->tree == NULL)
    return FALSE;

  if (x > tree_view->priv->hadjustment->upper)
    return FALSE;

  if (x < 0 || y < 0)
    return FALSE;

  if (column || cell_x)
    {
      RotatorColumn *tmp_column;
      RotatorColumn *last_column = NULL;
      GList *list;
      gint remaining_x = x;
      gboolean found = FALSE;
      gboolean rtl;

      rtl = (gtk_widget_get_direction (GTK_WIDGET (tree_view)) == GTK_TEXT_DIR_RTL);
      for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
	   list;
	   list = (rtl ? list->prev : list->next))
	{
	  tmp_column = list->data;

	  if (tmp_column->visible == FALSE)
	    continue;

	  last_column = tmp_column;
	  if (remaining_x <= tmp_column->width)
	    {
              found = TRUE;

              if (column)
                *column = tmp_column;

              if (cell_x)
                *cell_x = remaining_x;

	      break;
	    }
	  remaining_x -= tmp_column->width;
	}

      /* If found is FALSE and there is a last_column, then it the remainder
       * space is in that area
       */
      if (!found)
        {
	  if (last_column)
	    {
	      if (column)
		*column = last_column;
	      
	      if (cell_x)
		*cell_x = last_column->width + remaining_x;
	    }
	  else
	    {
	      return FALSE;
	    }
	}
    }

  y_offset = _gtk_rbtree_find_offset (tree_view->priv->tree,
				      TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, y),
				      &tree, &node);

  if (tree == NULL)
    return FALSE;

  if (cell_y)
    *cell_y = y_offset;

  if (path)
    *path = _rotator_find_path (tree_view, tree, node);

  return TRUE;
}


/**
 * rotator_get_cell_area:
 * @tree_view: a #Rotator
 * @path: a #GtkTreePath for the row, or %NULL to get only horizontal coordinates
 * @column: a #RotatorColumn for the column, or %NULL to get only vertical coordinates
 * @rect: rectangle to fill with cell rect
 *
 * Fills the bounding rectangle in bin_window coordinates for the cell at the
 * row specified by @path and the column specified by @column.  If @path is
 * %NULL, or points to a path not currently displayed, the @y and @height fields
 * of the rectangle will be filled with 0. If @column is %NULL, the @x and @width
 * fields will be filled with 0.  The sum of all cell rects does not cover the
 * entire tree; there are extra pixels in between rows, for example. The
 * returned rectangle is equivalent to the @cell_area passed to
 * gtk_cell_renderer_render().  This function is only valid if @tree_view is
 * realized.
 **/
void
rotator_get_cell_area (Rotator        *tree_view,
                             GtkTreePath        *path,
                             RotatorColumn  *column,
                             GdkRectangle       *rect)
{
  GtkRBTree *tree = NULL;
  GtkRBNode *node = NULL;
  gint vertical_separator;
  gint horizontal_separator;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (column == NULL || GTK_IS_ROTATOR_COLUMN (column));
  g_return_if_fail (rect != NULL);
  g_return_if_fail (!column || column->tree_view == (GtkWidget *) tree_view);
  g_return_if_fail (GTK_WIDGET_REALIZED (tree_view));

  gtk_widget_style_get (GTK_WIDGET (tree_view),
			"vertical-separator", &vertical_separator,
			"horizontal-separator", &horizontal_separator,
			NULL);

  rect->x = 0;
  rect->y = 0;
  rect->width = 0;
  rect->height = 0;

  if (column)
    {
      rect->x = column->button->allocation.x + horizontal_separator/2;
      rect->width = column->button->allocation.width - horizontal_separator;
    }

  if (path)
    {
      gboolean ret = _rotator_find_node (tree_view, path, &tree, &node);

      /* Get vertical coords */
      if ((!ret && tree == NULL) || ret)
	return;

      rect->y = CELL_FIRST_PIXEL (tree_view, tree, node, vertical_separator);
      rect->height = MAX (CELL_HEIGHT (node, vertical_separator), tree_view->priv->expander_size - vertical_separator);

      if (column &&
	  rotator_is_expander_column (tree_view, column))
	{
	  gint depth = gtk_tree_path_get_depth (path);
	  gboolean rtl;

	  rtl = gtk_widget_get_direction (GTK_WIDGET (tree_view)) == GTK_TEXT_DIR_RTL;

	  if (!rtl)
	    rect->x += (depth - 1) * tree_view->priv->level_indentation;
	  rect->width -= (depth - 1) * tree_view->priv->level_indentation;

	  if (ROTATOR_DRAW_EXPANDERS (tree_view))
	    {
	      if (!rtl)
		rect->x += depth * tree_view->priv->expander_size;
	      rect->width -= depth * tree_view->priv->expander_size;
	    }

	  rect->width = MAX (rect->width, 0);
	}
    }
}

/**
 * rotator_get_background_area:
 * @tree_view: a #Rotator
 * @path: a #GtkTreePath for the row, or %NULL to get only horizontal coordinates
 * @column: a #RotatorColumn for the column, or %NULL to get only vertical coordiantes
 * @rect: rectangle to fill with cell background rect
 *
 * Fills the bounding rectangle in bin_window coordinates for the cell at the
 * row specified by @path and the column specified by @column.  If @path is
 * %NULL, or points to a node not found in the tree, the @y and @height fields of
 * the rectangle will be filled with 0. If @column is %NULL, the @x and @width
 * fields will be filled with 0.  The returned rectangle is equivalent to the
 * @background_area passed to gtk_cell_renderer_render().  These background
 * areas tile to cover the entire bin window.  Contrast with the @cell_area,
 * returned by rotator_get_cell_area(), which returns only the cell
 * itself, excluding surrounding borders and the tree expander area.
 *
 **/
void
rotator_get_background_area (Rotator        *tree_view,
                                   GtkTreePath        *path,
                                   RotatorColumn  *column,
                                   GdkRectangle       *rect)
{
  GtkRBTree *tree = NULL;
  GtkRBNode *node = NULL;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (column == NULL || GTK_IS_ROTATOR_COLUMN (column));
  g_return_if_fail (rect != NULL);

  rect->x = 0;
  rect->y = 0;
  rect->width = 0;
  rect->height = 0;

  if (path)
    {
      /* Get vertical coords */

      if (!_rotator_find_node (tree_view, path, &tree, &node) &&
	  tree == NULL)
	return;

      rect->y = BACKGROUND_FIRST_PIXEL (tree_view, tree, node);

      rect->height = ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node));
    }

  if (column)
    {
      gint x2 = 0;

      rotator_get_background_xrange (tree_view, tree, column, &rect->x, &x2);
      rect->width = x2 - rect->x;
    }
}

/**
 * rotator_get_visible_rect:
 * @tree_view: a #Rotator
 * @visible_rect: rectangle to fill
 *
 * Fills @visible_rect with the currently-visible region of the
 * buffer, in tree coordinates. Convert to bin_window coordinates with
 * rotator_convert_tree_to_bin_window_coords().
 * Tree coordinates start at 0,0 for row 0 of the tree, and cover the entire
 * scrollable area of the tree.
 **/
void
rotator_get_visible_rect (Rotator  *tree_view,
                                GdkRectangle *visible_rect)
{
  GtkWidget *widget;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  widget = GTK_WIDGET (tree_view);

  if (visible_rect)
    {
      visible_rect->x = tree_view->priv->hadjustment->value;
      visible_rect->y = tree_view->priv->vadjustment->value;
      visible_rect->width = widget->allocation.width;
      visible_rect->height = widget->allocation.height - ROTATOR_HEADER_HEIGHT (tree_view);
    }
}

/**
 * rotator_widget_to_tree_coords:
 * @tree_view: a #Rotator
 * @wx: X coordinate relative to bin_window
 * @wy: Y coordinate relative to bin_window
 * @tx: return location for tree X coordinate
 * @ty: return location for tree Y coordinate
 *
 * Converts bin_window coordinates to coordinates for the
 * tree (the full scrollable area of the tree).
 *
 * Deprecated: 2.12: Due to historial reasons the name of this function is
 * incorrect.  For converting coordinates relative to the widget to
 * bin_window coordinates, please see
 * rotator_convert_widget_to_bin_window_coords().
 *
 **/
void
rotator_widget_to_tree_coords (Rotator *tree_view,
				      gint         wx,
				      gint         wy,
				      gint        *tx,
				      gint        *ty)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (tx)
    *tx = wx + tree_view->priv->hadjustment->value;
  if (ty)
    *ty = wy + tree_view->priv->dy;
}

/**
 * rotator_tree_to_widget_coords:
 * @tree_view: a #Rotator
 * @tx: tree X coordinate
 * @ty: tree Y coordinate
 * @wx: return location for X coordinate relative to bin_window
 * @wy: return location for Y coordinate relative to bin_window
 *
 * Converts tree coordinates (coordinates in full scrollable area of the tree)
 * to bin_window coordinates.
 *
 * Deprecated: 2.12: Due to historial reasons the name of this function is
 * incorrect.  For converting bin_window coordinates to coordinates relative
 * to bin_window, please see
 * rotator_convert_bin_window_to_widget_coords().
 *
 **/
void
rotator_tree_to_widget_coords (Rotator *tree_view,
                                     gint         tx,
                                     gint         ty,
                                     gint        *wx,
                                     gint        *wy)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (wx)
    *wx = tx - tree_view->priv->hadjustment->value;
  if (wy)
    *wy = ty - tree_view->priv->dy;
}


/**
 * rotator_convert_widget_to_tree_coords:
 * @tree_view: a #Rotator
 * @wx: X coordinate relative to the widget
 * @wy: Y coordinate relative to the widget
 * @tx: return location for tree X coordinate
 * @ty: return location for tree Y coordinate
 *
 * Converts widget coordinates to coordinates for the
 * tree (the full scrollable area of the tree).
 *
 * Since: 2.12
 **/
void
rotator_convert_widget_to_tree_coords (Rotator *tree_view,
                                             gint         wx,
                                             gint         wy,
                                             gint        *tx,
                                             gint        *ty)
{
  gint x, y;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  rotator_convert_widget_to_bin_window_coords (tree_view,
						     wx, wy,
						     &x, &y);
  rotator_convert_bin_window_to_tree_coords (tree_view,
						   x, y,
						   tx, ty);
}

/**
 * rotator_convert_tree_to_widget_coords:
 * @tree_view: a #Rotator
 * @tx: X coordinate relative to the tree
 * @ty: Y coordinate relative to the tree
 * @wx: return location for widget X coordinate
 * @wy: return location for widget Y coordinate
 *
 * Converts tree coordinates (coordinates in full scrollable area of the tree)
 * to widget coordinates.
 *
 * Since: 2.12
 **/
void
rotator_convert_tree_to_widget_coords (Rotator *tree_view,
                                             gint         tx,
                                             gint         ty,
                                             gint        *wx,
                                             gint        *wy)
{
  gint x, y;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  rotator_convert_tree_to_bin_window_coords (tree_view,
						   tx, ty,
						   &x, &y);
  rotator_convert_bin_window_to_widget_coords (tree_view,
						     x, y,
						     wx, wy);
}

/**
 * rotator_convert_widget_to_bin_window_coords:
 * @tree_view: a #Rotator
 * @wx: X coordinate relative to the widget
 * @wy: Y coordinate relative to the widget
 * @bx: return location for bin_window X coordinate
 * @by: return location for bin_window Y coordinate
 *
 * Converts widget coordinates to coordinates for the bin_window
 * (see rotator_get_bin_window()).
 *
 * Since: 2.12
 **/
void
rotator_convert_widget_to_bin_window_coords (Rotator *tree_view,
                                                   gint         wx,
                                                   gint         wy,
                                                   gint        *bx,
                                                   gint        *by)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (bx)
    *bx = wx + tree_view->priv->hadjustment->value;
  if (by)
    *by = wy - ROTATOR_HEADER_HEIGHT (tree_view);
}

/**
 * rotator_convert_bin_window_to_widget_coords:
 * @tree_view: a #Rotator
 * @bx: bin_window X coordinate
 * @by: bin_window Y coordinate
 * @wx: return location for widget X coordinate
 * @wy: return location for widget Y coordinate
 *
 * Converts bin_window coordinates (see rotator_get_bin_window())
 * to widget relative coordinates.
 *
 * Since: 2.12
 **/
void
rotator_convert_bin_window_to_widget_coords (Rotator *tree_view,
                                                   gint         bx,
                                                   gint         by,
                                                   gint        *wx,
                                                   gint        *wy)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (wx)
    *wx = bx - tree_view->priv->hadjustment->value;
  if (wy)
    *wy = by + ROTATOR_HEADER_HEIGHT (tree_view);
}

/**
 * rotator_convert_tree_to_bin_window_coords:
 * @tree_view: a #Rotator
 * @tx: tree X coordinate
 * @ty: tree Y coordinate
 * @bx: return location for X coordinate relative to bin_window
 * @by: return location for Y coordinate relative to bin_window
 *
 * Converts tree coordinates (coordinates in full scrollable area of the tree)
 * to bin_window coordinates.
 *
 * Since: 2.12
 **/
void
rotator_convert_tree_to_bin_window_coords (Rotator *tree_view,
                                                 gint         tx,
                                                 gint         ty,
                                                 gint        *bx,
                                                 gint        *by)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (bx)
    *bx = tx;
  if (by)
    *by = ty - tree_view->priv->dy;
}

/**
 * rotator_convert_bin_window_to_tree_coords:
 * @tree_view: a #Rotator
 * @bx: X coordinate relative to bin_window
 * @by: Y coordinate relative to bin_window
 * @tx: return location for tree X coordinate
 * @ty: return location for tree Y coordinate
 *
 * Converts bin_window coordinates to coordinates for the
 * tree (the full scrollable area of the tree).
 *
 * Since: 2.12
 **/
void
rotator_convert_bin_window_to_tree_coords (Rotator *tree_view,
                                                 gint         bx,
                                                 gint         by,
                                                 gint        *tx,
                                                 gint        *ty)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (tx)
    *tx = bx;
  if (ty)
    *ty = by + tree_view->priv->dy;
}



/**
 * rotator_get_visible_range:
 * @tree_view: A #Rotator
 * @start_path: Return location for start of region, or %NULL.
 * @end_path: Return location for end of region, or %NULL.
 *
 * Sets @start_path and @end_path to be the first and last visible path.
 * Note that there may be invisible paths in between.
 *
 * The paths should be freed with gtk_tree_path_free() after use.
 *
 * Returns: %TRUE, if valid paths were placed in @start_path and @end_path.
 *
 * Since: 2.8
 **/
gboolean
rotator_get_visible_range (Rotator  *tree_view,
                                 GtkTreePath **start_path,
                                 GtkTreePath **end_path)
{
  GtkRBTree *tree;
  GtkRBNode *node;
  gboolean retval;
  
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);

  if (!tree_view->priv->tree)
    return FALSE;

  retval = TRUE;

  if (start_path)
    {
      _gtk_rbtree_find_offset (tree_view->priv->tree,
                               TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, 0),
                               &tree, &node);
      if (node)
        *start_path = _rotator_find_path (tree_view, tree, node);
      else
        retval = FALSE;
    }

  if (end_path)
    {
      gint y;

      if (tree_view->priv->height < tree_view->priv->vadjustment->page_size)
        y = tree_view->priv->height - 1;
      else
        y = TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, tree_view->priv->vadjustment->page_size) - 1;

      _gtk_rbtree_find_offset (tree_view->priv->tree, y, &tree, &node);
      if (node)
        *end_path = _rotator_find_path (tree_view, tree, node);
      else
        retval = FALSE;
    }

  return retval;
}

static void
unset_reorderable (Rotator *tree_view)
{
  if (tree_view->priv->reorderable)
    {
      tree_view->priv->reorderable = FALSE;
      g_object_notify (G_OBJECT (tree_view), "reorderable");
    }
}

/**
 * rotator_enable_model_drag_source:
 * @tree_view: a #Rotator
 * @start_button_mask: Mask of allowed buttons to start drag
 * @targets: the table of targets that the drag will support
 * @n_targets: the number of items in @targets
 * @actions: the bitmask of possible actions for a drag from this
 *    widget
 *
 * Turns @tree_view into a drag source for automatic DND. Calling this
 * method sets #Rotator:reorderable to %FALSE.
 **/
void
rotator_enable_model_drag_source (Rotator              *tree_view,
					GdkModifierType           start_button_mask,
					const GtkTargetEntry     *targets,
					gint                      n_targets,
					GdkDragAction             actions)
{
  TreeViewDragInfo *di;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  gtk_drag_source_set (GTK_WIDGET (tree_view),
		       0,
		       targets,
		       n_targets,
		       actions);

  di = ensure_info (tree_view);

  di->start_button_mask = start_button_mask;
  di->source_actions = actions;
  di->source_set = TRUE;

  unset_reorderable (tree_view);
}

/**
 * rotator_enable_model_drag_dest:
 * @tree_view: a #Rotator
 * @targets: the table of targets that the drag will support
 * @n_targets: the number of items in @targets
 * @actions: the bitmask of possible actions for a drag from this
 *    widget
 * 
 * Turns @tree_view into a drop destination for automatic DND. Calling
 * this method sets #Rotator:reorderable to %FALSE.
 **/
void
rotator_enable_model_drag_dest (Rotator              *tree_view,
				      const GtkTargetEntry     *targets,
				      gint                      n_targets,
				      GdkDragAction             actions)
{
  TreeViewDragInfo *di;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  gtk_drag_dest_set (GTK_WIDGET (tree_view),
                     0,
                     targets,
                     n_targets,
                     actions);

  di = ensure_info (tree_view);
  di->dest_set = TRUE;

  unset_reorderable (tree_view);
}

/**
 * rotator_unset_rows_drag_source:
 * @tree_view: a #Rotator
 *
 * Undoes the effect of
 * rotator_enable_model_drag_source(). Calling this method sets
 * #Rotator:reorderable to %FALSE.
 **/
void
rotator_unset_rows_drag_source (Rotator *tree_view)
{
  TreeViewDragInfo *di;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  di = get_info (tree_view);

  if (di)
    {
      if (di->source_set)
        {
          gtk_drag_source_unset (GTK_WIDGET (tree_view));
          di->source_set = FALSE;
        }

      if (!di->dest_set && !di->source_set)
        remove_info (tree_view);
    }
  
  unset_reorderable (tree_view);
}

/**
 * rotator_unset_rows_drag_dest:
 * @tree_view: a #Rotator
 *
 * Undoes the effect of
 * rotator_enable_model_drag_dest(). Calling this method sets
 * #Rotator:reorderable to %FALSE.
 **/
void
rotator_unset_rows_drag_dest (Rotator *tree_view)
{
  TreeViewDragInfo *di;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  di = get_info (tree_view);

  if (di)
    {
      if (di->dest_set)
        {
          gtk_drag_dest_unset (GTK_WIDGET (tree_view));
          di->dest_set = FALSE;
        }

      if (!di->dest_set && !di->source_set)
        remove_info (tree_view);
    }

  unset_reorderable (tree_view);
}

/**
 * rotator_set_drag_dest_row:
 * @tree_view: a #Rotator
 * @path: The path of the row to highlight, or %NULL.
 * @pos: Specifies whether to drop before, after or into the row
 * 
 * Sets the row that is highlighted for feedback.
 **/
void
rotator_set_drag_dest_row (Rotator            *tree_view,
                                 GtkTreePath            *path,
                                 RotatorDropPosition pos)
{
  GtkTreePath *current_dest;

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  current_dest = NULL;

  if (tree_view->priv->drag_dest_row)
    {
      current_dest = gtk_tree_row_reference_get_path (tree_view->priv->drag_dest_row);
      gtk_tree_row_reference_free (tree_view->priv->drag_dest_row);
    }

  /* special case a drop on an empty model */
  tree_view->priv->empty_view_drop = 0;

  if (pos == GTK_ROTATOR_DROP_BEFORE && path
      && gtk_tree_path_get_depth (path) == 1
      && gtk_tree_path_get_indices (path)[0] == 0)
    {
      gint n_children;

      n_children = gtk_tree_model_iter_n_children (tree_view->priv->model,
                                                   NULL);

      if (!n_children)
        tree_view->priv->empty_view_drop = 1;
    }

  tree_view->priv->drag_dest_pos = pos;

  if (path)
    {
      tree_view->priv->drag_dest_row =
        gtk_tree_row_reference_new_proxy (G_OBJECT (tree_view), tree_view->priv->model, path);
      rotator_queue_draw_path (tree_view, path, NULL);
    }
  else
    tree_view->priv->drag_dest_row = NULL;

  if (current_dest)
    {
      GtkRBTree *tree, *new_tree;
      GtkRBNode *node, *new_node;

      _rotator_find_node (tree_view, current_dest, &tree, &node);
      _rotator_queue_draw_node (tree_view, tree, node, NULL);

      if (tree && node)
	{
	  _gtk_rbtree_next_full (tree, node, &new_tree, &new_node);
	  if (new_tree && new_node)
	    _rotator_queue_draw_node (tree_view, new_tree, new_node, NULL);

	  _gtk_rbtree_prev_full (tree, node, &new_tree, &new_node);
	  if (new_tree && new_node)
	    _rotator_queue_draw_node (tree_view, new_tree, new_node, NULL);
	}
      gtk_tree_path_free (current_dest);
    }
}

/**
 * rotator_get_drag_dest_row:
 * @tree_view: a #Rotator
 * @path: Return location for the path of the highlighted row, or %NULL.
 * @pos: Return location for the drop position, or %NULL
 * 
 * Gets information about the row that is highlighted for feedback.
 **/
void
rotator_get_drag_dest_row (Rotator              *tree_view,
                                 GtkTreePath             **path,
                                 RotatorDropPosition  *pos)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (path)
    {
      if (tree_view->priv->drag_dest_row)
        *path = gtk_tree_row_reference_get_path (tree_view->priv->drag_dest_row);
      else
        {
          if (tree_view->priv->empty_view_drop)
            *path = gtk_tree_path_new_from_indices (0, -1);
          else
            *path = NULL;
        }
    }

  if (pos)
    *pos = tree_view->priv->drag_dest_pos;
}

/**
 * rotator_get_dest_row_at_pos:
 * @tree_view: a #Rotator
 * @drag_x: the position to determine the destination row for
 * @drag_y: the position to determine the destination row for
 * @path: Return location for the path of the highlighted row, or %NULL.
 * @pos: Return location for the drop position, or %NULL
 * 
 * Determines the destination row for a given position.  @drag_x and
 * @drag_y are expected to be in widget coordinates.
 * 
 * Return value: whether there is a row at the given position.
 **/
gboolean
rotator_get_dest_row_at_pos (Rotator             *tree_view,
                                   gint                     drag_x,
                                   gint                     drag_y,
                                   GtkTreePath            **path,
                                   RotatorDropPosition *pos)
{
  gint cell_y;
  gint bin_x, bin_y;
  gdouble offset_into_row;
  gdouble third;
  GdkRectangle cell;
  RotatorColumn *column = NULL;
  GtkTreePath *tmp_path = NULL;

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_val_if_fail (tree_view != NULL, FALSE);
  g_return_val_if_fail (drag_x >= 0, FALSE);
  g_return_val_if_fail (drag_y >= 0, FALSE);
  g_return_val_if_fail (tree_view->priv->bin_window != NULL, FALSE);


  if (path)
    *path = NULL;

  if (tree_view->priv->tree == NULL)
    return FALSE;

  /* If in the top third of a row, we drop before that row; if
   * in the bottom third, drop after that row; if in the middle,
   * and the row has children, drop into the row.
   */
  rotator_convert_widget_to_bin_window_coords (tree_view, drag_x, drag_y,
						     &bin_x, &bin_y);

  if (!rotator_get_path_at_pos (tree_view,
				      bin_x,
				      bin_y,
                                      &tmp_path,
                                      &column,
                                      NULL,
                                      &cell_y))
    return FALSE;

  rotator_get_background_area (tree_view, tmp_path, column,
                                     &cell);

  offset_into_row = cell_y;

  if (path)
    *path = tmp_path;
  else
    gtk_tree_path_free (tmp_path);

  tmp_path = NULL;

  third = cell.height / 3.0;

  if (pos)
    {
      if (offset_into_row < third)
        {
          *pos = GTK_ROTATOR_DROP_BEFORE;
        }
      else if (offset_into_row < (cell.height / 2.0))
        {
          *pos = GTK_ROTATOR_DROP_INTO_OR_BEFORE;
        }
      else if (offset_into_row < third * 2.0)
        {
          *pos = GTK_ROTATOR_DROP_INTO_OR_AFTER;
        }
      else
        {
          *pos = GTK_ROTATOR_DROP_AFTER;
        }
    }

  return TRUE;
}



/* KEEP IN SYNC WITH GTK_ROTATOR_BIN_EXPOSE */
/**
 * rotator_create_row_drag_icon:
 * @tree_view: a #Rotator
 * @path: a #GtkTreePath in @tree_view
 *
 * Creates a #GdkPixmap representation of the row at @path.  
 * This image is used for a drag icon.
 *
 * Return value: a newly-allocated pixmap of the drag icon.
 **/
GdkPixmap *
rotator_create_row_drag_icon (Rotator  *tree_view,
                                    GtkTreePath  *path)
{
  GtkTreeIter   iter;
  GtkRBTree    *tree;
  GtkRBNode    *node;
  gint cell_offset;
  GList *list;
  GdkRectangle background_area;
  GdkRectangle expose_area;
  GtkWidget *widget;
  gint depth;
  /* start drawing inside the black outline */
  gint x = 1, y = 1;
  GdkDrawable *drawable;
  gint bin_window_width;
  gboolean is_separator = FALSE;
  gboolean rtl;

  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  widget = GTK_WIDGET (tree_view);

  if (!GTK_WIDGET_REALIZED (tree_view))
    return NULL;

  depth = gtk_tree_path_get_depth (path);

  _rotator_find_node (tree_view,
                            path,
                            &tree,
                            &node);

  if (tree == NULL)
    return NULL;

  if (!gtk_tree_model_get_iter (tree_view->priv->model,
                                &iter,
                                path))
    return NULL;
  
  is_separator = row_is_separator (tree_view, &iter, NULL);

  cell_offset = x;

  background_area.y = y;
  background_area.height = ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node));

  gdk_drawable_get_size (tree_view->priv->bin_window,
                         &bin_window_width, NULL);

  drawable = gdk_pixmap_new (tree_view->priv->bin_window,
                             bin_window_width + 2,
                             background_area.height + 2,
                             -1);

  expose_area.x = 0;
  expose_area.y = 0;
  expose_area.width = bin_window_width + 2;
  expose_area.height = background_area.height + 2;

  gdk_draw_rectangle (drawable,
                      widget->style->base_gc [GTK_WIDGET_STATE (widget)],
                      TRUE,
                      0, 0,
                      bin_window_width + 2,
                      background_area.height + 2);

  rtl = gtk_widget_get_direction (GTK_WIDGET (tree_view)) == GTK_TEXT_DIR_RTL;

  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
      list;
      list = (rtl ? list->prev : list->next))
    {
      RotatorColumn *column = list->data;
      GdkRectangle cell_area;
      gint vertical_separator;

      if (!column->visible)
        continue;

      rotator_column_cell_set_cell_data (column, tree_view->priv->model, &iter,
					       GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_PARENT),
					       node->children?TRUE:FALSE);

      background_area.x = cell_offset;
      background_area.width = column->width;

      gtk_widget_style_get (widget,
			    "vertical-separator", &vertical_separator,
			    NULL);

      cell_area = background_area;

      cell_area.y += vertical_separator / 2;
      cell_area.height -= vertical_separator;

      if (rotator_is_expander_column (tree_view, column))
        {
	  if (!rtl)
	    cell_area.x += (depth - 1) * tree_view->priv->level_indentation;
	  cell_area.width -= (depth - 1) * tree_view->priv->level_indentation;

          if (ROTATOR_DRAW_EXPANDERS(tree_view))
	    {
	      if (!rtl)
		cell_area.x += depth * tree_view->priv->expander_size;
	      cell_area.width -= depth * tree_view->priv->expander_size;
	    }
        }

      if (rotator_column_cell_is_visible (column))
	{
	  if (is_separator)
	    gtk_paint_hline (widget->style,
			     drawable,
			     GTK_STATE_NORMAL,
			     &cell_area,
			     widget,
			     NULL,
			     cell_area.x,
			     cell_area.x + cell_area.width,
			     cell_area.y + cell_area.height / 2);
	  else
	    _rotator_column_cell_render (column,
					       drawable,
					       &background_area,
					       &cell_area,
					       &expose_area,
					       0);
	}
      cell_offset += column->width;
    }

  gdk_draw_rectangle (drawable,
                      widget->style->black_gc,
                      FALSE,
                      0, 0,
                      bin_window_width + 1,
                      background_area.height + 1);

  return drawable;
}


/**
 * rotator_set_destroy_count_func:
 * @tree_view: A #Rotator
 * @func: Function to be called when a view row is destroyed, or %NULL
 * @data: User data to be passed to @func, or %NULL
 * @destroy: Destroy notifier for @data, or %NULL
 *
 * This function should almost never be used.  It is meant for private use by
 * ATK for determining the number of visible children that are removed when the
 * user collapses a row, or a row is deleted.
 **/
void
rotator_set_destroy_count_func (Rotator             *tree_view,
				      GtkTreeDestroyCountFunc  func,
				      gpointer                 data,
				      GDestroyNotify           destroy)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (tree_view->priv->destroy_count_destroy)
    tree_view->priv->destroy_count_destroy (tree_view->priv->destroy_count_data);

  tree_view->priv->destroy_count_func = func;
  tree_view->priv->destroy_count_data = data;
  tree_view->priv->destroy_count_destroy = destroy;
}


/*
 * Interactive search
 */

/**
 * rotator_set_enable_search:
 * @tree_view: A #Rotator
 * @enable_search: %TRUE, if the user can search interactively
 *
 * If @enable_search is set, then the user can type in text to search through
 * the tree interactively (this is sometimes called "typeahead find").
 * 
 * Note that even if this is %FALSE, the user can still initiate a search 
 * using the "start-interactive-search" key binding.
 */
void
rotator_set_enable_search (Rotator *tree_view,
				 gboolean     enable_search)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  enable_search = !!enable_search;
  
  if (tree_view->priv->enable_search != enable_search)
    {
       tree_view->priv->enable_search = enable_search;
       g_object_notify (G_OBJECT (tree_view), "enable-search");
    }
}

/**
 * rotator_get_enable_search:
 * @tree_view: A #Rotator
 *
 * Returns whether or not the tree allows to start interactive searching 
 * by typing in text.
 *
 * Return value: whether or not to let the user search interactively
 */
gboolean
rotator_get_enable_search (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);

  return tree_view->priv->enable_search;
}


/**
 * rotator_get_search_column:
 * @tree_view: A #Rotator
 *
 * Gets the column searched on by the interactive search code.
 *
 * Return value: the column the interactive search code searches in.
 */
gint
rotator_get_search_column (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), -1);

  return (tree_view->priv->search_column);
}

/**
 * rotator_set_search_column:
 * @tree_view: A #Rotator
 * @column: the column of the model to search in, or -1 to disable searching
 *
 * Sets @column as the column where the interactive search code should
 * search in for the current model. 
 * 
 * If the search column is set, users can use the "start-interactive-search"
 * key binding to bring up search popup. The enable-search property controls
 * whether simply typing text will also start an interactive search.
 *
 * Note that @column refers to a column of the current model. The search 
 * column is reset to -1 when the model is changed.
 */
void
rotator_set_search_column (Rotator *tree_view,
				 gint         column)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (column >= -1);

  if (tree_view->priv->search_column == column)
    return;

  tree_view->priv->search_column = column;
  g_object_notify (G_OBJECT (tree_view), "search-column");
}

/**
 * rotator_get_search_equal_func:
 * @tree_view: A #Rotator
 *
 * Returns the compare function currently in use.
 *
 * Return value: the currently used compare function for the search code.
 */

RotatorSearchEqualFunc
rotator_get_search_equal_func (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), NULL);

  return tree_view->priv->search_equal_func;
}

/**
 * rotator_set_search_equal_func:
 * @tree_view: A #Rotator
 * @search_equal_func: the compare function to use during the search
 * @search_user_data: user data to pass to @search_equal_func, or %NULL
 * @search_destroy: Destroy notifier for @search_user_data, or %NULL
 *
 * Sets the compare function for the interactive search capabilities; note
 * that somewhat like strcmp() returning 0 for equality
 * #RotatorSearchEqualFunc returns %FALSE on matches.
 **/
void
rotator_set_search_equal_func (Rotator                *tree_view,
				     RotatorSearchEqualFunc  search_equal_func,
				     gpointer                    search_user_data,
				     GDestroyNotify              search_destroy)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (search_equal_func != NULL);

  if (tree_view->priv->search_destroy)
    tree_view->priv->search_destroy (tree_view->priv->search_user_data);

  tree_view->priv->search_equal_func = search_equal_func;
  tree_view->priv->search_user_data = search_user_data;
  tree_view->priv->search_destroy = search_destroy;
  if (tree_view->priv->search_equal_func == NULL)
    tree_view->priv->search_equal_func = rotator_search_equal_func;
}

/**
 * rotator_get_search_entry:
 * @tree_view: A #Rotator
 *
 * Returns the #GtkEntry which is currently in use as interactive search
 * entry for @tree_view.  In case the built-in entry is being used, %NULL
 * will be returned.
 *
 * Return value: the entry currently in use as search entry.
 *
 * Since: 2.10
 */
GtkEntry *
rotator_get_search_entry (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), NULL);

  if (tree_view->priv->search_custom_entry_set)
    return GTK_ENTRY (tree_view->priv->search_entry);

  return NULL;
}

/**
 * rotator_set_search_entry:
 * @tree_view: A #Rotator
 * @entry: the entry the interactive search code of @tree_view should use or %NULL
 *
 * Sets the entry which the interactive search code will use for this
 * @tree_view.  This is useful when you want to provide a search entry
 * in our interface at all time at a fixed position.  Passing %NULL for
 * @entry will make the interactive search code use the built-in popup
 * entry again.
 *
 * Since: 2.10
 */
void
rotator_set_search_entry (Rotator *tree_view,
				GtkEntry    *entry)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  if (entry != NULL)
    g_return_if_fail (GTK_IS_ENTRY (entry));

  if (tree_view->priv->search_custom_entry_set)
    {
      if (tree_view->priv->search_entry_changed_id)
        {
	  g_signal_handler_disconnect (tree_view->priv->search_entry,
				       tree_view->priv->search_entry_changed_id);
	  tree_view->priv->search_entry_changed_id = 0;
	}
      g_signal_handlers_disconnect_by_func (tree_view->priv->search_entry,
					    G_CALLBACK (rotator_search_key_press_event),
					    tree_view);

      g_object_unref (tree_view->priv->search_entry);
    }
  else if (tree_view->priv->search_window)
    {
      gtk_widget_destroy (tree_view->priv->search_window);

      tree_view->priv->search_window = NULL;
    }

  if (entry)
    {
      tree_view->priv->search_entry = g_object_ref (entry);
      tree_view->priv->search_custom_entry_set = TRUE;

      if (tree_view->priv->search_entry_changed_id == 0)
        {
          tree_view->priv->search_entry_changed_id =
	    g_signal_connect (tree_view->priv->search_entry, "changed",
			      G_CALLBACK (rotator_search_init),
			      tree_view);
	}
      
        g_signal_connect (tree_view->priv->search_entry, "key-press-event",
		          G_CALLBACK (rotator_search_key_press_event),
		          tree_view);

	rotator_search_init (tree_view->priv->search_entry, tree_view);
    }
  else
    {
      tree_view->priv->search_entry = NULL;
      tree_view->priv->search_custom_entry_set = FALSE;
    }
}

/**
 * rotator_set_search_position_func:
 * @tree_view: A #Rotator
 * @func: the function to use to position the search dialog, or %NULL
 *    to use the default search position function
 * @data: user data to pass to @func, or %NULL
 * @destroy: Destroy notifier for @data, or %NULL
 *
 * Sets the function to use when positioning the search dialog.
 *
 * Since: 2.10
 **/
void
rotator_set_search_position_func (Rotator                   *tree_view,
				        RotatorSearchPositionFunc  func,
				        gpointer                       user_data,
				        GDestroyNotify                 destroy)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (tree_view->priv->search_position_destroy)
    tree_view->priv->search_position_destroy (tree_view->priv->search_position_user_data);

  tree_view->priv->search_position_func = func;
  tree_view->priv->search_position_user_data = user_data;
  tree_view->priv->search_position_destroy = destroy;
  if (tree_view->priv->search_position_func == NULL)
    tree_view->priv->search_position_func = rotator_search_position_func;
}

/**
 * rotator_get_search_position_func:
 * @tree_view: A #Rotator
 *
 * Returns the positioning function currently in use.
 *
 * Return value: the currently used function for positioning the search dialog.
 *
 * Since: 2.10
 */
RotatorSearchPositionFunc
rotator_get_search_position_func (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), NULL);

  return tree_view->priv->search_position_func;
}


static void
rotator_search_dialog_hide (GtkWidget   *search_dialog,
				  Rotator *tree_view)
{
  if (tree_view->priv->disable_popdown)
    return;

  if (tree_view->priv->search_entry_changed_id)
    {
      g_signal_handler_disconnect (tree_view->priv->search_entry,
				   tree_view->priv->search_entry_changed_id);
      tree_view->priv->search_entry_changed_id = 0;
    }
  if (tree_view->priv->typeselect_flush_timeout)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout = 0;
    }
	
  if (GTK_WIDGET_VISIBLE (search_dialog))
    {
      /* send focus-in event */
      send_focus_change (GTK_WIDGET (tree_view->priv->search_entry), FALSE);
      gtk_widget_hide (search_dialog);
      gtk_entry_set_text (GTK_ENTRY (tree_view->priv->search_entry), "");
      send_focus_change (GTK_WIDGET (tree_view), TRUE);
    }
}

static void
rotator_search_position_func (Rotator *tree_view,
				    GtkWidget   *search_dialog,
				    gpointer     user_data)
{
  gint x, y;
  gint tree_x, tree_y;
  gint tree_width, tree_height;
  GdkWindow *tree_window = GTK_WIDGET (tree_view)->window;
  GdkScreen *screen = gdk_drawable_get_screen (tree_window);
  GtkRequisition requisition;
  gint monitor_num;
  GdkRectangle monitor;

  monitor_num = gdk_screen_get_monitor_at_window (screen, tree_window);
  gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  gtk_widget_realize (search_dialog);

  gdk_window_get_origin (tree_window, &tree_x, &tree_y);
  gdk_drawable_get_size (tree_window,
			 &tree_width,
			 &tree_height);
  gtk_widget_size_request (search_dialog, &requisition);

  if (tree_x + tree_width > gdk_screen_get_width (screen))
    x = gdk_screen_get_width (screen) - requisition.width;
  else if (tree_x + tree_width - requisition.width < 0)
    x = 0;
  else
    x = tree_x + tree_width - requisition.width;

  if (tree_y + tree_height + requisition.height > gdk_screen_get_height (screen))
    y = gdk_screen_get_height (screen) - requisition.height;
  else if (tree_y + tree_height < 0) /* isn't really possible ... */
    y = 0;
  else
    y = tree_y + tree_height;

  gtk_window_move (GTK_WINDOW (search_dialog), x, y);
}

static void
rotator_search_disable_popdown (GtkEntry *entry,
				      GtkMenu  *menu,
				      gpointer  data)
{
  Rotator *tree_view = (Rotator *)data;

  tree_view->priv->disable_popdown = 1;
  g_signal_connect (menu, "hide",
		    G_CALLBACK (rotator_search_enable_popdown), data);
}

/* Because we're visible but offscreen, we just set a flag in the preedit
 * callback.
 */
static void
rotator_search_preedit_changed (GtkIMContext *im_context,
				      Rotator  *tree_view)
{
  tree_view->priv->imcontext_changed = 1;
  if (tree_view->priv->typeselect_flush_timeout)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout =
	gdk_threads_add_timeout (GTK_ROTATOR_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) rotator_search_entry_flush_timeout,
		       tree_view);
    }

}

static void
rotator_search_activate (GtkEntry    *entry,
			       Rotator *tree_view)
{
  GtkTreePath *path;
  GtkRBNode *node;
  GtkRBTree *tree;

  rotator_search_dialog_hide (tree_view->priv->search_window,
				    tree_view);

  /* If we have a row selected and it's the cursor row, we activate
   * the row XXX */
  if (gtk_tree_row_reference_valid (tree_view->priv->cursor))
    {
      path = gtk_tree_row_reference_get_path (tree_view->priv->cursor);
      
      _rotator_find_node (tree_view, path, &tree, &node);
      
      if (node && GTK_RBNODE_FLAG_SET (node, GTK_RBNODE_IS_SELECTED))
	rotator_row_activated (tree_view, path, tree_view->priv->focus_column);
      
      gtk_tree_path_free (path);
    }
}

static gboolean
rotator_real_search_enable_popdown (gpointer data)
{
  Rotator *tree_view = (Rotator *)data;

  tree_view->priv->disable_popdown = 0;

  return FALSE;
}

static void
rotator_search_enable_popdown (GtkWidget *widget,
				     gpointer   data)
{
  gdk_threads_add_timeout_full (G_PRIORITY_HIGH, 200, rotator_real_search_enable_popdown, g_object_ref (data), g_object_unref);
}

static gboolean
rotator_search_delete_event (GtkWidget *widget,
				   GdkEventAny *event,
				   Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  rotator_search_dialog_hide (widget, tree_view);

  return TRUE;
}

static gboolean
rotator_search_scroll_event (GtkWidget *widget, GdkEventScroll *event, Rotator *tree_view)
{
  gboolean retval = FALSE;

  if (event->direction == GDK_SCROLL_UP)
    {
      rotator_search_move (widget, tree_view, TRUE);
      retval = TRUE;
    }
  else if (event->direction == GDK_SCROLL_DOWN)
    {
      rotator_search_move (widget, tree_view, FALSE);
      retval = TRUE;
    }

  /* renew the flush timeout */
  if (retval && tree_view->priv->typeselect_flush_timeout
      && !tree_view->priv->search_custom_entry_set)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout =
	gdk_threads_add_timeout (GTK_ROTATOR_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) rotator_search_entry_flush_timeout,
		       tree_view);
    }

  return retval;
}

static gboolean
rotator_search_key_press_event (GtkWidget *widget,
				      GdkEventKey *event,
				      Rotator *tree_view)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);

  /* close window and cancel the search */
  if (!tree_view->priv->search_custom_entry_set
      && (event->keyval == GDK_Escape ||
          event->keyval == GDK_Tab ||
	    event->keyval == GDK_KP_Tab ||
	    event->keyval == GDK_ISO_Left_Tab))
    {
      rotator_search_dialog_hide (widget, tree_view);
      return TRUE;
    }

  /* select previous matching iter */
  if (event->keyval == GDK_Up || event->keyval == GDK_KP_Up)
    {
      if (!rotator_search_move (widget, tree_view, TRUE))
        gtk_widget_error_bell (widget);

      retval = TRUE;
    }

  if (((event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
      && (event->keyval == GDK_g || event->keyval == GDK_G))
    {
      if (!rotator_search_move (widget, tree_view, TRUE))
        gtk_widget_error_bell (widget);

      retval = TRUE;
    }

  /* select next matching iter */
  if (event->keyval == GDK_Down || event->keyval == GDK_KP_Down)
    {
      if (!rotator_search_move (widget, tree_view, FALSE))
        gtk_widget_error_bell (widget);

      retval = TRUE;
    }

  if (((event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
      && (event->keyval == GDK_g || event->keyval == GDK_G))
    {
      if (!rotator_search_move (widget, tree_view, FALSE))
        gtk_widget_error_bell (widget);

      retval = TRUE;
    }

  /* renew the flush timeout */
  if (retval && tree_view->priv->typeselect_flush_timeout
      && !tree_view->priv->search_custom_entry_set)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout =
	gdk_threads_add_timeout (GTK_ROTATOR_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) rotator_search_entry_flush_timeout,
		       tree_view);
    }

  return retval;
}

/*  this function returns FALSE if there is a search string but
 *  nothing was found, and TRUE otherwise.
 */
static gboolean
rotator_search_move (GtkWidget   *window,
			   Rotator *tree_view,
			   gboolean     up)
{
  gboolean ret;
  gint len;
  gint count = 0;
  const gchar *text;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;

  text = gtk_entry_get_text (GTK_ENTRY (tree_view->priv->search_entry));

  g_return_val_if_fail (text != NULL, FALSE);

  len = strlen (text);

  if (up && tree_view->priv->selected_iter == 1)
    return strlen (text) < 1;

  len = strlen (text);

  if (len < 1)
    return TRUE;

  model = rotator_get_model (tree_view);
  selection = rotator_get_selection (tree_view);

  /* search */
  gtk_tree_selection_unselect_all (selection);
  if (!gtk_tree_model_get_iter_first (model, &iter))
    return TRUE;

  ret = rotator_search_iter (model, selection, &iter, text,
				   &count, up?((tree_view->priv->selected_iter) - 1):((tree_view->priv->selected_iter + 1)));

  if (ret)
    {
      /* found */
      tree_view->priv->selected_iter += up?(-1):(1);
      return TRUE;
    }
  else
    {
      /* return to old iter */
      count = 0;
      gtk_tree_model_get_iter_first (model, &iter);
      rotator_search_iter (model, selection,
				 &iter, text,
				 &count, tree_view->priv->selected_iter);
      return FALSE;
    }
}

static gboolean
rotator_search_equal_func (GtkTreeModel *model,
				 gint          column,
				 const gchar  *key,
				 GtkTreeIter  *iter,
				 gpointer      search_data)
{
  gboolean retval = TRUE;
  const gchar *str;
  gchar *normalized_string;
  gchar *normalized_key;
  gchar *case_normalized_string = NULL;
  gchar *case_normalized_key = NULL;
  GValue value = {0,};
  GValue transformed = {0,};

  gtk_tree_model_get_value (model, iter, column, &value);

  g_value_init (&transformed, G_TYPE_STRING);

  if (!g_value_transform (&value, &transformed))
    {
      g_value_unset (&value);
      return TRUE;
    }

  g_value_unset (&value);

  str = g_value_get_string (&transformed);
  if (!str)
    {
      g_value_unset (&transformed);
      return TRUE;
    }

  normalized_string = g_utf8_normalize (str, -1, G_NORMALIZE_ALL);
  normalized_key = g_utf8_normalize (key, -1, G_NORMALIZE_ALL);

  if (normalized_string && normalized_key)
    {
      case_normalized_string = g_utf8_casefold (normalized_string, -1);
      case_normalized_key = g_utf8_casefold (normalized_key, -1);

      if (strncmp (case_normalized_key, case_normalized_string, strlen (case_normalized_key)) == 0)
        retval = FALSE;
    }

  g_value_unset (&transformed);
  g_free (normalized_key);
  g_free (normalized_string);
  g_free (case_normalized_key);
  g_free (case_normalized_string);

  return retval;
}

static gboolean
rotator_search_iter (GtkTreeModel     *model,
			   GtkTreeSelection *selection,
			   GtkTreeIter      *iter,
			   const gchar      *text,
			   gint             *count,
			   gint              n)
{
  GtkRBTree *tree = NULL;
  GtkRBNode *node = NULL;
  GtkTreePath *path;

  Rotator *tree_view = gtk_tree_selection_get_tree_view (selection);

  path = gtk_tree_model_get_path (model, iter);
  _rotator_find_node (tree_view, path, &tree, &node);

  do
    {
      if (! tree_view->priv->search_equal_func (model, tree_view->priv->search_column, text, iter, tree_view->priv->search_user_data))
        {
          (*count)++;
          if (*count == n)
            {
              rotator_scroll_to_cell (tree_view, path, NULL,
					    TRUE, 0.5, 0.0);
              gtk_tree_selection_select_iter (selection, iter);
              rotator_real_set_cursor (tree_view, path, FALSE, TRUE);

	      if (path)
		gtk_tree_path_free (path);

              return TRUE;
            }
        }

      if (node->children)
	{
	  gboolean has_child;
	  GtkTreeIter tmp;

	  tree = node->children;
	  node = tree->root;

	  while (node->left != tree->nil)
	    node = node->left;

	  tmp = *iter;
	  has_child = gtk_tree_model_iter_children (model, iter, &tmp);
	  gtk_tree_path_down (path);

	  /* sanity check */
	  ROTATOR_INTERNAL_ASSERT (has_child, FALSE);
	}
      else
	{
	  gboolean done = FALSE;

	  do
	    {
	      node = _gtk_rbtree_next (tree, node);

	      if (node)
		{
		  gboolean has_next;

		  has_next = gtk_tree_model_iter_next (model, iter);

		  done = TRUE;
		  gtk_tree_path_next (path);

		  /* sanity check */
		  ROTATOR_INTERNAL_ASSERT (has_next, FALSE);
		}
	      else
		{
		  gboolean has_parent;
		  GtkTreeIter tmp_iter = *iter;

		  node = tree->parent_node;
		  tree = tree->parent_tree;

		  if (!tree)
		    {
		      if (path)
			gtk_tree_path_free (path);

		      /* we've run out of tree, done with this func */
		      return FALSE;
		    }

		  has_parent = gtk_tree_model_iter_parent (model,
							   iter,
							   &tmp_iter);
		  gtk_tree_path_up (path);

		  /* sanity check */
		  ROTATOR_INTERNAL_ASSERT (has_parent, FALSE);
		}
	    }
	  while (!done);
	}
    }
  while (1);

  return FALSE;
}

static void
rotator_search_init (GtkWidget   *entry,
			   Rotator *tree_view)
{
  gint ret;
  gint count = 0;
  const gchar *text;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;

  g_return_if_fail (GTK_IS_ENTRY (entry));
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  text = gtk_entry_get_text (GTK_ENTRY (entry));

  model = rotator_get_model (tree_view);
  selection = rotator_get_selection (tree_view);

  /* search */
  gtk_tree_selection_unselect_all (selection);
  if (tree_view->priv->typeselect_flush_timeout
      && !tree_view->priv->search_custom_entry_set)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout =
	gdk_threads_add_timeout (GTK_ROTATOR_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) rotator_search_entry_flush_timeout,
		       tree_view);
    }

  if (*text == '\0')
    return;

  if (!gtk_tree_model_get_iter_first (model, &iter))
    return;

  ret = rotator_search_iter (model, selection,
				   &iter, text,
				   &count, 1);

  if (ret)
    tree_view->priv->selected_iter = 1;
}

static void
rotator_remove_widget (GtkCellEditable *cell_editable,
			     Rotator     *tree_view)
{
  if (tree_view->priv->edited_column == NULL)
    return;

  _rotator_column_stop_editing (tree_view->priv->edited_column);
  tree_view->priv->edited_column = NULL;

  if (GTK_WIDGET_HAS_FOCUS (cell_editable))
    gtk_widget_grab_focus (GTK_WIDGET (tree_view));

  g_signal_handlers_disconnect_by_func (cell_editable,
					rotator_remove_widget,
					tree_view);

  gtk_container_remove (GTK_CONTAINER (tree_view),
			GTK_WIDGET (cell_editable));  

  /* FIXME should only redraw a single node */
  gtk_widget_queue_draw (GTK_WIDGET (tree_view));
}

static gboolean
rotator_start_editing (Rotator *tree_view,
			     GtkTreePath *cursor_path)
{
  GtkTreeIter iter;
  GdkRectangle background_area;
  GdkRectangle cell_area;
  GtkCellEditable *editable_widget = NULL;
  gchar *path_string;
  guint flags = 0; /* can be 0, as the flags are primarily for rendering */
  gint retval = FALSE;
  GtkRBTree *cursor_tree;
  GtkRBNode *cursor_node;

  g_assert (tree_view->priv->focus_column);

  if (! GTK_WIDGET_REALIZED (tree_view))
    return FALSE;

  if (_rotator_find_node (tree_view, cursor_path, &cursor_tree, &cursor_node) ||
      cursor_node == NULL)
    return FALSE;

  path_string = gtk_tree_path_to_string (cursor_path);
  gtk_tree_model_get_iter (tree_view->priv->model, &iter, cursor_path);

  validate_row (tree_view, cursor_tree, cursor_node, &iter, cursor_path);

  rotator_column_cell_set_cell_data (tree_view->priv->focus_column,
					   tree_view->priv->model,
					   &iter,
					   GTK_RBNODE_FLAG_SET (cursor_node, GTK_RBNODE_IS_PARENT),
					   cursor_node->children?TRUE:FALSE);
  rotator_get_background_area (tree_view,
				     cursor_path,
				     tree_view->priv->focus_column,
				     &background_area);
  rotator_get_cell_area (tree_view,
			       cursor_path,
			       tree_view->priv->focus_column,
			       &cell_area);

  if (_rotator_column_cell_event (tree_view->priv->focus_column,
					&editable_widget,
					NULL,
					path_string,
					&background_area,
					&cell_area,
					flags))
    {
      retval = TRUE;
      if (editable_widget != NULL)
	{
	  gint left, right;
	  GdkRectangle area;
	  GtkCellRenderer *cell;

	  area = cell_area;
	  cell = _rotator_column_get_edited_cell (tree_view->priv->focus_column);

	  _rotator_column_get_neighbor_sizes (tree_view->priv->focus_column, cell, &left, &right);

	  area.x += left;
	  area.width -= right + left;

	  rotator_real_start_editing (tree_view,
					    tree_view->priv->focus_column,
					    cursor_path,
					    editable_widget,
					    &area,
					    NULL,
					    flags);
	}

    }
  g_free (path_string);
  return retval;
}

static void
rotator_real_start_editing (Rotator       *tree_view,
				  RotatorColumn *column,
				  GtkTreePath       *path,
				  GtkCellEditable   *cell_editable,
				  GdkRectangle      *cell_area,
				  GdkEvent          *event,
				  guint              flags)
{
  gint pre_val = tree_view->priv->vadjustment->value;
  GtkRequisition requisition;

  tree_view->priv->edited_column = column;
  _rotator_column_start_editing (column, GTK_CELL_EDITABLE (cell_editable));

  rotator_real_set_cursor (tree_view, path, FALSE, TRUE);
  cell_area->y += pre_val - (int)tree_view->priv->vadjustment->value;

  gtk_widget_size_request (GTK_WIDGET (cell_editable), &requisition);

  GTK_ROTATOR_SET_FLAG (tree_view, GTK_ROTATOR_DRAW_KEYFOCUS);

  if (requisition.height < cell_area->height)
    {
      gint diff = cell_area->height - requisition.height;
      rotator_put (tree_view,
			 GTK_WIDGET (cell_editable),
			 cell_area->x, cell_area->y + diff/2,
			 cell_area->width, requisition.height);
    }
  else
    {
      rotator_put (tree_view,
			 GTK_WIDGET (cell_editable),
			 cell_area->x, cell_area->y,
			 cell_area->width, cell_area->height);
    }

  gtk_cell_editable_start_editing (GTK_CELL_EDITABLE (cell_editable),
				   (GdkEvent *)event);

  gtk_widget_grab_focus (GTK_WIDGET (cell_editable));
  g_signal_connect (cell_editable, "remove-widget",
		    G_CALLBACK (rotator_remove_widget), tree_view);
}

static void
rotator_stop_editing (Rotator *tree_view,
			    gboolean     cancel_editing)
{
  RotatorColumn *column;
  GtkCellRenderer *cell;

  if (tree_view->priv->edited_column == NULL)
    return;

  /*
   * This is very evil. We need to do this, because
   * gtk_cell_editable_editing_done may trigger rotator_row_changed
   * later on. If rotator_row_changed notices
   * tree_view->priv->edited_column != NULL, it'll call
   * rotator_stop_editing again. Bad things will happen then.
   *
   * Please read that again if you intend to modify anything here.
   */

  column = tree_view->priv->edited_column;
  tree_view->priv->edited_column = NULL;

  cell = _rotator_column_get_edited_cell (column);
  gtk_cell_renderer_stop_editing (cell, cancel_editing);

  if (!cancel_editing)
    gtk_cell_editable_editing_done (column->editable_widget);

  tree_view->priv->edited_column = column;

  gtk_cell_editable_remove_widget (column->editable_widget);
}


/**
 * rotator_set_hover_selection:
 * @tree_view: a #Rotator
 * @hover: %TRUE to enable hover selection mode
 *
 * Enables of disables the hover selection mode of @tree_view.
 * Hover selection makes the selected row follow the pointer.
 * Currently, this works only for the selection modes 
 * %GTK_SELECTION_SINGLE and %GTK_SELECTION_BROWSE.
 * 
 * Since: 2.6
 **/
void     
rotator_set_hover_selection (Rotator *tree_view,
				   gboolean     hover)
{
  hover = hover != FALSE;

  if (hover != tree_view->priv->hover_selection)
    {
      tree_view->priv->hover_selection = hover;

      g_object_notify (G_OBJECT (tree_view), "hover-selection");
    }
}

/**
 * rotator_get_hover_selection:
 * @tree_view: a #Rotator
 * 
 * Returns whether hover selection mode is turned on for @tree_view.
 * 
 * Return value: %TRUE if @tree_view is in hover selection mode
 *
 * Since: 2.6 
 **/
gboolean 
rotator_get_hover_selection (Rotator *tree_view)
{
  return tree_view->priv->hover_selection;
}

/**
 * rotator_set_hover_expand:
 * @tree_view: a #Rotator
 * @expand: %TRUE to enable hover selection mode
 *
 * Enables of disables the hover expansion mode of @tree_view.
 * Hover expansion makes rows expand or collapse if the pointer 
 * moves over them.
 * 
 * Since: 2.6
 **/
void     
rotator_set_hover_expand (Rotator *tree_view,
				gboolean     expand)
{
  expand = expand != FALSE;

  if (expand != tree_view->priv->hover_expand)
    {
      tree_view->priv->hover_expand = expand;

      g_object_notify (G_OBJECT (tree_view), "hover-expand");
    }
}

/**
 * rotator_get_hover_expand:
 * @tree_view: a #Rotator
 * 
 * Returns whether hover expansion mode is turned on for @tree_view.
 * 
 * Return value: %TRUE if @tree_view is in hover expansion mode
 *
 * Since: 2.6 
 **/
gboolean 
rotator_get_hover_expand (Rotator *tree_view)
{
  return tree_view->priv->hover_expand;
}

/**
 * rotator_set_rubber_banding:
 * @tree_view: a #Rotator
 * @enable: %TRUE to enable rubber banding
 *
 * Enables or disables rubber banding in @tree_view.  If the selection mode
 * is #GTK_SELECTION_MULTIPLE, rubber banding will allow the user to select
 * multiple rows by dragging the mouse.
 * 
 * Since: 2.10
 **/
void
rotator_set_rubber_banding (Rotator *tree_view,
				  gboolean     enable)
{
  enable = enable != FALSE;

  if (enable != tree_view->priv->rubber_banding_enable)
    {
      tree_view->priv->rubber_banding_enable = enable;

      g_object_notify (G_OBJECT (tree_view), "rubber-banding");
    }
}

/**
 * rotator_get_rubber_banding:
 * @tree_view: a #Rotator
 * 
 * Returns whether rubber banding is turned on for @tree_view.  If the
 * selection mode is #GTK_SELECTION_MULTIPLE, rubber banding will allow the
 * user to select multiple rows by dragging the mouse.
 * 
 * Return value: %TRUE if rubber banding in @tree_view is enabled.
 *
 * Since: 2.10
 **/
gboolean
rotator_get_rubber_banding (Rotator *tree_view)
{
  return tree_view->priv->rubber_banding_enable;
}

/**
 * rotator_is_rubber_banding_active:
 * @tree_view: a #Rotator
 * 
 * Returns whether a rubber banding operation is currently being done
 * in @tree_view.
 *
 * Return value: %TRUE if a rubber banding operation is currently being
 * done in @tree_view.
 *
 * Since: 2.12
 **/
gboolean
rotator_is_rubber_banding_active (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);

  if (tree_view->priv->rubber_banding_enable
      && tree_view->priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    return TRUE;

  return FALSE;
}

/**
 * rotator_get_row_separator_func:
 * @tree_view: a #Rotator
 * 
 * Returns the current row separator function.
 * 
 * Return value: the current row separator function.
 *
 * Since: 2.6
 **/
RotatorRowSeparatorFunc 
rotator_get_row_separator_func (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), NULL);

  return tree_view->priv->row_separator_func;
}

/**
 * rotator_set_row_separator_func:
 * @tree_view: a #Rotator
 * @func: a #RotatorRowSeparatorFunc
 * @data: user data to pass to @func, or %NULL
 * @destroy: destroy notifier for @data, or %NULL
 * 
 * Sets the row separator function, which is used to determine
 * whether a row should be drawn as a separator. If the row separator
 * function is %NULL, no separators are drawn. This is the default value.
 *
 * Since: 2.6
 **/
void
rotator_set_row_separator_func (Rotator                 *tree_view,
				      RotatorRowSeparatorFunc  func,
				      gpointer                     data,
				      GDestroyNotify               destroy)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (tree_view->priv->row_separator_destroy)
    tree_view->priv->row_separator_destroy (tree_view->priv->row_separator_data);

  tree_view->priv->row_separator_func = func;
  tree_view->priv->row_separator_data = data;
  tree_view->priv->row_separator_destroy = destroy;
}

  
static void
rotator_grab_notify (GtkWidget *widget,
			   gboolean   was_grabbed)
{
  Rotator *tree_view = GTK_ROTATOR (widget);

  tree_view->priv->in_grab = !was_grabbed;

  if (!was_grabbed)
    {
      tree_view->priv->pressed_button = -1;

      if (tree_view->priv->rubber_band_status)
	rotator_stop_rubber_band (tree_view);
    }
}

static void
rotator_state_changed (GtkWidget      *widget,
		 	     GtkStateType    previous_state)
{
  Rotator *tree_view = GTK_ROTATOR (widget);

  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_set_back_pixmap (widget->window, NULL, FALSE);
      gdk_window_set_background (tree_view->priv->bin_window, &widget->style->base[widget->state]);
    }

  gtk_widget_queue_draw (widget);
}

/**
 * rotator_get_grid_lines:
 * @tree_view: a #Rotator
 *
 * Returns which grid lines are enabled in @tree_view.
 *
 * Return value: a #RotatorGridLines value indicating which grid lines
 * are enabled.
 *
 * Since: 2.10
 */
RotatorGridLines
rotator_get_grid_lines (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), 0);

  return tree_view->priv->grid_lines;
}

/**
 * rotator_set_grid_lines:
 * @tree_view: a #Rotator
 * @grid_lines: a #RotatorGridLines value indicating which grid lines to
 * enable.
 *
 * Sets which grid lines to draw in @tree_view.
 *
 * Since: 2.10
 */
void
rotator_set_grid_lines (Rotator           *tree_view,
			      RotatorGridLines   grid_lines)
{
  RotatorPrivate *priv;
  GtkWidget *widget;
  RotatorGridLines old_grid_lines;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  priv = tree_view->priv;
  widget = GTK_WIDGET (tree_view);

  old_grid_lines = priv->grid_lines;
  priv->grid_lines = grid_lines;
  
  if (GTK_WIDGET_REALIZED (widget))
    {
      if (grid_lines == GTK_ROTATOR_GRID_LINES_NONE &&
	  priv->grid_line_gc)
	{
	  g_object_unref (priv->grid_line_gc);
	  priv->grid_line_gc = NULL;
	}
      
      if (grid_lines != GTK_ROTATOR_GRID_LINES_NONE && 
	  !priv->grid_line_gc)
	{
	  gint line_width;
	  gint8 *dash_list;

	  gtk_widget_style_get (widget,
				"grid-line-width", &line_width,
				"grid-line-pattern", (gchar *)&dash_list,
				NULL);
      
	  priv->grid_line_gc = gdk_gc_new (widget->window);
	  gdk_gc_copy (priv->grid_line_gc, widget->style->black_gc);
	  
	  gdk_gc_set_line_attributes (priv->grid_line_gc, line_width,
				      GDK_LINE_ON_OFF_DASH,
				      GDK_CAP_BUTT, GDK_JOIN_MITER);
	  gdk_gc_set_dashes (priv->grid_line_gc, 0, dash_list, 2);

	  g_free (dash_list);
	}      
    }

  if (old_grid_lines != grid_lines)
    {
      gtk_widget_queue_draw (GTK_WIDGET (tree_view));
      
      g_object_notify (G_OBJECT (tree_view), "enable-grid-lines");
    }
}

/**
 * rotator_set_show_expanders:
 * @tree_view: a #Rotator
 * @enabled: %TRUE to enable expander drawing, %FALSE otherwise.
 *
 * Sets whether to draw and enable expanders and indent child rows in
 * @tree_view.  When disabled there will be no expanders visible in trees
 * and there will be no way to expand and collapse rows by default.  Also
 * note that hiding the expanders will disable the default indentation.  You
 * can set a custom indentation in this case using
 * rotator_set_level_indentation().
 * This does not have any visible effects for lists.
 *
 * Since: 2.12
 */
void
rotator_set_show_expanders (Rotator *tree_view,
				  gboolean     enabled)
{
  gboolean was_enabled;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  enabled = enabled != FALSE;
  was_enabled = GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_SHOW_EXPANDERS);

  if (enabled)
    GTK_ROTATOR_SET_FLAG (tree_view, GTK_ROTATOR_SHOW_EXPANDERS);
  else
    GTK_ROTATOR_UNSET_FLAG (tree_view, GTK_ROTATOR_SHOW_EXPANDERS);

  if (enabled != was_enabled)
    gtk_widget_queue_draw (GTK_WIDGET (tree_view));
}

/**
 * rotator_get_show_expanders:
 * @tree_view: a #Rotator.
 *
 * Returns whether or not expanders are drawn in @tree_view.
 *
 * Return value: %TRUE if expanders are drawn in @tree_view, %FALSE
 * otherwise.
 *
 * Since: 2.12
 */
gboolean
rotator_get_show_expanders (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);

  return GTK_ROTATOR_FLAG_SET (tree_view, GTK_ROTATOR_SHOW_EXPANDERS);
}

/**
 * rotator_set_level_indentation:
 * @tree_view: a #Rotator
 * @indentation: the amount, in pixels, of extra indentation in @tree_view.
 *
 * Sets the amount of extra indentation for child levels to use in @tree_view
 * in addition to the default indentation.  The value should be specified in
 * pixels, a value of 0 disables this feature and in this case only the default
 * indentation will be used.
 * This does not have any visible effects for lists.
 *
 * Since: 2.12
 */
void
rotator_set_level_indentation (Rotator *tree_view,
				     gint         indentation)
{
  tree_view->priv->level_indentation = indentation;

  gtk_widget_queue_draw (GTK_WIDGET (tree_view));
}

/**
 * rotator_get_level_indentation:
 * @tree_view: a #Rotator.
 *
 * Returns the amount, in pixels, of extra indentation for child levels
 * in @tree_view.
 *
 * Return value: the amount of extra indentation for child levels in
 * @tree_view.  A return value of 0 means that this feature is disabled.
 *
 * Since: 2.12
 */
gint
rotator_get_level_indentation (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), 0);

  return tree_view->priv->level_indentation;
}

/**
 * rotator_set_tooltip_row:
 * @tree_view: a #Rotator
 * @tooltip: a #GtkTooltip
 * @path: a #GtkTreePath
 *
 * Sets the tip area of @tooltip to be the area covered by the row at @path.
 * See also rotator_set_tooltip_column() for a simpler alternative.
 * See also gtk_tooltip_set_tip_area().
 *
 * Since: 2.12
 */
void
rotator_set_tooltip_row (Rotator *tree_view,
			       GtkTooltip  *tooltip,
			       GtkTreePath *path)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (GTK_IS_TOOLTIP (tooltip));

  rotator_set_tooltip_cell (tree_view, tooltip, path, NULL, NULL);
}

/**
 * rotator_set_tooltip_cell:
 * @tree_view: a #Rotator
 * @tooltip: a #GtkTooltip
 * @path: a #GtkTreePath or %NULL
 * @column: a #RotatorColumn or %NULL
 * @cell: a #GtkCellRenderer or %NULL
 *
 * Sets the tip area of @tooltip to the area @path, @column and @cell have
 * in common.  For example if @path is %NULL and @column is set, the tip
 * area will be set to the full area covered by @column.  See also
 * gtk_tooltip_set_tip_area().
 *
 * Note that if @path is not specified and @cell is set and part of a column
 * containing the expander, the tooltip might not show and hide at the correct
 * position.  In such cases @path must be set to the current node under the
 * mouse cursor for this function to operate correctly.
 *
 * See also rotator_set_tooltip_column() for a simpler alternative.
 *
 * Since: 2.12
 */
void
rotator_set_tooltip_cell (Rotator       *tree_view,
				GtkTooltip        *tooltip,
				GtkTreePath       *path,
				RotatorColumn *column,
				GtkCellRenderer   *cell)
{
  GdkRectangle rect;

  g_return_if_fail (GTK_IS_ROTATOR (tree_view));
  g_return_if_fail (GTK_IS_TOOLTIP (tooltip));

  if (column)
    g_return_if_fail (GTK_IS_ROTATOR_COLUMN (column));

  if (cell)
    g_return_if_fail (GTK_IS_CELL_RENDERER (cell));

  /* Determine x values. */
  if (column && cell)
    {
      GdkRectangle tmp;
      gint start, width;

      /* We always pass in path here, whether it is NULL or not.
       * For cells in expander columns path must be specified so that
       * we can correctly account for the indentation.  This also means
       * that the tooltip is constrained vertically by the "Determine y
       * values" code below; this is not a real problem since cells actually
       * don't stretch vertically in constrast to columns.
       */
      rotator_get_cell_area (tree_view, path, column, &tmp);
      rotator_column_cell_get_position (column, cell, &start, &width);

      rotator_convert_bin_window_to_widget_coords (tree_view,
							 tmp.x + start, 0,
							 &rect.x, NULL);
      rect.width = width;
    }
  else if (column)
    {
      GdkRectangle tmp;

      rotator_get_background_area (tree_view, NULL, column, &tmp);
      rotator_convert_bin_window_to_widget_coords (tree_view,
							 tmp.x, 0,
							 &rect.x, NULL);
      rect.width = tmp.width;
    }
  else
    {
      rect.x = 0;
      rect.width = GTK_WIDGET (tree_view)->allocation.width;
    }

  /* Determine y values. */
  if (path)
    {
      GdkRectangle tmp;

      rotator_get_background_area (tree_view, path, NULL, &tmp);
      rotator_convert_bin_window_to_widget_coords (tree_view,
							 0, tmp.y,
							 NULL, &rect.y);
      rect.height = tmp.height;
    }
  else
    {
      rect.y = 0;
      rect.height = tree_view->priv->vadjustment->page_size;
    }

  gtk_tooltip_set_tip_area (tooltip, &rect);
}

/**
 * rotator_get_tooltip_context:
 * @tree_view: a #Rotator
 * @x: the x coordinate (relative to widget coordinates)
 * @y: the y coordinate (relative to widget coordinates)
 * @keyboard_tip: whether this is a keyboard tooltip or not
 * @model: a pointer to receive a #GtkTreeModel or %NULL
 * @path: a pointer to receive a #GtkTreePath or %NULL
 * @iter: a pointer to receive a #GtkTreeIter or %NULL
 *
 * This function is supposed to be used in a #GtkWidget::query-tooltip
 * signal handler for #Rotator.  The @x, @y and @keyboard_tip values
 * which are received in the signal handler, should be passed to this
 * function without modification.
 *
 * The return value indicates whether there is a tree view row at the given
 * coordinates (%TRUE) or not (%FALSE) for mouse tooltips.  For keyboard
 * tooltips the row returned will be the cursor row.  When %TRUE, then any of
 * @model, @path and @iter which have been provided will be set to point to
 * that row and the corresponding model.  @x and @y will always be converted
 * to be relative to @tree_view's bin_window if @keyboard_tooltip is %FALSE.
 *
 * Return value: whether or not the given tooltip context points to a row.
 *
 * Since: 2.12
 */
gboolean
rotator_get_tooltip_context (Rotator   *tree_view,
				   gint          *x,
				   gint          *y,
				   gboolean       keyboard_tip,
				   GtkTreeModel **model,
				   GtkTreePath  **path,
				   GtkTreeIter   *iter)
{
  GtkTreePath *tmppath = NULL;

  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), FALSE);
  g_return_val_if_fail (x != NULL, FALSE);
  g_return_val_if_fail (y != NULL, FALSE);

  if (keyboard_tip)
    {
      rotator_get_cursor (tree_view, &tmppath, NULL);

      if (!tmppath)
	return FALSE;
    }
  else
    {
      rotator_convert_widget_to_bin_window_coords (tree_view, *x, *y,
							 x, y);

      if (!rotator_get_path_at_pos (tree_view, *x, *y,
					  &tmppath, NULL, NULL, NULL))
	return FALSE;
    }

  if (model)
    *model = rotator_get_model (tree_view);

  if (iter)
    gtk_tree_model_get_iter (rotator_get_model (tree_view),
			     iter, tmppath);

  if (path)
    *path = tmppath;
  else
    gtk_tree_path_free (tmppath);

  return TRUE;
}

static gboolean
rotator_set_tooltip_query_cb (GtkWidget  *widget,
				    gint        x,
				    gint        y,
				    gboolean    keyboard_tip,
				    GtkTooltip *tooltip,
				    gpointer    data)
{
  GValue value = { 0, };
  GValue transformed = { 0, };
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkTreeModel *model;
  Rotator *tree_view = GTK_ROTATOR (widget);

  if (!rotator_get_tooltip_context (GTK_ROTATOR (widget),
					  &x, &y,
					  keyboard_tip,
					  &model, &path, &iter))
    return FALSE;

  gtk_tree_model_get_value (model, &iter,
                            tree_view->priv->tooltip_column, &value);

  g_value_init (&transformed, G_TYPE_STRING);

  if (!g_value_transform (&value, &transformed))
    {
      g_value_unset (&value);
      gtk_tree_path_free (path);

      return FALSE;
    }

  g_value_unset (&value);

  if (!g_value_get_string (&transformed))
    {
      g_value_unset (&transformed);
      gtk_tree_path_free (path);

      return FALSE;
    }

  gtk_tooltip_set_markup (tooltip, g_value_get_string (&transformed));
  rotator_set_tooltip_row (tree_view, tooltip, path);

  gtk_tree_path_free (path);
  g_value_unset (&transformed);

  return TRUE;
}

/**
 * rotator_set_tooltip_column:
 * @tree_view: a #Rotator
 * @column: an integer, which is a valid column number for @tree_view's model
 *
 * If you only plan to have simple (text-only) tooltips on full rows, you
 * can use this function to have #Rotator handle these automatically
 * for you. @column should be set to the column in @tree_view's model
 * containing the tooltip texts, or -1 to disable this feature.
 *
 * When enabled, #GtkWidget::has-tooltip will be set to %TRUE and
 * @tree_view will connect a #GtkWidget::query-tooltip signal handler.
 *
 * Note that the signal handler sets the text with gtk_tooltip_set_markup(),
 * so &amp;, &lt;, etc have to be escaped in the text.
 *
 * Since: 2.12
 */
void
rotator_set_tooltip_column (Rotator *tree_view,
			          gint         column)
{
  g_return_if_fail (GTK_IS_ROTATOR (tree_view));

  if (column == tree_view->priv->tooltip_column)
    return;

  if (column == -1)
    {
      g_signal_handlers_disconnect_by_func (tree_view,
	  				    rotator_set_tooltip_query_cb,
					    NULL);
      gtk_widget_set_has_tooltip (GTK_WIDGET (tree_view), FALSE);
    }
  else
    {
      if (tree_view->priv->tooltip_column == -1)
        {
          g_signal_connect (tree_view, "query-tooltip",
		            G_CALLBACK (rotator_set_tooltip_query_cb), NULL);
          gtk_widget_set_has_tooltip (GTK_WIDGET (tree_view), TRUE);
        }
    }

  tree_view->priv->tooltip_column = column;
  g_object_notify (G_OBJECT (tree_view), "tooltip-column");
}

/**
 * rotator_get_tooltip_column:
 * @tree_view: a #Rotator
 *
 * Returns the column of @tree_view's model which is being used for
 * displaying tooltips on @tree_view's rows.
 *
 * Return value: the index of the tooltip column that is currently being
 * used, or -1 if this is disabled.
 *
 * Since: 2.12
 */
gint
rotator_get_tooltip_column (Rotator *tree_view)
{
  g_return_val_if_fail (GTK_IS_ROTATOR (tree_view), 0);

  return tree_view->priv->tooltip_column;
}
#endif


static gboolean canvas_init_done = false;
static void
on_canvas_realise (GtkWidget* widget, gpointer user_data)
{
	PF;
	RotatorPrivate* _r = ((Rotator*)widget)->priv;
	if(canvas_init_done) return;
	if(!GTK_WIDGET_REALIZED (widget)) return;

	gl_initialised = true;

	((AGlActor*)_r->scene)->region = (AGlfRegion){
		.x2 = widget->allocation.width,
		.y2 = widget->allocation.height
	};
	((AGlActor*)_r->scene)->scrollable = (AGliRegion){
		.x2 = widget->allocation.width,
		.y2 = widget->allocation.height
	};

	canvas_init_done = true;

	on_allocate(widget, &widget->allocation, user_data);

	// Allow the WaveformCanvas to initiate redraws
	void _on_scene_requests_redraw (AGlScene* wfc, gpointer __canvas)
	{
		gdk_window_invalidate_rect(((GtkWidget*)__canvas)->window, NULL, false);
	}
	_r->scene->draw = _on_scene_requests_redraw;
}


static void
on_allocate (GtkWidget* widget, GtkAllocation* allocation, gpointer user_data)
{
	if(!gl_initialised) return;

	start_zoom((Rotator*)widget, zoom);
}


static void
rotator_setup_projection (AGlActor* scene)
{
	if(!scene) return;

	int vx = 0;
	int vy = 0;
	int vw = scene->region.x2;
	int vh = scene->region.y2;
	glViewport(vx, vy, vw, vh);

	dbg (1, "viewport: %i %i %i %i", vx, vy, vw, vh);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	double left = 0;
	double right = scene->region.x2;
	double bottom = scene->region.y2;
	double top = 0;
	glOrtho (left, right, bottom, top, 512.0, -512.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(rotate[0], 1.0f, 0.0f, 0.0f);
	glRotatef(rotate[1], 0.0f, 1.0f, 0.0f);
	glScalef(1.0f, 1.0f, -1.0f);
}


static void
start_zoom (Rotator* tree_view, float target_zoom)
{
	// When zooming in, the Region is preserved so the box gets bigger. Drawing is clipped by the Viewport.

	zoom = MAX(0.1, target_zoom);

	int i = 0;
	GList* l = tree_view->priv->actors;
	for(;l;l=l->next){
		set_position(((SampleActor*)l->data)->actor, i);
		i++;
	}
}


static void
set_position (WaveformActor* actor, int j)
{
	#define Y_FACTOR 0.0f //0.5f //currently set to zero to simplify changing stack order

	float width = ((AGlActor*)actor)->parent->region.x2;
	float height = ((AGlActor*)actor)->parent->region.y2;

	int h = height / 4;

	if(actor) wf_actor_set_rect(actor, &(WfRectangle){
		20.0,
		height - h + ((float)j) * height * Y_FACTOR / 4 - 10.0f - 0.25 * width,
		width * zoom,
		h
	});
}
#endif // GTK4_TODO
