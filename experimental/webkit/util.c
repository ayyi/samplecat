/*
 * wgp-util.c: Utilities
 *
 * Copyright (C) 2010 Manuel Rego Casasnovas <mrego@igalia.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "util.h"

void
wgp_util_remove_all_children (WebKitDOMNode* parent)
{
	WebKitDOMNode* node;

	while (webkit_dom_node_has_child_nodes (parent)) {
		node =  webkit_dom_node_get_first_child (parent);
		webkit_dom_node_remove_child (parent, node, NULL);
	}
}
