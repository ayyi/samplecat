/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __DH_LINK_H__ 
#define __DH_LINK_H__ 

#include <glib.h>

typedef struct _DhLink   DhLink;

#define DH_LINK(x) ((DhLink *) x)

typedef enum {
	DH_LINK_TYPE_BOOK,
	DH_LINK_TYPE_PAGE,
	DH_LINK_TYPE_KEYWORD
} DhLinkType;

struct _DhLink {
	gchar       *name;
	gchar       *uri;
	DhLinkType   type;
	
	guint        ref_count;
};

DhLink * dh_link_new        (DhLinkType      type,
			     const gchar    *name,
			     const gchar    *uri);

DhLink * dh_link_copy       (const DhLink   *link);

void     dh_link_free       (DhLink         *link);

gint     dh_link_compare    (gconstpointer   a,
			     gconstpointer   b);

DhLink * dh_link_ref        (DhLink         *link);
void     dh_link_unref      (DhLink         *link);

#endif /* __DH_LINK_H__ */


