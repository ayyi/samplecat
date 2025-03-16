/*
 +----------------------------------------------------------------------+
 | This file is part of AyyiDock                                        |
 | copyright (C) 2007-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include "gdl-dock-item.h"
#include "registry.h"

typedef struct {
	gchar          name[32];
	GObjectClass*  class;
	DockParameter  params[10];
	int            n_params;
} DockItemSpec;

typedef struct {
	DockItemSpec   spec;
	bool           done;
} Constructor;

typedef struct {
	Constructor    items[20];
	GdlDockObject* objects[20];
	int            sp;
	GdlDockItem*   added[20];   // newly created objects. used to notify the client once the layout build is complete
	int            added_sp;
	GdlDockMaster* master;
	GdlDockObject* parent;
} Stack;

GdlDockItem* dock_item_factory      (Stack*);
const char*  dock_item_factory_name (DockItemSpec*);
