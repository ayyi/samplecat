#include "config.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <glib.h>
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_typedefs.h>
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
	a->trid = ayyi.trans_id++;
	a->op   = AYYI_SET;

	vsnprintf(a->label, 63, format, argp);
	gettimeofday(&a->time_stamp, NULL);

	ayyi.actionlist = g_list_append(ayyi.actionlist, a);
	printf("%s%s%s(): %s\n", yellow, __func__, white, a->label);

	va_end(argp);

	return a;
}


void
ayyi_action_free(AyyiAction* ayyi_action)
{
  ASSERT_POINTER(ayyi_action, "action");
  void* action = ayyi_action->app_data;
  ASSERT_POINTER(ayyi.actionlist, "actionlist");
  dbg (1, "list len: %i", g_list_length(ayyi.actionlist));
  dbg (2, "id=%i %s%s%s", ayyi_action->trid, bold, strlen(ayyi_action->label) ? ayyi_action->label : "<unnamed>", white);

  ayyi.actionlist = g_list_remove(ayyi.actionlist, ayyi_action);
  if(ayyi_action->pending) g_list_free(ayyi_action->pending);
  if(action) g_free(ayyi_action->app_data);
  g_free(ayyi_action);
  dbg (1, "new list len: %i", g_list_length(ayyi.actionlist));
}


void
ayyi_action_execute(AyyiAction* action)
{
	switch(action->op){
		case AYYI_NEW:
			gwarn("FIXME");
			break;
		case AYYI_GET:
			gwarn("FIXME");
			break;
		case AYYI_SET:
			switch(action->prop){
				case AYYI_LENGTH:
				case AYYI_AUTO_PT:
				case AYYI_TRANSPORT_REC:
				case AYYI_ARM:
				case AYYI_SOLO:
				case AYYI_HEIGHT:
					gwarn("FIXME");
					break;
				default:
					//integer properties:
					dbus_set_prop_int(action, action->obj_type, action->prop, action->obj_idx.idx1, action->i_val);
					return;
			}
			break;

		case AYYI_DEL:
			#ifdef USE_DBUS
			dbus_object_del(action, action->obj_type, action->obj_idx.idx1);
			return;
			#endif
			break;

		default:
			gerr ("unsupported op type: %i", action->op);
			break;
	}
}


void
ayyi_action_complete(AyyiAction* action)
{
  //actions are now freed here and (ideally) only here.

  ASSERT_POINTER(action, "action");

  if(action->callback) (*(action->callback))(action);

  ayyi_action_free(action);
}


// set length of object
int
action_set_length(AyyiAction* action, uint32_t _obj_id, uint64_t _len)
{
	// @param _obj_id should *not* have any masks applied to it (except perhaps for container/block indexing)

	//we should probably use ardour_set_obj() instead.

#ifdef USE_DBUS
	dbus_set_prop_int64(action, AYYI_OBJECT_AUDIO_PART, AYYI_LENGTH, _obj_id, _len);
#endif
	return 0;
}


int
ayyi_set_float(AyyiAction* action, uint32_t object_type, uint32_t _obj_id, int _prop, double _val)
{
#ifdef USE_DBUS
	dbus_set_prop_float(action, AYYI_OBJECT_AUDIO_PART, _prop, _obj_id, _val);
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
		case AYYI_SAVE:
			dbus_set_prop_int(action, object_type, prop, obj_id, val);
			break;
		default:
			gwarn ("unexpected property: %i", prop);
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


const char*
ayyi_action_print_optype (AyyiOp op)
{
	static char* ops[] = {"", "AYYI_NEW", "AYYI_DEL", "AYYI_GET", "AYYI_SET", "AYYI_UNDO", "BAD OPTYPE"};
	return ops[MIN(op, 6)];
}


