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
	Service* ardourd = &known_services[0];

	ayyi_shm_init();
	ayyi_shm_seg_new(0, SEG_TYPE_SONG);

	GError* error = NULL;
	if((ayyi_client_connect (ardourd, &error))){
		ardourd->on_shm = on_shm;
		ayyi_client__dbus_get_shm(ardourd, NULL);
		//dbus_register_signals();
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


