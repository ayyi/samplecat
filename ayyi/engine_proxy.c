#define __engine_proxy_c__
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <ayyi/dbus_marshallers.h>
#include <ayyi/dbus_marshallers.c>

typedef void action;
#include <ayyi/ayyi_typedefs.h>
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/ayyi_msg.h>
#include <ayyi/ayyi_action.h>
#include <ayyi/ayyi_client.h>
#include <ayyi/ayyi_dbus.h>
#include <ayyi/ayyi_log.h>

extern int debug;

DBusGProxy *proxy = NULL;

static void     ayyi_create_notify       (DBusGProxy*, DBusGProxyCall*, gpointer data);
static void     ayyi_delete_notify       (DBusGProxy*, DBusGProxyCall*, gpointer data);
static void     dbus_set_prop_notify     (DBusGProxy*, DBusGProxyCall*, gpointer data);
static void     dbus_get_prop_notify     (DBusGProxy*, DBusGProxyCall*, gpointer data);
static void     report_dbus_error        (const char* func, GError*);

#define P_IN printf("%s--->%s ", green, white)
//#define SIG_IN1 printf("%s--->%s ", bold, white)


GQuark
ayyi_msg_error_quark()
{
	static GQuark quark = 0;
	if (!quark) quark = g_quark_from_static_string ("ayyi_comms_error");
	return quark;
}


void
ayyi_dbus_object_new(AyyiAction* action, const char* name, gboolean from_source, AyyiIdx src_idx, guint32 parent_idx, struct _song_pos* stime, guint64 len, guint32 inset)
{
	unsigned char type = action->obj_type;
	SIG_OUT;
	guint64 stime64; //TODO send the struct directly.
	if (stime) memcpy(&stime64, stime, 8);

	gpointer user_data = action;
	dbus_g_proxy_begin_call(proxy, "CreateObject", ayyi_create_notify, user_data, NULL, 
							G_TYPE_INT, type, G_TYPE_STRING, name, G_TYPE_BOOLEAN, from_source, G_TYPE_UINT, src_idx, 
							G_TYPE_UINT, parent_idx, G_TYPE_UINT64, stime64, G_TYPE_UINT64, len, G_TYPE_UINT, inset, G_TYPE_INVALID);
}


void
ayyi_dbus_part_new(AyyiAction* action, const char* name, gboolean from_source, AyyiIdx src_idx, guint32 parent_idx, struct _song_pos* stime, guint64 len, guint32 inset)
{
	unsigned char type = action->obj_type;
	SIG_OUT;
	guint64 stime64; //TODO send the struct directly.
	if (stime) memcpy(&stime64, stime, 8);

	gpointer user_data = action;
	dbus_g_proxy_begin_call(proxy, "CreatePart", ayyi_create_notify, user_data, NULL, 
							G_TYPE_INT, type, G_TYPE_STRING, name, G_TYPE_BOOLEAN, from_source, G_TYPE_UINT, src_idx, 
							G_TYPE_UINT, parent_idx, G_TYPE_UINT64, stime64, G_TYPE_UINT64, len, G_TYPE_UINT, inset, G_TYPE_INVALID);
}


static GError*
ayyi_communication_error()
{
	return g_error_new(ayyi_msg_error_quark(), 3539, "communication error");
}


static void
ayyi_dbus_process_error(const char* func, GError** _error)
{
	GError* error = *_error;

	if(error->domain != DBUS_GERROR){
		gwarn("unexpected error domain !!! %s", g_quark_to_string(error->domain));
	}
	if(error->code == DBUS_GERROR_REMOTE_EXCEPTION){
		//this is a remote application error rather than a dbus error.
		if(ayyi.debug > 1) gwarn("dbus remote exception error: name=%s", dbus_g_error_get_name(error));
	}
	if(error->code < DBUS_GERROR_REMOTE_EXCEPTION){
		report_dbus_error(func, error);
		g_error_free(error);

		//set a generic communication error. callers should not normally report this.
		*_error = ayyi_communication_error();
	}
}


static void
ayyi_create_notify(DBusGProxy *proxy, DBusGProxyCall *call, gpointer data)
{
	P_IN;
	PF2;
	AyyiAction* action = (AyyiAction*)data;
	if(!action) gwarn ("no action!");

	guint idx;
	GError* error = NULL;
	if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_UINT, &idx, G_TYPE_INVALID)){
		if(error){
			ayyi_dbus_process_error(__func__, &error);

			if(action) action->ret.error = error;
		}
	}else{
		if(action) action->ret.idx = idx;
	}

	if(action){
		dbg (2, "request done. idx=%u", idx);
		ayyi_action_complete(action);
	}
}


void
ayyi_dbus_object_del(AyyiAction *action, int object_type, uint32_t object_idx)
{
	gpointer user_data = action;
	dbus_g_proxy_begin_call(proxy, "DeleteObject", ayyi_delete_notify, user_data, NULL, 
							G_TYPE_INT, object_type, G_TYPE_UINT, object_idx, G_TYPE_INVALID);
}


static void
ayyi_delete_notify(DBusGProxy *proxy, DBusGProxyCall *call, gpointer data)
{
	P_IN;
	PF2;
	AyyiAction *action = (AyyiAction*)data;
	if(!action) gwarn ("no action!");

	action->ret.idx = action->obj_idx.idx1;

	GError *error = NULL;
	if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID)){
		if(error){
//dbg(0, "%s", error->message);

			/*
			//------> experimential <---------
			if(error->domain == DBUS_GERROR && error->domain != DBUS_GERROR_REMOTE_EXCEPTION){
				gwarn("dbus domain error");
				//these errors should perhaps not be passed on out of dbus scope
				report_dbus_error(__func__, error);
				g_error_free(error);

				//set a generic communication error. callers should not normally report this.
				error = g_error_new(ayyi_msg_error_quark(), 3539, "communication error");
			}
			*/
			ayyi_dbus_process_error(__func__, &error);

			if(action) action->ret.error = error;
		}
	}

	ayyi_action_complete(action);
}


DBusGProxyCall*
dbus_get_prop_string(AyyiAction *action, int object_type, int property_type)
{
	SIG_OUT;
	gpointer user_data = action;
	return dbus_g_proxy_begin_call(proxy, "GetPropString", dbus_get_prop_notify, user_data, NULL, G_TYPE_INT, object_type, G_TYPE_INT, property_type, G_TYPE_INVALID); 
}


static void
dbus_get_prop_notify(DBusGProxy *proxy, DBusGProxyCall *call, gpointer _action)
{
	P_IN;
	PF;
	AyyiAction* action = (AyyiAction*)_action;

	char* val = NULL;
	GError* error = NULL;
	if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &val, G_TYPE_INVALID)){

		//where possible, the AyyiAction handles the error
		if(action){
			action->ret.error = error;
		}else{
			report_dbus_error(__func__, error);
			g_error_free(error);
			error = NULL;
		}
	}

	if(action){
		action->ret.ptr_1 = val;
		ayyi_action_complete(action);
	}
}


DBusGProxyCall*
dbus_set_prop_int(AyyiAction* action, int object_type, int property_type, uint32_t object_idx, int val)
{
	dbg(2, "...");
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
	PF2;
	return dbus_g_proxy_begin_call(proxy, "SetPropFloat", dbus_set_prop_notify, action, NULL, G_TYPE_INT, object_type, G_TYPE_INT, property_type, G_TYPE_UINT, object_idx, G_TYPE_DOUBLE, val, G_TYPE_INVALID); 
}


DBusGProxyCall*
dbus_set_prop_bool (AyyiAction* action, int object_type, int property_type, uint32_t object_idx, gboolean val)
{
	PF2;
	gpointer user_data = action;
	return dbus_g_proxy_begin_call(proxy, "SetPropBool", dbus_set_prop_notify, user_data, NULL, G_TYPE_INT, object_type, G_TYPE_INT, property_type, G_TYPE_UINT, object_idx, G_TYPE_BOOLEAN, val, G_TYPE_INVALID); 
}


DBusGProxyCall*
dbus_set_prop_string(AyyiAction* action, int object_type, int property_type, uint32_t object_idx, const char* val)
{
	PF2;
	action->ret.idx = object_idx;
	gpointer user_data = action;
	return dbus_g_proxy_begin_call(proxy, "SetPropString", dbus_set_prop_notify, user_data, NULL, G_TYPE_INT, object_type, G_TYPE_INT, property_type, G_TYPE_UINT, object_idx, G_TYPE_STRING, val, G_TYPE_INVALID); 
}


DBusGProxyCall*
dbus_set_prop_3i_2f (AyyiAction* action, int object_type, int property_type, uint32_t object_idx, uint32_t idx2, uint32_t idx3, double val1, double val2)
{
	SIG_OUT;
	return dbus_g_proxy_begin_call(proxy, "SetProp3Index2Float", dbus_set_prop_notify, action, NULL, G_TYPE_INT, object_type, G_TYPE_INT, property_type, G_TYPE_UINT, object_idx, G_TYPE_UINT, idx2, G_TYPE_UINT, idx3, G_TYPE_DOUBLE, val1, G_TYPE_DOUBLE, val2, G_TYPE_INVALID); 
}


//#define DBUS_GVAL_ARRAY (dbus_g_type_get_struct ("GValueArray", DBUS_STRUCT_BYTE_BYTE_UINT_UINT, G_TYPE_INVALID))

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

	AyyiAction* action = (AyyiAction*)data;

	if(action) dbg (1, "%s%s%s", bold, action->label, white);
	else       dbg (2, "no action!"); //some msgs, such as mixer events, dont need actions.

	gboolean ok;
	GError *error = NULL;
	if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_BOOLEAN, &ok, G_TYPE_INVALID)){
		if(action){
			if(error){
				ayyi_dbus_process_error(__func__, &error);

				action->ret.error = error;
			}
			ayyi_action_complete(action);
		}
		return;
	}

	if(action) ayyi_action_complete(action);
}


void
dbus_undo(Service* service, int n)
{
	SIG_OUT;
	gboolean ok;
	GError *error = NULL;
	if(!dbus_g_proxy_call(service->priv->proxy, "Undo", &error, G_TYPE_UINT, n, G_TYPE_INVALID, G_TYPE_BOOLEAN, &ok, G_TYPE_INVALID)){
		gwarn("%s", error->message);
		g_error_free(error);
	}
}


static void report_dbus_error(const char* func, GError* error)
{
	if(error->code == DBUS_GERROR_NO_REPLY){
		g_warning("%s: server timeout", func);
		log_print(LOG_FAIL, "server timeout");
	}else if(error->code == DBUS_GERROR_SERVICE_UNKNOWN){
		log_print(LOG_FAIL, "service not running");
	}else{
		g_warning ("%s: %i: %s", func, error->code, error->message);
		log_print(LOG_FAIL, "%s", error->message);
	}
}

