/* gdl-dock-object.h - Abstract base class for all dock related objects
 *
 * This file is part of the GNOME Devtools Libraries.
 *
 * Copyright (C) 2002 Gustavo Giráldez <gustavo.giraldez@gmx.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GDL_TYPE_DOCK_OBJECT             (gdl_dock_object_get_type ())
#define GDL_DOCK_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDL_TYPE_DOCK_OBJECT, GdlDockObject))
#define GDL_DOCK_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK_OBJECT, GdlDockObjectClass))
#define GDL_IS_DOCK_OBJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDL_TYPE_DOCK_OBJECT))
#define GDL_IS_DOCK_OBJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK_OBJECT))
#define GDL_DOCK_OBJECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_DOCK_OBJECT, GdlDockObjectClass))

/**
 * GdlDockParamFlags:
 * @GDL_DOCK_PARAM_EXPORT: The parameter is to be exported for later layout rebuilding
 * @GDL_DOCK_PARAM_AFTER: The parameter must be set after adding the children objects
 *
 * Used to flag additional characteristics to GObject properties used in dock
 * object.
 *
 **/
typedef enum {
    /* the parameter is to be exported for later layout rebuilding */
    GDL_DOCK_PARAM_EXPORT = 1 << G_PARAM_USER_SHIFT,
    /* the parameter must be set after adding the children objects */
    GDL_DOCK_PARAM_AFTER  = 1 << (G_PARAM_USER_SHIFT + 1)
} GdlDockParamFlags;

#define GDL_DOCK_NAME_PROPERTY    "name"
#define GDL_DOCK_MASTER_PROPERTY  "master"

#ifndef GDL_DISABLE_DEPRECATED
/**
 * GdlDockObjectFlags:
 * @GDL_DOCK_AUTOMATIC: Object is created and destroyed by the master, not the user
 * @GDL_DOCK_ATTACHED: Object has a parent
 * @GDL_DOCK_IN_REFLOW: Object is currently part of a rearrangement
 * @GDL_DOCK_IN_DETACH: Object will be removed
 *
 * Described the state of a #GdlDockObject.
 *
 * Since 3.6: These flags are available using access function, like
 * gdl_dock_object_is_automatic() or gdl_dock_object_is_closed().
 */
typedef enum {
    GDL_DOCK_AUTOMATIC  = 1 << 0,
    GDL_DOCK_ATTACHED   = 1 << 1,
    GDL_DOCK_IN_REFLOW  = 1 << 2,
    GDL_DOCK_IN_DETACH  = 1 << 3
} GdlDockObjectFlags;
#endif

#ifndef GDL_DISABLE_DEPRECATED
/**
 * GDL_DOCK_OBJECT_FLAGS_SHIFT:
 *
 * Minimum shift count to be used for user defined flags, to be stored in
 * #GdlDockObject.flags.
 *
 * Deprecated: 3.6: Use a private flag instead
 */
#define GDL_DOCK_OBJECT_FLAGS_SHIFT 8
#endif

/**
 * GdlDockPlacement:
 * @GDL_DOCK_NONE: No position defined
 * @GDL_DOCK_TOP: Dock object on the top
 * @GDL_DOCK_BOTTOM: Dock object on the bottom
 * @GDL_DOCK_RIGHT: Dock object on the right
 * @GDL_DOCK_LEFT: Dock object on the left
 * @GDL_DOCK_CENTER: Dock object on top of the other
 * @GDL_DOCK_FLOATING: Dock object in its own window
 *
 * Described the docking position.
 *
 **/
typedef enum {
    GDL_DOCK_NONE = 0,
    GDL_DOCK_TOP,
    GDL_DOCK_BOTTOM,
    GDL_DOCK_RIGHT,
    GDL_DOCK_LEFT,
    GDL_DOCK_CENTER,
    GDL_DOCK_FLOATING
} GdlDockPlacement;

typedef struct _GdlDockObject             GdlDockObject;
typedef struct _GdlDockObjectPrivate      GdlDockObjectPrivate;
typedef struct _GdlDockObjectClass        GdlDockObjectClass;
typedef struct _GdlDockObjectClassPrivate GdlDockObjectClassPrivate;
typedef struct _GdlDockRequest            GdlDockRequest;

typedef void   (*GdlDockObjectFn)         (GdlDockObject*, gpointer);

/**
 * GdlDockRequest:
 * @applicant: A #GdlDockObject to dock
 * @target: The #GdlDockObject target
 * @position: how to dock @applicant in @target
 * @rect: Precise position
 * @extra: Additional information
 *
 * Full docking information.
 **/
struct _GdlDockRequest {
    GdlDockObject               *applicant;
    GdlDockObject               *target;
    GdlDockPlacement            position;
    GtkWidget*                  dock;
    graphene_rect_t             rect;
    GValue                      extra;
};

struct _GdlDockObject {
    GtkWidget           container;
    GObject            *master;
#ifndef GDL_DISABLE_DEPRECATED
    /* Just for compiling, these data are not initialized anymore */
    GdlDockObjectFlags  deprecated_flags;
#endif
    /*< private >*/
    GdlDockObjectPrivate  *priv;
};

struct _GdlDockObjectClass {
    GtkWidgetClass parent_class;

    GdlDockObjectClassPrivate *priv;

    void     (* detach)          (GdlDockObject    *object,
                                  gboolean          recursive);
    void     (* reduce)          (GdlDockObject    *object);

    gboolean (* dock_request)    (GdlDockObject    *object,
                                  gint              x,
                                  gint              y,
                                  GdlDockRequest   *request);

    void     (* dock)            (GdlDockObject    *object,
                                  GdlDockObject    *requestor,
                                  GdlDockPlacement  position,
                                  GValue           *other_data);

    gboolean (* reorder)         (GdlDockObject    *object,
                                  GdlDockObject    *child,
                                  GdlDockPlacement  new_position,
                                  GValue           *other_data);

    void     (* present)         (GdlDockObject    *object,
                                  GdlDockObject    *child);

    gboolean (* child_placement) (GdlDockObject    *object,
                                  GdlDockObject    *child,
                                  GdlDockPlacement *placement);

    GList*   (* children)        (GdlDockObject    *object);
    void     (* foreach_child)   (GdlDockObject    *object,
                                  GdlDockObjectFn, gpointer);
    void     (* remove)          (GdlDockObject    *object,
                                  GtkWidget* widget);
    /**
     *  remove_widgets should remove private child widgets (not including any Dock widgets)
     */
    void     (* remove_widgets)  (GdlDockObject    *object);
#ifdef DEBUG
    bool     (* validate)        (GdlDockObject    *object);
#endif
};

/* additional macros */

#ifndef GDL_DISABLE_DEPRECATED
/**
 * GDL_DOCK_OBJECT_FLAGS:
 * @obj: A #GdlDockObject
 *
 * Get all flags of #GdlDockObject.
 *
 * Deprecated: 3.6: The flags are not accessible anymore.
 */
#define GDL_DOCK_OBJECT_FLAGS(obj)  (GDL_DOCK_OBJECT (obj)->deprecated_flags)
#endif

#ifndef GDL_DISABLE_DEPRECATED
/**
 * GDL_DOCK_OBJECT_AUTOMATIC:
 * @obj: A #GdlDockObject
 *
 * Evaluates to %TRUE if the object's lifecycle is entirely managed by the dock
 * master.
 *
 * Deprecated: 3.6: Use gdl_dock_object_is_automatic()
 */
#define GDL_DOCK_OBJECT_AUTOMATIC(obj) gdl_dock_object_is_automatic (GDL_DOCK_OBJECT (obj))
#endif

#ifndef GDL_DISABLE_DEPRECATED
/**
 * GDL_DOCK_OBJECT_ATTACHED:
 * @obj: A #GdlDockObject
 *
 * Evaluates to %TRUE if the object has a parent.
 *
 * Deprecated: 3.6: Use
 */
#define GDL_DOCK_OBJECT_ATTACHED(obj) (!gdl_dock_object_is_closed(GDL_DOCK_OBJECT (obj)))
#endif

#ifndef GDL_DISABLE_DEPRECATED
/**
 * GDL_DOCK_OBJECT_IN_REFLOW:
 * @obj: A #GdlDockObject
 *
 * Evaluates to %TRUE if the object is currently rearranged.
 *
 * Deprecated: 3.6: Use gdl_dock_object_is_frozen()
 */
#define GDL_DOCK_OBJECT_IN_REFLOW(obj) \
    ((GDL_DOCK_OBJECT_FLAGS (obj) & GDL_DOCK_IN_REFLOW) != 0)
#endif

#ifndef GDL_DISABLE_DEPRECATED
/**
 * GDL_DOCK_OBJECT_IN_DETACH:
 * @obj: A #GdlDockObject
 *
 * Evaluates to %TRUE if the object will be detached.
 *
 * Deprecated: 3.6: This flag is no longer available
 */
#define GDL_DOCK_OBJECT_IN_DETACH(obj) \
    ((GDL_DOCK_OBJECT_FLAGS (obj) & GDL_DOCK_IN_DETACH) != 0)
#endif

#ifndef GDL_DISABLE_DEPRECATED
/**
 * GDL_DOCK_OBJECT_SET_FLAGS:
 * @obj: A #GdlDockObject
 * @flag: One or more #GdlDockObjectFlags
 *
 * Set one or more flags of a dock object.
 *
 * Deprecated: 3.6: This flags are no longer accessible.
 */
#define GDL_DOCK_OBJECT_SET_FLAGS(obj,flag) \
    G_STMT_START { (GDL_DOCK_OBJECT_FLAGS (obj) |= (flag)); } G_STMT_END
#endif


#ifndef GDL_DISABLE_DEPRECATED
/**
 * GDL_DOCK_OBJECT_UNSET_FLAGS:
 * @obj: A #GdlDockObject
 * @flag: One or more #GdlDockObjectFlags
 *
 * Clear one or more flags of a dock object.
 *
 * Deprecated: 3.6: This flags are no longer accessible.
 */
#define GDL_DOCK_OBJECT_UNSET_FLAGS(obj,flag) \
    G_STMT_START { (GDL_DOCK_OBJECT_FLAGS (obj) &= ~(flag)); } G_STMT_END
#endif


/* public interface */

GType          gdl_dock_object_get_type          (void);

gboolean       gdl_dock_object_is_compound       (GdlDockObject    *object);

void           gdl_dock_object_detach            (GdlDockObject    *object,
                                                  gboolean          recursive);
void           gdl_dock_object_destroy           (GdlDockObject    *object);
void           gdl_dock_object_add_child         (GdlDockObject    *object, GtkWidget* child);
void           gdl_dock_object_remove_child      (GdlDockObject    *object, GtkWidget* child);

GdlDockObject *gdl_dock_object_get_parent_object (GdlDockObject    *object);
GList*         gdl_dock_object_get_children      (GdlDockObject    *object);
void           gdl_dock_object_foreach_child     (GdlDockObject    *object, GdlDockObjectFn, gpointer);

void           gdl_dock_object_freeze            (GdlDockObject    *object);
void           gdl_dock_object_thaw              (GdlDockObject    *object);
gboolean       gdl_dock_object_is_frozen          (GdlDockObject    *object);

void           gdl_dock_object_reduce            (GdlDockObject    *object);

gboolean       gdl_dock_object_dock_request      (GdlDockObject    *object,
                                                  gint              x,
                                                  gint              y,
                                                  GdlDockRequest   *request);
void           gdl_dock_object_dock              (GdlDockObject    *object,
                                                  GdlDockObject    *requestor,
                                                  GdlDockPlacement  position,
                                                  GValue           *other_data);

void           gdl_dock_object_bind              (GdlDockObject    *object,
                                                  GObject          *master);
void           gdl_dock_object_unbind            (GdlDockObject    *object);
gboolean       gdl_dock_object_is_bound          (GdlDockObject    *object);
GObject       *gdl_dock_object_get_master        (GdlDockObject    *object);
GdlDockObject *gdl_dock_object_get_controller    (GdlDockObject    *object);
void           gdl_dock_object_layout_changed_notify (GdlDockObject *object);

gboolean       gdl_dock_object_reorder           (GdlDockObject    *object,
                                                  GdlDockObject    *child,
                                                  GdlDockPlacement  new_position,
                                                  GValue           *other_data);

void           gdl_dock_object_present           (GdlDockObject    *object,
                                                  GdlDockObject    *child);

gboolean       gdl_dock_object_child_placement   (GdlDockObject    *object,
                                                  GdlDockObject    *child,
                                                  GdlDockPlacement *placement);

gboolean       gdl_dock_object_is_closed         (GdlDockObject    *object);

gboolean       gdl_dock_object_is_automatic      (GdlDockObject    *object);
void           gdl_dock_object_set_manual        (GdlDockObject    *object);

const gchar   *gdl_dock_object_get_name          (GdlDockObject    *object);
void           gdl_dock_object_set_name          (GdlDockObject    *object,
                                                  const gchar      *name);
const gchar   *gdl_dock_object_get_long_name     (GdlDockObject    *object);
void           gdl_dock_object_set_long_name     (GdlDockObject    *object,
                                                  const gchar      *name);
const gchar   *gdl_dock_object_get_stock_id      (GdlDockObject    *object);
void           gdl_dock_object_set_stock_id      (GdlDockObject    *object,
                                                  const gchar      *stock_id);
GdkPixbuf     *gdl_dock_object_get_pixbuf        (GdlDockObject    *object);
void           gdl_dock_object_set_pixbuf        (GdlDockObject    *object,
                                                  GdkPixbuf        *icon);

void          gdl_dock_object_class_set_is_compound (GdlDockObjectClass *object_class,
                                                     gboolean           is_compound);


/* this type derives from G_TYPE_STRING and is meant to be the basic
   type for serializing object parameters which are exported
   (i.e. those that are needed for layout rebuilding) */
#define GDL_TYPE_DOCK_PARAM   (gdl_dock_param_get_type ())

GType gdl_dock_param_get_type (void)  __attribute__ ((no_instrument_function));

/* functions for setting/retrieving nick names for serializing GdlDockObject types */
const gchar          *gdl_dock_object_nick_from_type    (GType        type);
GType                 gdl_dock_object_type_from_nick    (const gchar *nick);
GType                 gdl_dock_object_set_type_for_nick (const gchar *nick,
                                                         GType        type);


/* helper macros */
/**
 * GDL_TRACE_OBJECT:
 * @object: A #GdlDockObject
 * @format: Additional printf format string
 * @...: Additional arguments
 *
 * Output a debugging message for the corresponding dock object.
 */
#define GDL_TRACE_OBJECT(object, format, args...) \
    G_STMT_START {                            \
    g_log (G_LOG_DOMAIN,                      \
	   G_LOG_LEVEL_DEBUG,                 \
           "%s:%d (%s) %s [%p %d%s:%d]: " format, \
	   __FILE__,                          \
	   __LINE__,                          \
	   __PRETTY_FUNCTION__,               \
           G_OBJECT_TYPE_NAME (object), object, \
           G_OBJECT (object)->ref_count, \
           (GTK_IS_OBJECT (object) && g_object_is_floating (object)) ? "(float)" : "", \
           GDL_IS_DOCK_OBJECT (object) ? gdl_dock_object_is_frozen (GDL_DOCK_OBJECT (object)) : -1, \
	   ##args); } G_STMT_END

G_END_DECLS
