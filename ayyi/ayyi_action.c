#include "config.h"
#include <stdint.h>
#include <glib.h>
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_typedefs.h>
#include <ayyi/engine_proxy.h>
#include <ayyi/ayyi_utils.h>

extern int debug;

GList* actionlist = NULL;  // list of AyyiAction
unsigned trans_id = 1;


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

	we could scrap this fn and just use engine_set_obj() instead?

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
engine_set_obj(AyyiAction* action, uint32_t object_type, uint32_t _obj_id, int _prop, uint64_t _val)
{
#ifdef USE_DBUS
	switch(_prop){
		case AYYI_MUTE:
		case AYYI_TRACK:
			dbus_set_prop_int(action, object_type, _prop, _obj_id, _val);
			break;
		case AYYI_SPLIT:
		case AYYI_INSET:
			dbus_set_prop_int64(action, AYYI_OBJECT_AUDIO_PART, _prop, _obj_id, _val);
			break;
		case AYYI_SAVE:
			dbus_set_prop_int(action, object_type, _prop, _obj_id, _val);
			break;
		default:
			gwarn ("unexpected property: %i", _prop);
			return -1;
	}
	return 0;
#endif
}


int
engine_set_string(AyyiAction* action, uint32_t object_type, uint32_t idx, const char* _string)
{
#ifdef USE_DBUS
	//printf("%s(): object_type=%i\n", __func__, object_type);
	dbus_set_prop_string(action, object_type, AYYI_NAME, idx, _string);
	return 0;
#endif
}


