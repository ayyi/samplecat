#include "config.h"
#include <stdio.h>
#include <stdlib.h>
//#define __USE_GNU
#include <string.h>
#include <unistd.h>
#include <sndfile.h>
#include <gtk/gtk.h>
#include "ayyi.h"

void
ayyi_connect()
{
	Service* ardourd = &known_services[0];

	ayyi_shm_init();
	shm_seg_new(0, 0);

	GError* error = NULL;
	if((dbus_server_connect (ardourd, &error))){
		ardourd->on_shm = on_shm;
		dbus_server_get_shm(ardourd);
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
	//return TRUE;
}


void
action_free(AyyiAction* ayyi_action)
{
#if 0
  ASSERT_POINTER(ayyi_action, "action");
  action* action = ayyi_action->app_data;
  ASSERT_POINTER(actionlist, "actionlist");
  dbg (1, "list len: %i", g_list_length(actionlist));
  if(action) dbg (2, "id=%i %s%s%s", ayyi_action->trid, bold, strlen(action->label) ? action->label : "<unnamed>", white);

  actionlist = g_list_remove(actionlist, ayyi_action);
  if(action && action->pending) g_list_free(action->pending);
  if(action) g_free(action);
  g_free(ayyi_action);
  dbg (1, "new list len: %i", g_list_length(actionlist));
#endif
}


void
action_complete(AyyiAction* ayyi_action)
{
#if 0
  //actions are now freed here and (ideally) only here.

  ASSERT_POINTER(ayyi_action, "action");

  action* action = ayyi_action->app_data;
  dbg(2, "action=%p", action);
  if(action && action->callback){
    (*(action->callback))(ayyi_action);
  }

  action_free(ayyi_action);
#endif
}
