#define __ayyi_dbus_c__
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <ayyi/dbus_marshallers.h>
#include <ayyi/ayyi_typedefs.h>
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_seg0.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/ayyi_time.h>
#include <ayyi/ayyi_client.h>
#include <ayyi/ayyi_server.h>
#include <ayyi/engine_proxy.h>
#include <ayyi/ayyi_dbus.h>

#define ARDOURD_DBUS_SERVICE   "org.ayyi.ardourd.ApplicationService"
#define ARDOURD_DBUS_PATH      "/org/ayyi/ardourd/Ardourd"      //the "node" tag in the xml file, and string in the dbus_g_connection_register_g_object() call.
#define ARDOURD_DBUS_INTERFACE "org.ayyi.ardourd.Application"   //the "interface" name given in the xml file

#define AYYI_DBUS_ERROR ayyi_dbus_error_quark()

extern DBusGProxy *proxy;

//extern char seg_strs[3][16];

static void     dbus_shm_notify          (DBusGProxy*, DBusGProxyCall*, gpointer data);
static char*    dbus_introspect          (DBusGConnection*);
static GQuark   ayyi_dbus_error_quark    ();

Service known_services[] = {{ARDOURD_DBUS_SERVICE, ARDOURD_DBUS_PATH, ARDOURD_DBUS_INTERFACE}};


static gboolean
dbus_service_test(Service* service, GError** error)
{
	//fn is used speculatively - failing is not neccesarily an error.

	if(!service->proxy) return FALSE;

	int id;
	if(!dbus_g_proxy_call(service->proxy, "GetShmSingle", error, G_TYPE_STRING, seg_strs[SEG_TYPE_SONG], G_TYPE_INVALID, G_TYPE_UINT, &id, G_TYPE_INVALID)){
		if(error){
			dbg (0, "GetShm: %s\n", (*error)->message);
			g_error_free(*error);
		}
		return FALSE;
	}
	return TRUE;
}


gboolean
dbus_server_connect(Service* server, GError** error)
{
	// first we try DBUS_BUS_SESSION, then DBUS_BUS_SYSTEM.
	// app can work with either buss under the right conditions.

	#define BUS_COUNT 2
	struct _busses {
		int bus;
		char name[64];
	} busses[BUS_COUNT] = {
		{DBUS_BUS_SESSION, "session"},
		{DBUS_BUS_SYSTEM, "system"}
	};
	static DBusGConnection *connection[BUS_COUNT] = {NULL, NULL}; //should never be unreffed - is shared amongst all processes.  

	DBusGConnection*
	get_connection(int i, GError** error)
	{
		if(!connection[i]){
			connection[i] = dbus_g_bus_get(busses[i].bus, error);
		}
		return connection[i];
	}

	gboolean acquired = FALSE;

	int i = 0;
	if(!connection[0]){
		for(i=0;i<BUS_COUNT;i++){
			GError *error = NULL;
			dbg (0, "trying bus: %s...", busses[i].name);
			//if((connection = dbus_g_bus_get(busses[i].bus, &error))){
			if((connection[i] = get_connection(i, &error))){
				if((server->proxy = dbus_g_proxy_new_for_name (connection[i], server->service, server->path, server->interface))){
					//we always get a proxy, even if the requested service isnt running.
					dbg (2, "got proxy ok");
					if(dbus_service_test(server, NULL)){
						acquired = TRUE;
						dbg (0, "using '%s' bus.", busses[i].name);
						break;
					}
				}
				else dbg (0, "service not running on this bus? (%s)", busses[i].name); //TODO busname may be wrong if connection made previously.
			}
			else{
				switch(error->code){
					case DBUS_GERROR_FILE_NOT_FOUND:
						printf("%s dbus not running?\n", busses[i].name);
						break;
					default:
						printf("failed to open connection to %s%s%s bus", bold, busses[i].name, white);
						if(error->code != 0) printf(": %i %s\n", error->code, error->message);
						printf("\n");
						break;
				}
				g_error_free (error);
				continue;
			}

		}
	} else { //this just means that the fn is being run for the second time. FIXME we should be able to move this back into the above block
		if((server->proxy = dbus_g_proxy_new_for_name (connection[i], server->service, server->path, server->interface))){
			//we always get a proxy, even if the requested service isnt running.
			if(dbus_service_test(server, NULL)){
				acquired = TRUE;
			}
			//else gwarn ("service test failed.");
		}
		else dbg (0, "service not running on this bus? (%s)", busses[i].name); //TODO busname may be wrong if connection made previously.
	}

	if(!connection[0] && !connection[1]){ if(!*error) g_set_error (error, AYYI_DBUS_ERROR, 93642, "no connections made to any dbus."); return FALSE; }

	if(!acquired){
		g_set_error (error, AYYI_DBUS_ERROR, 98742, "service not found: %s. session_bus=%s system_bus=%s", server->service, connection[0]?"y":"n", connection[1]?"y":"n");
		return FALSE;
	}

	if(!proxy) proxy = server->proxy; //temp

	dbus_g_object_register_marshaller (_ad_dbus_marshal_VOID__STRING_INT, G_TYPE_NONE, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INVALID);
	dbus_g_object_register_marshaller (_ad_dbus_marshal_VOID__INT_INT, G_TYPE_NONE, G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);
	dbus_g_object_register_marshaller (_ad_dbus_marshal_VOID__INT_INT_INT, G_TYPE_NONE, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);

	if(0) dbg(0, "%s", dbus_introspect(connection[0]));

	return TRUE;
}


gboolean
dbus_server_get_shm(Service* server)
{
	dbg (2, "...");

	if(!ayyi.segs){ gwarn("no shm segs have been configured."); return FALSE; }

	GList* list = ayyi.segs;
	for(;list;list=list->next){
#define DBUS_SYNC
#ifdef DBUS_SYNC
		shm_seg* seg = list->data;

		GError *error = NULL;
		//char **strs;
		//char **strs_p;
		//guint id;
		//if(!dbus_g_proxy_call(proxy, "GetShm", &error, G_TYPE_INVALID, G_TYPE_STRV, &strs, G_TYPE_INVALID)){
		if(!dbus_g_proxy_call(server->proxy, "GetShmSingle", &error, G_TYPE_STRING, seg_strs[seg->type], G_TYPE_INVALID, G_TYPE_UINT, &seg->id, G_TYPE_INVALID)){

			//just to demonstrate remote exceptions versus regular GError
			if(error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION){
				gerr ("caught remote method exception %s: %s", dbus_g_error_get_name(error), error->message);
			}else{
				gerr ("GetShm: %s", error->message);
			}
			g_error_free(error);

			return FALSE;
		}
		dbg (1, "fd=%i\n", seg->id);
		//for (strs_p = strs; *strs_p; strs_p++) printf ("got string: \"%s\"", *strs_p);
		//g_strfreev (strs);

		dbus_shm_notify(server->proxy, NULL, seg);

#else
		dbus_g_proxy_begin_call(server->proxy, "GetShmSingle", dbus_shm_notify, NULL, NULL, G_TYPE_STRING, segs[i], G_TYPE_INVALID);
#endif
	}
	return TRUE;
}


gboolean
dbus_server_get_shm__server(AyyiServer* ayyi)
{
	//FIXME refactor to use client version instead: dbus_server_get_shm()

	//attempt to import all shm segments listed in ayyi->foreign_shm_segs
	dbg (1, "...");

	//char segs[3][16]= {"", "song", "mixer"};
	//int type;
	//for(type=1;type<=2;type++){

	//ASSERT_POINTER_FALSE(shm.segs, "app.shm_segs");
	GList* list = ayyi->foreign_shm_segs;
	for(;list;list=list->next){
#define DBUS_SYNC
#ifdef DBUS_SYNC
		AyyiShmSeg* seg = list->data;

		GError *error = NULL;
		//char **strs;
		//char **strs_p;
		//guint id;
		//if(!dbus_g_proxy_call(proxy, "GetShm", &error, G_TYPE_INVALID, G_TYPE_STRV, &strs, G_TYPE_INVALID)){
		if(!dbus_g_proxy_call(proxy, "GetShmSingle", &error, G_TYPE_STRING, seg_strs[seg->type], G_TYPE_INVALID, G_TYPE_UINT, &seg->id, G_TYPE_INVALID)){

			//just to demonstrate remote exceptions versus regular GError
			if(error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION){
				gerr ("caught remote method exception %s: %s", dbus_g_error_get_name(error), error->message);
			}else{
				gerr ("GetShm: %s", error->message);
			}
			g_error_free(error);

			return FALSE;
		}
		dbg (1, "fd=%i\n", seg->id);
		//for (strs_p = strs; *strs_p; strs_p++) printf ("got string: \"%s\"", *strs_p);
		//g_strfreev (strs);

		dbus_shm_notify(proxy, NULL, seg);

#else
		dbus_g_proxy_begin_call(proxy, "GetShmSingle", dbus_shm_notify, NULL, NULL, G_TYPE_STRING, segs[i], G_TYPE_INVALID);
#endif
	}
	return TRUE;
}


static void
dbus_shm_notify(DBusGProxy *proxy, DBusGProxyCall *call, gpointer _seg)
{
	//our request for shm address has returned.

	shm_seg* seg;
	guint id;
	if(call){
		GError *error = NULL;
		if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID, G_TYPE_UINT, &id, G_TYPE_INVALID)){
			gerr ("failed to get shm address: %s", error->message);
			g_error_free(error);
			return;
		}
	}
	//else id = GPOINTER_TO_INT(_id);
	else{
		seg = (struct _shm_seg*)_seg;
		//seg->attached = TRUE; //err....
	}

	ayyi_shm_import();

	if(known_services[0].on_shm) known_services[0].on_shm();
	else gwarn("no on_shm callback set.");
}


static char*
dbus_introspect(DBusGConnection* connection)
{
	//fetch the xml file describing the dbus service.

	g_return_val_if_fail (connection != NULL, NULL);

	DBusGProxy* remote_object_introspectable = dbus_g_proxy_new_for_name (connection, ARDOURD_DBUS_SERVICE, ARDOURD_DBUS_PATH, "org.freedesktop.DBus.Introspectable");

	GError *err = NULL;
	char *introspect_data = NULL;
	if (!dbus_g_proxy_call (remote_object_introspectable, "Introspect", &err, G_TYPE_INVALID, G_TYPE_STRING, &introspect_data, G_TYPE_INVALID)) {
		gwarn ("failed to complete Introspect: %s", err->message);
		g_error_free (err);
	}

	g_object_unref (G_OBJECT (remote_object_introspectable));

	return introspect_data;
}


static GQuark
ayyi_dbus_error_quark()
{
	static GQuark quark = 0;
	if (!quark) quark = g_quark_from_static_string ("ayyi_dbus_error");
	return quark;
}

