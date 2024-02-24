/* GTK - The GIMP Toolkit
 * Copyright (C) 2019 Red Hat, Inc.
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

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DEMO_TYPE_TAGGED_ENTRY (demo_tagged_entry_get_type ())
G_DECLARE_FINAL_TYPE (DemoTaggedEntry, demo_tagged_entry, DEMO, TAGGED_ENTRY, GtkWidget)

#define DEMO_TYPE_TAGGED_ENTRY_TAG (demo_tagged_entry_tag_get_type ())
G_DECLARE_FINAL_TYPE (DemoTaggedEntryTag, demo_tagged_entry_tag, DEMO, TAGGED_ENTRY_TAG, GtkWidget)

#define DEMO_TYPE_TAGGED_ENTRY_TAB               (demo_tagged_entry_tag_get_type ())
#define DEMO_IS_TAGGED_ENTRY_TAB(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DEMO_TYPE_TAGGED_ENTRY_TAB))

GtkWidget*      demo_tagged_entry_new              (void);

GPtrArray*      tagged_entry_get_tags              (DemoTaggedEntry*);
void            tagged_entry_set_tags_from_string  (DemoTaggedEntry*, const char*);
void            demo_tagged_entry_add_tag          (DemoTaggedEntry*, GtkWidget* tag);
void            demo_tagged_entry_insert_tag_after (DemoTaggedEntry*, GtkWidget* tag, GtkWidget*);
void            demo_tagged_entry_remove_tag       (DemoTaggedEntry*, GtkWidget* tag);

DemoTaggedEntryTag*
                demo_tagged_entry_tag_new          (const char*);
const char*     demo_tagged_entry_tag_get_label    (DemoTaggedEntryTag*);
void            demo_tagged_entry_tag_set_label    (DemoTaggedEntryTag*, const char* label);

gboolean        demo_tagged_entry_tag_get_has_close_button (DemoTaggedEntryTag*);
void            demo_tagged_entry_tag_set_has_close_button (DemoTaggedEntryTag*, bool);

G_END_DECLS
