/*
  This file is part of the Ayyi Project. http://ayyi.org
  copyright (C) 2004-2010 Tim Orford <tim@orford.org>

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
#define __ayyi_action_c__
#include "config.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <glib.h>
#include <ayyi/ayyi_typedefs.h>
#include <ayyi/ayyi_types.h>
#include <ayyi/engine_proxy.h>
#include <ayyi/ayyi_msg.h>
#include <ayyi/ayyi_client.h>
#include <ayyi/ayyi_utils.h>

extern int debug;


AyyiAction*
ayyi_action_new(char* format, ...)
{
	va_list argp;
	va_start(argp, format);

	AyyiAction* a = g_new0(AyyiAction, 1);
	a->trid = ayyi.priv->trans_id++;
	a->op   = AYYI_SET;

	vsnprintf(a->label, 63, format, argp);
	gettimeofday(&a->time_stamp, NULL);

	ayyi.priv->actionlist = g_list_append(ayyi.priv->actionlist, a);
	printf("%saction%s: %s\n", yellow_r, white, a->label);

	va_end(argp);

	return a;
}


AyyiAction*
ayyi_action_new_pending(GList* pending_list)
{
	// create an action that is dependant on other actions being completed first.
	PF;
	AyyiAction* a = ayyi_action_new("");
	a->pending = pending_list;
	return a;
}


void
ayyi_action_free(AyyiAction* ayyi_action)
{
  g_return_if_fail(ayyi_action);
  g_return_if_fail(ayyi.priv->actionlist);
  dbg (1, "list len: %i", g_list_length(ayyi.priv->actionlist));
  dbg (2, "id=%i %s%s%s", ayyi_action->trid, bold, strlen(ayyi_action->label) ? ayyi_action->label : "<unnamed>", white);

  ayyi.priv->actionlist = g_list_remove(ayyi.priv->actionlist, ayyi_action);
  if(ayyi_action->pending) g_list_free(ayyi_action->pending);
  if(ayyi_action->app_data) g_free(ayyi_action->app_data);
  g_free(ayyi_action);
  dbg (1, "new list len: %i", g_list_length(ayyi.priv->actionlist));
}


void
ayyi_action_execute(AyyiAction* a)
{
	switch(a->op){
		case AYYI_NEW:
			gwarn("AYYI_NEW: FIXME not implemented");
			break;
		case AYYI_GET:
			gwarn("AYYI_GET: FIXME not implemented");
			break;
		case AYYI_SET:
			switch(a->prop){
				case AYYI_LENGTH:
				case AYYI_AUTO_PT:
				case AYYI_HEIGHT:
					gwarn("AYYI_SET: FIXME not implemented");
					break;
				case AYYI_TRANSPORT_REC:
				case AYYI_ARM:
				case AYYI_SOLO:
					#if USE_DBUS
					dbus_set_prop_bool(a, a->obj_type, a->prop, a->obj_idx.idx1, a->i_val);
					#endif
					break;
				default:
					//integer properties:
					dbus_set_prop_int(a, a->obj_type, a->prop, a->obj_idx.idx1, a->i_val);
					return;
			}
			break;

		case AYYI_DEL:
			#ifdef USE_DBUS
			dbus_object_del(a, a->obj_type, a->obj_idx.idx1);
			return;
			#endif
			break;

		default:
			gerr ("unsupported op type: %i", a->op);
			break;
	}
}


void
ayyi_action_complete(AyyiAction* action)
{
  //actions are now freed here and (ideally) only here.

  g_return_if_fail(action);

  if(action->callback) (*(action->callback))(action);

  ayyi_action_free(action);
}


AyyiAction*
ayyi_action_lookup_by_id(unsigned trid)
{
	//returns the action struct if the given trid id is in the actionlist, or NULL if not.

	if(ayyi.priv->actionlist){
		GList* l = ayyi.priv->actionlist;
		for(;l;l=l->next){
			AyyiAction* a = l->data;
			if(a->trid == trid){
				return a;
			}
		}
		gerr ("trid not found! (%i)", trid);
	}

	return NULL;
}


gboolean
ayyi_check_actions(gpointer data)
{
  //check that message requests are not timing out.

  struct timeval now;
  gettimeofday(&now, NULL);

  GList* l = ayyi.priv->actionlist;
  if(!l) return TRUE;
  for(;l;l=l->next){
    AyyiAction* action = l->data;

    if(now.tv_sec - action->time_stamp.tv_sec > 5){
      //the idea is to call the normal callback. The callback should handle the error itself...

      if(ayyi.debug>-1) if(!action->trid || action->trid > ayyi.priv->trans_id){ gerr ("bad trid? %i", action->trid); continue; }
      dbg(0, "timeout! calling callback... trid=%u '%s'", action->trid, action->label);
      if(action->callback){
        action->ret.err = 1; //FIXME define a number for timeouts.
        (*(action->callback))(action);
      }
      else ayyi_action_free(action);

      break; //only do one at a time to avoid problems with iterating over a changing list.
    }
  }

  return TRUE;
}


void
ayyi_action_print_status()
{
	log_print(0, "outstanding requests: %i", g_list_length(ayyi.priv->actionlist));
}


// set length of object
int
action_set_length(AyyiAction* action, uint64_t len)
{
	// @param _obj_id should *not* have any masks applied to it (except perhaps for container/block indexing)

	//we should probably use ardour_set_obj() instead.

#ifdef USE_DBUS
	dbus_set_prop_int64(action, action->obj_type, AYYI_LENGTH, action->obj_idx.idx1, len);
#endif
	return 0;
}


int
ayyi_set_float(AyyiAction* action, uint32_t object_type, AyyiIdx obj_idx, int _prop, double _val)
{
#ifdef USE_DBUS
	dbus_set_prop_float(action, object_type, _prop, obj_idx, _val);
	return 0;
#endif
}


#if 0
int
engine_set_start_time(AyyiAction* ayyi_action, gpart* gpart, int _sample)
{
	/*

	currently we are not using the ardour id as an identifier, as it is 64 bit....

	unlike other functions, we are using the sample number!
	what time format should we use? with a view to universality, we should perhaps use the higher resolution song_pos.

	*/
	ASSERT_POINTER_FALSE(gpart, "gpart");

	if (!ayyi.song) return -1;
	unsigned idx = gpart->idx;

	ayyi_action->op  = AYYI_SET;
	ayyi_action->obj_type = PART_IS_AUDIO ? AYYI_OBJECT_AUDIO_PART : AYYI_OBJECT_MIDI_PART;
	action* action   = ayyi_action->app_data;
	action->obj_idx  = idx;
	action->prop     = AYYI_STIME;
	action->i_val    = _sample;

	return action_execute(ayyi_action);
}


int
engine_set_track(AyyiAction* action, gpart* gpart, int _track)
{
	/*

	currently we are not using the ardour id as an identifier, as it is 64 bit....

	currently parts are the only objects which live on tracks, so this fn sig should do ok for now.

	we could scrap this fn and just use ayyi_set_obj() instead?

	*/
	ASSERT_POINTER_FALSE(gpart, "gpart");
	if (!ayyi.song) return -1;
	dbg (0, "track=%u", _track);

	unsigned idx = gpart->idx;

#ifdef USE_DBUS
	dbus_set_prop_int(action, AYYI_OBJECT_AUDIO_PART, AYYI_TRACK, idx, _track);
#endif
	return 0;
}
#endif


// returns 0 if ok (ie msg sent)
// returns -1 if obj not found
// returns -2 if obj TYPE or SUBTYPE incorrect for given property
int
ayyi_set_obj(AyyiAction* action, uint32_t object_type, uint32_t obj_id, int prop, uint64_t val)
{
#ifdef USE_DBUS
	switch(prop){
		case AYYI_MUTE:
		case AYYI_TRACK:
			dbus_set_prop_int(action, object_type, prop, obj_id, val);
			break;
		case AYYI_SPLIT:
		case AYYI_INSET:
			dbus_set_prop_int64(action, AYYI_OBJECT_AUDIO_PART, prop, obj_id, val);
			break;
		case AYYI_LOAD_SONG:
		case AYYI_SAVE:
			dbus_set_prop_int(action, object_type, prop, obj_id, val);
			break;
		default:
			gwarn ("unexpected property: %s (%i)", ayyi_print_prop_type(prop), prop);
			return -1;
	}
	return 0;
#endif
}


int
ayyi_set_string(AyyiAction* action, uint32_t object_type, uint32_t idx, const char* _string)
{
#ifdef USE_DBUS
	//printf("%s(): object_type=%i\n", __func__, object_type);
	dbus_set_prop_string(action, object_type, AYYI_NAME, idx, _string);
	return 0;
#endif
}


/*
const char*
ayyi_print_prop_type(uint32_t prop_type)
{
	static char* types[] = {"AYYI_NO_PROP", "AYYI_NAME", "AYYI_STIME", "AYYI_LENGTH", "AYYI_HEIGHT", "AYYI_COLOUR", "AYYI_END", "AYYI_TRACK", "AYYI_MUTE", "AYYI_ARM", "AYYI_SOLO", 
	                        "AYYI_SDEF", "AYYI_INSET", "AYYI_FADEIN", "AYYI_FADEOUT", "AYYI_INPUT", "AYYI_OUTPUT",
	                        "AYYI_AUX1_OUTPUT", "AYYI_AUX2_OUTPUT", "AYYI_AUX3_OUTPUT", "AYYI_AUX4_OUTPUT",
	                        "AYYI_PREPOST", "AYYI_SPLIT",
	                        "AYYI_PB_LEVEL", "AYYI_PB_PAN", "AYYI_PB_DELAY_MU", "AYYI_PLUGIN_SEL", "AYYI_PLUGIN_BYPASS",
	                        "AYYI_CHAN_LEVEL", "AYYI_CHAN_PAN",
	                        "AYYI_TRANSPORT_PLAY", "AYYI_TRANSPORT_STOP", "AYYI_TRANSPORT_REW", "AYYI_TRANSPORT_FF",
	                        "AYYI_TRANSPORT_REC", "AYYI_TRANSPORT_LOCATE", "AYYI_TRANSPORT_CYCLE", "AYYI_TRANSPORT_LOCATOR",
	                        "AYYI_AUTO_PT",
	                        "AYYI_ADD_POINT", "AYYI_DEL_POINT",
	                        "AYYI_TEMPO", "AYYI_HISTORY",
	                        "AYYI_LOAD_SONG", "AYYI_SAVE", "AYYI_NEW_SONG"
	                       };
	if(G_N_ELEMENTS(types) != AYYI_NEW_SONG + 1) err("!!\n");
	return types[prop_type];
}
*/

