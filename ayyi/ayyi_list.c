/*
  This file is part of the Ayyi Project. http://ayyi.org
  copyright (C) 2004-2009 Tim Orford <tim@orford.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 3
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*
  An AyyiList is a single linked list living only in shared memory. It is read-only for Ayyi clients.

*/
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <glib.h>
#include <jack/jack.h>

typedef void action;
#include <ayyi/ayyi_typedefs.h>
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/interface.h>
#include <ayyi/ayyi_msg.h>
#include <ayyi/ayyi_client.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_list.h>

void*    ayyi_song__translate_address(void* address);


int
ayyi_list__length(AyyiList* list)
{
#warning ayyi_list__length gives wrong result.
	guint length = 0;
	list = ayyi_song__translate_address(list);
	while(list){
		length++;
		list = ayyi_song__translate_address(list->next);
	}

	return length;
}


AyyiList*
ayyi_list__first(AyyiList* list)
{
	return ayyi_song__translate_address(list);
}


AyyiList*
ayyi_list__next(AyyiList* list)
{
	if(!list) return NULL;

	//dbg(0, "%p --> %p", list, ayyi_song__translate_address(list->next));
	return ayyi_song__translate_address(list->next);
}


void*
ayyi_list__first_data(AyyiList** list_p)
{
	AyyiList* list = *list_p;
	if(!list) return NULL;

	list = *list_p = ayyi_song__translate_address(list);

	dbg(0, "  list_p=%p list=%p container=%p", list_p, *list_p, list ? ayyi_song__translate_address(list->data) : NULL);
	return list ? ayyi_song__translate_address(list->data) : NULL;
}


void*
ayyi_list__next_data(AyyiList** list_p)
{
	dbg(3, "  list_p=%p list=%p", list_p, *list_p);
	AyyiList* list = *list_p;
	if(!list) return NULL;

	list = *list_p = ayyi_song__translate_address(list->next);

	return list ? ayyi_song__translate_address(list->data) : NULL;
}


AyyiList*
ayyi_list__find(AyyiList* list, uint32_t item)
{
	AyyiList* i = ayyi_list__first(list);
	if(i)
		do {
			uint32_t id = i->id;
			dbg(3, "  id=%i %s", id, (id == item) ? "found" : "");
			if(id == item) return i;
		} while((i = ayyi_list__next(i)));
	return NULL;
}


