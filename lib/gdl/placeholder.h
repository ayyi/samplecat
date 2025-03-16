/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://samplecat.orford.org         |
 | copyright (C) 2025-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <glib-object.h>
#include "gdl/utils.h"
#include "gdl/factory.h"

G_BEGIN_DECLS

#define DOCK_TYPE_PLACEHOLDER            (dock_placeholder_get_type ())
#define DOCK_PLACEHOLDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DOCK_TYPE_PLACEHOLDER, DockPlaceholder))
#define DOCK_PLACEHOLDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DOCK_TYPE_PLACEHOLDER, DockPlaceholderClass))
#define DOCK_IS_PLACEHOLDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DOCK_TYPE_PLACEHOLDER))
#define DOCK_IS_PLACEHOLDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DOCK_TYPE_PLACEHOLDER))
#define DOCK_PLACEHOLDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DOCK_TYPE_PLACEHOLDER, DockPlaceholderClass))

typedef struct _DockPlaceholder        DockPlaceholder;
typedef struct _DockPlaceholderClass   DockPlaceholderClass;

struct _DockPlaceholder
{
	GdlDockItem    parent_instance;

	DockItemSpec   spec;
};

struct _DockPlaceholderClass {
	GdlDockItemClass parent_class;
};

GType            dock_placeholder_get_type (void) G_GNUC_CONST;
GdlDockItem*     dock_placeholder_fill     (DockPlaceholder*);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (DockPlaceholder, g_object_unref)

G_END_DECLS
