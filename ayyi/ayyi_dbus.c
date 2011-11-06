/*
  This file is part of the Ayyi Project. http://ayyi.org
  copyright (C) 2004-2011 Tim Orford <tim@orford.org>

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
#define __ayyi_dbus_c__
#include "config.h"
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <ayyi/dbus_marshallers.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_time.h>
#include <ayyi/ayyi_server.h>
#include <ayyi/engine_proxy.h>
#include <ayyi/ayyi_dbus.h>

#define ARDOURD_DBUS_SERVICE   "org.ayyi.ardourd.ApplicationService"
#define ARDOURD_DBUS_PATH      "/org/ayyi/ardourd/Ardourd"      //the "node" tag in the xml file, and string in the dbus_g_connection_register_g_object() call.
#define ARDOURD_DBUS_INTERFACE "org.ayyi.ardourd.Application"   //the "interface" name given in the xml file

#define AYYI_DBUS_ERROR ayyi_dbus_error_quark()

extern DBusGProxy *proxy;

static void     dbus_shm_notify            (DBusGProxy*, DBusGProxyCall*, gpointer data);
static char*    dbus_introspect            (DBusGConnection*);
static GQuark   ayyi_dbus_error_quark      ();
static void     ayyi_dbus__rx_deleted      (DBusGProxy*, int object_type, int object_idx, gpointer);
//static void     ayyi_dbus__rx_changed      (DBusGProxy*, int object_type, int object_idx, gpointer);
static void     ayyi_dbus__rx_property     (DBusGProxy*, int object_type, int object_idx, int property, gpointer);
static void     ayyi_handle                (AyyiIdent obj, AyyiPropType, const GList* _handlers);

Service known_services[] = {{ARDOURD_DBUS_SERVICE, ARDOURD_DBUS_PATH, ARDOURD_DBUS_INTERFACE, NULL, NULL, NULL, NULL, NULL}};


static gboolean
dbus_service_test(Service* service, GError** error)
{
	//fn is used speculatively - failing is not neccesarily an error.

	if(!service->priv->proxy) return FALSE;

	int id;
	if(!dbus_g_proxy_call(service->priv->proxy, "GetShmSingle", error, G_TYPE_STRING, seg_strs[SEG_TYPE_SONG], G_TYPE_INVALID, G_TYPE_UINT, &id, G_TYPE_INVALID)){
		if(error){
			dbg (0, "GetShm: %s\n", (*error)->message);
			g_error_free(*error);
		}
		return FALSE;
	}
	return TRUE;
}


gboolean
ayyi_client__dbus_connect(Service* server, GError** error)
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
	static DBusGConnection* connection[BUS_COUNT] = {NULL, NULL}; //never unreffed - is shared amongst all processes.  

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
			GError* error = NULL;
			dbg (2, "trying bus: %s...", busses[i].name);
			//if((connection = dbus_g_bus_get(busses[i].bus, &error))){
			if((connection[i] = get_connection(i, &error))){
				if((server->priv->proxy = dbus_g_proxy_new_for_name (connection[i], server->service, server->path, server->interface))){
					//we always get a proxy, even if the requested service isnt running.
					dbg (2, "got proxy ok");
					if(dbus_service_test(server, NULL)){
						acquired = TRUE;
						dbg (1, "using '%s' bus.", busses[i].name);
						break;
					}
					else dbg(2, "service test failed.");
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
		g_return_val_if_fail(server->priv, false);
		if((server->priv->proxy = dbus_g_proxy_new_for_name (connection[i], server->service, server->path, server->interface))){
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

	if(!proxy) proxy = server->priv->proxy; //temp

	dbus_g_object_register_marshaller (_ad_dbus_marshal_VOID__STRING_INT, G_TYPE_NONE, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INVALID);
	dbus_g_object_register_marshaller (_ad_dbus_marshal_VOID__INT_INT, G_TYPE_NONE, G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);
	dbus_g_object_register_marshaller (_ad_dbus_marshal_VOID__INT_INT_INT, G_TYPE_NONE, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);

	if(0) dbg(0, "%s", dbus_introspect(connection[0]));

	return TRUE;
}


gboolean
ayyi_client__dbus_get_shm(Service* server)
{
	dbg (2, "...");

	if(!ayyi.service->segs){ gwarn("no shm segs have been configured."); return FALSE; }

	GList* l = ayyi.service->segs;
	for(;l;l=l->next){
#define DBUS_SYNC
#ifdef DBUS_SYNC
		shm_seg* seg = l->data;

		GError *error = NULL;
		//char **strs;
		//char **strs_p;
		//guint id;
		//if(!dbus_g_proxy_call(proxy, "GetShm", &error, G_TYPE_INVALID, G_TYPE_STRV, &strs, G_TYPE_INVALID)){
		if(!dbus_g_proxy_call(server->priv->proxy, "GetShmSingle", &error, G_TYPE_STRING, seg_strs[seg->type], G_TYPE_INVALID, G_TYPE_UINT, &seg->id, G_TYPE_INVALID)){

			//just to demonstrate remote exceptions versus regular GError
			if(error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION){
				gerr ("caught remote method exception %s: %s", dbus_g_error_get_name(error), error->message);
			}else{
				gerr ("GetShm: %s", error->message);
			}
			g_error_free(error);

			return FALSE;
		}
		dbg (2, "fd=%i", seg->id);
		//for (strs_p = strs; *strs_p; strs_p++) printf ("got string: \"%s\"", *strs_p);
		//g_strfreev (strs);

		dbus_shm_notify(server->priv->proxy, NULL, seg);

#else
		dbus_g_proxy_begin_call(server->priv->proxy, "GetShmSingle", dbus_shm_notify, NULL, NULL, G_TYPE_STRING, segs[i], G_TYPE_INVALID);
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

	GList* l = ayyi->foreign_shm_segs;
	for(;l;l=l->next){
#define DBUS_SYNC
#ifdef DBUS_SYNC
		AyyiShmSeg* seg = l->data;

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
dbus_shm_notify(DBusGProxy* proxy, DBusGProxyCall* call, gpointer _seg)
{
	//our request for shm address has returned.

	typedef void (*ShmCallback)(shm_seg*);
	ShmCallback user_callback = known_services[0].on_shm;

	void do_callback(ShmCallback callback, shm_seg* shm_seg)
	{
		if (callback) callback(shm_seg);
		else gwarn("no on_shm callback set.");
	}

	if (call) {
		guint id;
		GError* error = NULL;
		if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID, G_TYPE_UINT, &id, G_TYPE_INVALID)){
			gerr ("failed to get shm address: %s", error->message);
			g_error_free(error);
			return;
		}
	} else {
		shm_seg* seg; seg = (struct _shm_seg*)_seg;
	}

	if (ayyi_shm_import()) {
		do_callback(user_callback, (shm_seg*)_seg);
	}
	else {
		if(ayyi.debug) gwarn("shm import failed. Calling callback with NULL arg");
		do_callback(user_callback, NULL);
	}
}


void
ayyi_dbus_ping(Service* server, void (*pong)(const char*))
{
	void ping_done(DBusGProxy* proxy, DBusGProxyCall* call, gpointer _user_data)
	{
		void (*pong)(const char*) = _user_data;

		gchar* message;
		GError* error = NULL;
		if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID, G_TYPE_STRING, &message, G_TYPE_INVALID)){
			return;
		}
		call(pong, message);
	}
	dbus_g_proxy_begin_call(proxy, "Ping", ping_done, pong, NULL, G_TYPE_INVALID);
}


static char*
dbus_introspect(DBusGConnection* connection)
{
	//fetch the xml file describing the dbus service.

	g_return_val_if_fail (connection != NULL, NULL);

	DBusGProxy* remote_object_introspectable = dbus_g_proxy_new_for_name (connection, ARDOURD_DBUS_SERVICE, ARDOURD_DBUS_PATH, "org.freedesktop.DBus.Introspectable");

	GError* err = NULL;
	char* introspect_data = NULL;
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


void
ayyi_dbus__register_signals(int signals)
{
	PF;
	void
	ayyi_dbus_rx_obj_new(DBusGProxy* proxy, int object_type, int object_idx, gpointer data)
	{
		SIG_IN1;
if(object_type == AYYI_OBJECT_SONG) dbg(0, "SONG load");
		ayyi_handle((AyyiIdent){object_type, object_idx}, AYYI_NO_PROP, ayyi_client_get_responders(object_type, AYYI_NEW, -1));
	}

	void ayyi_dbus_rx_transport(DBusGProxy *proxy, int type, gpointer user_data)
	{
		SIG_IN1;
		ayyi_handle(AYYI_NULL_IDENT, AYYI_NO_PROP, ayyi_client_get_responders(AYYI_OBJECT_TRANSPORT, AYYI_SET, -1));
	}

	void am_dbus_rx_locators(DBusGProxy *proxy, int type, gpointer user_data)
	{
		SIG_IN1;
		ayyi_handle(AYYI_NULL_IDENT, AYYI_NO_PROP, ayyi_client_get_responders(AYYI_OBJECT_LOCATORS, AYYI_SET, -1));
	}

	static int registered = 0;

	if(signals & NEW){
		dbus_g_proxy_add_signal     (proxy, "ObjnewSignal", G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);
		dbus_g_proxy_connect_signal (proxy, "ObjnewSignal", G_CALLBACK(ayyi_dbus_rx_obj_new), NULL, NULL);
		registered |= NEW;
	}else{
		if(registered & NEW){
			gwarn("already registered: NEW");
			/*
			dbg(0, "NEW: TODO need to unregister signal");
			registered &= ~NEW;
			*/
		}
	}

	if(signals & DELETED){
		dbus_g_proxy_add_signal     (proxy, "DeletedSignal", G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);
		dbus_g_proxy_connect_signal (proxy, "DeletedSignal", G_CALLBACK (ayyi_dbus__rx_deleted), NULL, NULL);
		registered |= DELETED;
	}else{
		if(registered & DELETED){
			dbg(0, "DELETED: TODO need to unregister signal");
			registered &= ~DELETED;
		}
	}

	/* this signal no longer exists
	if(signals & CHANGED){
		dbus_g_proxy_add_signal     (proxy, "ObjchangedSignal", G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);
		dbus_g_proxy_connect_signal (proxy, "ObjchangedSignal", G_CALLBACK (ayyi_dbus__rx_changed), NULL, NULL);
		registered |= CHANGED;
	}else{
		if(registered & CHANGED){
			dbg(0, "CHANGED: TODO need to unregister signal");
			registered &= ~CHANGED;
		}
	}
	*/

	if(signals & PROPERTY){
		dbus_g_proxy_add_signal     (proxy, "PropertySignal", G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);
		dbus_g_proxy_connect_signal (proxy, "PropertySignal", G_CALLBACK (ayyi_dbus__rx_property), NULL, NULL);
		registered |= PROPERTY;
	}else{
		if(registered & PROPERTY){
			dbg(0, "PROPERTY: TODO need to unregister signal");
			registered &= ~PROPERTY;
		}
	}

	if(signals & TRANSPORT){
		dbus_g_proxy_add_signal     (proxy, "TransportSignal", G_TYPE_STRING, G_TYPE_INVALID);
		dbus_g_proxy_connect_signal (proxy, "TransportSignal", G_CALLBACK (ayyi_dbus_rx_transport), NULL, NULL);
		registered |= TRANSPORT;
	}else{
		if(registered & TRANSPORT){
			dbg(0, "PROPERTY: TODO need to unregister signal");
			registered &= ~TRANSPORT;
		}
	}

	if(signals & LOCATORS){
		dbus_g_proxy_add_signal     (proxy, "LocatorsSignal", G_TYPE_STRING, G_TYPE_INVALID);
		dbus_g_proxy_connect_signal (proxy, "LocatorsSignal", G_CALLBACK (am_dbus_rx_locators), NULL, NULL);
		registered |= LOCATORS;
	}else{
		if(registered & LOCATORS){
			dbg(0, "LOCATOR: TODO need to unregister signal");
			registered &= ~LOCATORS;
		}
	}
}


static void 
ayyi_handle(AyyiIdent obj, AyyiPropType prop, const GList* _handlers)
{
	GList* handlers = g_list_copy((GList*)_handlers); //the list can be modified while iterating, so copy it first
	dbg(2, "%s n_handlers=%i", ayyi_print_object_type(obj.type), g_list_length(handlers));
	if(handlers){
		GList* h = handlers;
		for(;h;h=h->next){
			Responder* responder = h->data;
			dbg(2, "handler=%p", responder);
			dbg(2, "user_data=%p", responder->user_data);
			if(prop == AYYI_NO_PROP)
				(responder->callback)(obj, NULL, responder->user_data);
			else
				((AyyiPropHandler)responder->callback)(obj, NULL, prop, responder->user_data);
		}
		g_list_free(handlers);
	}
}


static void 
handle_all(AyyiIdent obj, AyyiPropType prop, AyyiOp op_type)
{
	//instance specific handlers:
	ayyi_handle(obj, prop, ayyi_client_get_responders(obj.type, op_type, obj.idx));

	//object-type handlers:
	ayyi_handle(obj, prop, ayyi_client_get_responders(obj.type, op_type, -1));

	//generic fallback handlers:
	ayyi_handle(obj, prop, ayyi_client_get_responders(-1, op_type, -1));
}


static void
ayyi_dbus__rx_deleted(DBusGProxy* proxy, int object_type, int object_idx, gpointer user_data)
{
	SIG_IN1;
	handle_all((AyyiIdent){object_type, object_idx}, AYYI_NO_PROP, AYYI_DEL);
}


#if 0
static void
ayyi_dbus__rx_changed(DBusGProxy* proxy, int object_type, int object_idx, gpointer user_data)
{
	if(ayyi.debug > 1) printf("%s--->%s changed: %s.%i\n", bold, white, ayyi_print_object_type(object_type), object_idx);
	AyyiIdent obj = {object_type, object_idx};

	handle_all(obj, AYYI_NO_PROP, AYYI_SET);
}
#endif


static void
ayyi_dbus__rx_property(DBusGProxy* proxy, int object_type, int object_idx, int property_type, gpointer user_data)
{
	if(ayyi.debug > 1) printf("%s--->%s changed: %s.%i %s\n", bold, white, ayyi_print_object_type(object_type), object_idx, ayyi_print_prop_type(property_type));
	AyyiIdent obj = {object_type, object_idx};

	handle_all(obj, property_type, AYYI_SET);
}


