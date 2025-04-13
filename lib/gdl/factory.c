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

#include "gdl/debug.h"
#include "gdl-dock-item.h"
#include "master.h"
#include "registry.h"
#include "factory.h"
					#include "placeholder.h"

#define DOCK_PARAM_CONSTRUCTION(p) \
    (((p)->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY)) != 0)


const char*
dock_item_factory_name (DockItemSpec* spec)
{
	char* name = NULL;
	for (int i=0;i<spec->n_params;i++) {
		DockParameter* param = &spec->params[i];
		GParamSpec* pspec = g_object_class_find_property(spec->class, param->name);
		if (!DOCK_PARAM_CONSTRUCTION (pspec) && (pspec->flags & GDL_DOCK_PARAM_AFTER)) {
		}
		if (!strcmp(param->name, "name"))
			name = (char*)g_value_get_string(&param->value);
	}
	return name;
}


static void
dock_specs_clear (DockItemSpec* spec)
{
	for (int i = 0; i < spec->n_params; i++) {
		g_value_unset (&spec->params[i].value);
		if (strcmp(spec->params[i].name, "long-name"))
			g_free((char*)spec->params[i].name);
	}
	spec->n_params = 0;
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (DockItemSpec, dock_specs_clear)


/*
 *  Construct the dock-item from the top of the stack and free the spec parameters
 */
GdlDockItem*
dock_item_factory (Stack* stack)
{
	Constructor* constructor = &stack->items[stack->sp];
	g_autoptr(DockItemSpec) spec = &constructor->spec;
	g_return_val_if_fail(spec->class, NULL);

	bool parent_is_notebook = stack->sp
		? !strcmp(stack->items[stack->sp - 1].spec.name, "notebook")
		: false;

	const char* name = dock_item_factory_name(spec);

	GdlDockObject* object = name ? gdl_dock_master_get_object(stack->master, name) : NULL;
	if (object) {
		/* set the parameters to the existing object */
		pwarn("existing object found NEVER GET HERE");

		for (int i = 0; i < spec->n_params; i++)
			if (strcmp(spec->params[i].name, "name"))
				g_object_set_property (G_OBJECT(object), spec->params[i].name, &spec->params[i].value);

		return (GdlDockItem*)object;
	}

	GtkWidget* child = NULL;
	bool need_child = true;

	{
		if (parent_is_notebook) {
			// notebook items are not created until the page is selected
			need_child = false;

			#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
			DockPlaceholder* p = g_object_newv (DOCK_TYPE_PLACEHOLDER, spec->n_params, (GParameter*)spec->params);
			#pragma GCC diagnostic warning "-Wdeprecated-declarations"

			void move_spec (DockItemSpec* dest, DockItemSpec* src)
			{
				memcpy(dest, src, sizeof(DockItemSpec));
				memset(src->params, 0, 10 * sizeof(DockParameter));
				src->n_params = 0;
			}
			move_spec(&p->spec, spec);

			object = GDL_DOCK_OBJECT(p);
		} else {
			if (name) {
				GType gtype = g_type_from_name(name);
				if (gtype) {
					void* klass = g_type_class_ref (gtype);
					if (GDL_IS_DOCK_ITEM_CLASS (klass)) {
						#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
						object = g_object_newv (gtype, spec->n_params, (GParameter*)spec->params);
						#pragma GCC diagnostic warning "-Wdeprecated-declarations"
					} else if (GTK_IS_WIDGET_CLASS (klass)) {
						object = g_object_new(gtype, NULL);
					} else {
						cdbg(1, "class is not widget (%s)", name);
					}
					if (object) {
						if (GDL_IS_DOCK_ITEM(object)) {
							need_child = false;
						} else {
							child = (GtkWidget*)object;
							object = NULL;
						}
					}
				}
			}
			if (need_child)
				#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
				object = g_object_newv (G_TYPE_FROM_CLASS(spec->class), spec->n_params, (GParameter*)spec->params);
				#pragma GCC diagnostic warning "-Wdeprecated-declarations"
		}

		if (GDL_IS_DOCK_ITEM(object) && !gdl_dock_object_is_compound(object))
			stack->added[stack->added_sp++] = (GdlDockItem*)object;

		if (need_child) {
			if (!child) {
				RegistryItem* item = gdl_dock_object_get_name(object) ? g_hash_table_lookup(registry, gdl_dock_object_get_name(object)) : NULL;
				if (item)
					child = item->info.gtkfn();
				else
					if (G_TYPE_FROM_CLASS(spec->class) == GDL_TYPE_DOCK_ITEM)
						pwarn("dont have either gtype or registry entry");
			}

			if (child) {
				GDL_DOCK_OBJECT_UNSET_FLAGS(object, GDL_DOCK_AUTOMATIC);

				gdl_dock_object_add_child(object, child);
			}
		}

		/*
		 *  The 'master' property has to be set _after_ construction so that
		 *  the 'automatic' property can be correctly set beforehand.
		 */
		gdl_dock_object_bind (object, (GObject*)stack->master);

		constructor->done = true;
		spec->class = NULL;
	}

	return (GdlDockItem*)object;
}
