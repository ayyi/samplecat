#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <ayyi/dbus_marshallers.h>
#include <ayyi/dbus_marshallers.c>

typedef struct _song_pos song_pos;
typedef struct _shm_seg  shm_seg;
typedef void             action;
typedef struct _ayyi_action AyyiAction;
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/ayyi_msg.h>
#include <ayyi/ayyi_client.h>
#include <ayyi/ayyi_dbus.h>
void        action_complete          (struct _ayyi_action*); //FIXME
void        action_free              (struct _ayyi_action*); //FIXME

extern int debug;

DBusGProxy *proxy = NULL;

static void     create_object_notify     (DBusGProxy*, DBusGProxyCall*, gpointer data);
static void     delete_object_notify     (DBusGProxy*, DBusGProxyCall*, gpointer data);
static void     dbus_set_prop_notify     (DBusGProxy*, DBusGProxyCall*, gpointer data);
static void     dbus_get_prop_notify     (DBusGProxy*, DBusGProxyCall*, gpointer data);

#define P_IN printf("%s--->%s ", green, white)
#define SIG_IN printf("%s--->%s %s\n", bold, white, __func__)
#define SIG_IN1 printf("%s--->%s ", bold, white)
#define SIG_OUT printf("%s(): %s--->%s\n", __func__, yellow, white)


void
dbus_object_new(AyyiAction* action, char* name, gboolean from_source, uint32_t src_idx, guint32 parent_idx, struct _song_pos* stime, guint64 len, guint32 inset)
{
	unsigned char type = action->obj_type;
	SIG_OUT;
	guint64 stime64; //TODO send the struct directly.
	if (stime) memcpy(&stime64, stime, 8);

	gpointer user_data = action;
	dbus_g_proxy_begin_call(proxy, "CreateObject", create_object_notify, user_data, NULL, 
							G_TYPE_INT, type, G_TYPE_STRING, name, G_TYPE_BOOLEAN, from_source, G_TYPE_UINT, src_idx, 
							G_TYPE_UINT, parent_idx, G_TYPE_UINT64, stime64, G_TYPE_UINT64, len, G_TYPE_UINT, inset, G_TYPE_INVALID);
}


static void
create_object_notify(DBusGProxy *proxy, DBusGProxyCall *call, gpointer data)
{
	P_IN;
	PF;
	AyyiAction* ayyi_action = (AyyiAction*)data;
	if(!ayyi_action) gwarn ("no action!");

	guint idx;
	GError *error = NULL;
	if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_UINT, &idx, G_TYPE_INVALID)){
		gwarn ("%s", error->message);
#ifdef __log_h__
		log_printf(LOG_FAIL, "%s", error->message);
#endif
		g_error_free(error);
		if(ayyi_action) action_free(ayyi_action);
		return;
	}

	if(ayyi_action){
		dbg (2, "request done. idx=%u", idx);
		ayyi_action->idx = idx;
		action_complete(ayyi_action);
	}
}


void
dbus_object_del(AyyiAction *action, int object_type, uint32_t object_idx)
{
	gpointer user_data = action;
	dbus_g_proxy_begin_call(proxy, "DeleteObject", delete_object_notify, user_data, NULL, 
							G_TYPE_INT, object_type, G_TYPE_UINT, object_idx, G_TYPE_INVALID);
}


static void
delete_object_notify(DBusGProxy *proxy, DBusGProxyCall *call, gpointer data)
{
	P_IN;
	PF;
	AyyiAction* ayyi_action = (AyyiAction*)data;
	//action* action = ayyi_action->app_data;
	if(!ayyi_action) gwarn ("no action!");

	GError *error = NULL;
	if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID)){
		gwarn ("%s", error->message);
#ifdef __log_h__
		log_printf(LOG_FAIL, "%s", error->message);//FIXME not curently in Ayyi client lib
#endif
		g_error_free(error);
		if(ayyi_action) action_free(ayyi_action);
		return;
	}

	action_complete(ayyi_action);
}


DBusGProxyCall*
dbus_get_prop_string(AyyiAction* action, int object_type, int property_type)
{
	SIG_OUT;
	gpointer user_data = action;
	return dbus_g_proxy_begin_call(proxy, "GetPropString", dbus_get_prop_notify, user_data, NULL, G_TYPE_INT, object_type, G_TYPE_INT, property_type, G_TYPE_INVALID); 
}


static void
dbus_get_prop_notify(DBusGProxy *proxy, DBusGProxyCall *call, gpointer data)
{
	P_IN;
	PF;
	AyyiAction* ayyi_action = (AyyiAction*)data;
	//action* action = ayyi_action ? ayyi_action->app_data : NULL;

	char* val = NULL;
	GError* error = NULL;
	if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &val, G_TYPE_INVALID)){
		gerr ("reply: %s", error->message);
		g_error_free(error);
		if(ayyi_action) action_free(ayyi_action);
		return;
	}

	dbg(0, "val=%p", val);
	ayyi_action->return_val_1 = val;

	if(ayyi_action) action_complete(ayyi_action);
}


DBusGProxyCall*
dbus_set_prop_int(AyyiAction* action, int object_type, int property_type, uint32_t object_idx, int val)
{
	PF;
	gpointer user_data = action;
	return dbus_g_proxy_begin_call(proxy, "SetPropInt", dbus_set_prop_notify, user_data, NULL, G_TYPE_INT, object_type, G_TYPE_INT, property_type, G_TYPE_UINT, object_idx, G_TYPE_INT, val, G_TYPE_INVALID); 
}


DBusGProxyCall*
dbus_set_prop_int64(AyyiAction* action, int object_type, int property_type, uint32_t object_idx, int64_t val)
{
	SIG_OUT;
	gpointer user_data = action;
	return dbus_g_proxy_begin_call(proxy, "SetPropInt64", dbus_set_prop_notify, user_data, NULL, G_TYPE_INT, object_type, G_TYPE_INT, property_type, G_TYPE_UINT, object_idx, G_TYPE_INT64, val, G_TYPE_INVALID); 
}


DBusGProxyCall*
dbus_set_prop_float (AyyiAction* action, int object_type, int property_type, uint32_t object_idx, double val)
{
	PF;
	gpointer user_data = action;
	return dbus_g_proxy_begin_call(proxy, "SetPropFloat", dbus_set_prop_notify, user_data, NULL, G_TYPE_INT, object_type, G_TYPE_INT, property_type, G_TYPE_UINT, object_idx, G_TYPE_DOUBLE, val, G_TYPE_INVALID); 
}


DBusGProxyCall*
dbus_set_prop_bool (AyyiAction* action, int object_type, int property_type, uint32_t object_idx, gboolean val)
{
	PF;
	gpointer user_data = action;
	return dbus_g_proxy_begin_call(proxy, "SetPropBool", dbus_set_prop_notify, user_data, NULL, G_TYPE_INT, object_type, G_TYPE_INT, property_type, G_TYPE_UINT, object_idx, G_TYPE_BOOLEAN, val, G_TYPE_INVALID); 
}


DBusGProxyCall*
dbus_set_prop_string(AyyiAction* action, int object_type, int property_type, uint32_t object_idx, const char* val)
{
	PF;
	gpointer user_data = action;
	return dbus_g_proxy_begin_call(proxy, "SetPropString", dbus_set_prop_notify, user_data, NULL, G_TYPE_INT, object_type, G_TYPE_INT, property_type, G_TYPE_UINT, object_idx, G_TYPE_STRING, val, G_TYPE_INVALID); 
}


DBusGProxyCall*
dbus_set_prop_3i_2f (AyyiAction* action, int object_type, int property_type, uint32_t object_idx, uint32_t idx2, uint32_t idx3, double val1, double val2)
{
	SIG_OUT;
	return dbus_g_proxy_begin_call(proxy, "SetProp3Index2Float", dbus_set_prop_notify, action, NULL, G_TYPE_INT, object_type, G_TYPE_INT, property_type, G_TYPE_UINT, object_idx, G_TYPE_UINT, idx2, G_TYPE_UINT, idx3, G_TYPE_DOUBLE, val1, G_TYPE_DOUBLE, val2, G_TYPE_INVALID); 
}


#define DBUS_GVAL_ARRAY (dbus_g_type_get_struct ("GValueArray", DBUS_STRUCT_BYTE_BYTE_UINT_UINT, G_TYPE_INVALID))

#if 0
#include <libgnomecanvas/libgnomecanvas.h>
#include "part_item.h" //FIXME we'll have to create the pointer array in midi_notes_move_async(). yes its available now
#include "canvas_op.h" //FIXME
DBusGProxyCall*
dbus_set_notelistX(AyyiAction* action, uint32_t part_idx, const GList* notelist)
{
	//the notes are sent as a ptr_array of type GValue.

	if(!notelist) return NULL;

	GPtrArray* array = g_ptr_array_new();

	GList* l = (GList*)notelist;
	int i = 0;
	for(;l;l=l->next){
		TransitionalNote* note = l->data;

		GValue value = { 0, };
		g_value_init (&value, DBUS_STRUCT_UINT_BYTE_BYTE_UINT_UINT);
		g_value_take_boxed (&value, dbus_g_type_specialized_construct (DBUS_STRUCT_UINT_BYTE_BYTE_UINT_UINT));

		dbus_g_type_struct_set (&value, 0, note->orig->idx, 1, note->copy.note, 2, note->copy.velocity, 3, note->copy.start, 4, note->copy.length, G_MAXUINT);
		g_ptr_array_add (array, g_value_get_boxed(&value));
		dbg(2, "array_len=%i key=%i", array->len, note->orig->note);

		i++;
	}

	SIG_OUT;
	return dbus_g_proxy_begin_call(proxy, "SetNotelist", dbus_set_prop_notify, action, NULL, G_TYPE_UINT, part_idx, dbus_g_type_get_collection ("GPtrArray", DBUS_STRUCT_UINT_BYTE_BYTE_UINT_UINT), array, G_TYPE_INVALID);
}
#endif


DBusGProxyCall*
dbus_set_notelist(AyyiAction* action, uint32_t part_idx, const GPtrArray* notes)
{
	//the notes are sent as a ptr_array of type GValue.

	if(!notes) return NULL;

	SIG_OUT;
	return dbus_g_proxy_begin_call(proxy, "SetNotelist", dbus_set_prop_notify, action, NULL, G_TYPE_UINT, part_idx, dbus_g_type_get_collection ("GPtrArray", DBUS_STRUCT_UINT_BYTE_BYTE_UINT_UINT), notes, G_TYPE_INVALID);
}


static void
dbus_set_prop_notify(DBusGProxy *proxy, DBusGProxyCall *call, gpointer data)
{
	P_IN;
	AyyiAction* ayyi_action = (AyyiAction*)data;
	if(debug) if(!ayyi_action) dbg (0, "no action!"); //some msgs, such as mixer events, dont bother with actions.

	action* action = ayyi_action ? ayyi_action->app_data : NULL;

#ifdef __log_h__
	if(IS_PTR(action)) dbg (0, "call=%p trid=%i %s%s%s", call, action->trid, bold, action->label, white);
#endif

	gboolean ok;
	GError *error = NULL;
	if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_BOOLEAN, &ok, G_TYPE_INVALID)){
#ifdef __log_h__
		log_printf(LOG_FAIL, "%s: %s", action?action->label:"", error->message);
#endif
		g_error_free(error);
		if(action){
			if(ayyi_action->err) ayyi_action->err = 1;
			//note: callback fns should now check for action->err.
			action_complete(ayyi_action);
		}
		return;
	}

	if(ayyi_action) action_complete(ayyi_action);
}


void
dbus_undo(Service* service, int n)
{
	SIG_OUT;
	gboolean ok;
	GError *error = NULL;
	if(!dbus_g_proxy_call(service->proxy, "Undo", &error, G_TYPE_UINT, n, G_TYPE_INVALID, G_TYPE_BOOLEAN, &ok, G_TYPE_INVALID)){
		gwarn("%s", error->message);
		g_error_free(error);
	}
}


