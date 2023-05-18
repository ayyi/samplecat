/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "gobject/gvaluecollector.h"
#include "types.h"

GType AYYI_TYPE_COLOUR = 0;


static void
value_init_long0 (GValue *value)
{
	value->data[0].v_long = 0;
}


static void
value_copy_long0 (const GValue* src_value, GValue* dest_value)
{
	dest_value->data[0].v_long = src_value->data[0].v_long;
}


static gchar*
value_collect_int (GValue* value, guint n_collect_values, GTypeCValue *collect_values, guint collect_flags)
{
	value->data[0].v_int = collect_values[0].v_int;

	return NULL;
}


static gchar*
value_lcopy_int (const GValue* value, guint n_collect_values, GTypeCValue* collect_values, guint collect_flags)
{
	gint *int_p = collect_values[0].v_pointer;

	g_return_val_if_fail (int_p != NULL, g_strdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

	*int_p = value->data[0].v_int;

	return NULL;
}


void
types_init ()
{
	static const GTypeValueTable value_table = {
		.value_init = value_init_long0,
		.value_copy = value_copy_long0,
		.collect_format = "i",
		.collect_value = value_collect_int,
		.lcopy_format = "p",
		.lcopy_value = value_lcopy_int,
	};

	AYYI_TYPE_COLOUR = g_type_register_fundamental (
		g_type_fundamental_next (),
		"colour",
		&(GTypeInfo){
			.value_table = &value_table
		},
		&(GTypeFundamentalInfo){ G_TYPE_FLAG_DERIVABLE, },
		0
	);
}
