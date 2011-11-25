/*
  copyright (C) 2007-2011 Tim Orford <tim@orford.org>

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
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
//#define __USE_GNU
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "ayyi.h"
#include "ayyi/engine_proxy.h"

Service* engine = &known_services[AYYI_SERVICE_AYYID1];

void
ayyi_connect()
{
	ayyi_shm_init();
	ayyi_shm_seg_new(0, SEG_TYPE_SONG);

	GError* error = NULL;
	if((ayyi_client_connect (engine, &error))){
		ardourd->on_shm = on_shm;
		ayyi_client__dbus_get_shm(ardourd, NULL);
	}else{
		P_GERR;
		dbg (0, "ayyi dbus connection failed.");
	}
}


void
on_shm(struct _shm_seg* seg)
{
	PF;
	if (ayyi.got_shm) {
		//synchronise local data cache.
		//ayyi_song__load();
	}
}


