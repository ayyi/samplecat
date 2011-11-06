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
#define __ayyi_client_c__
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <glib.h>
#include <gmodule.h>

typedef void action;
#include <ayyi/ayyi_typedefs.h>
#include <ayyi/ayyi_types.h>
#include <ayyi/interface.h> //good to remove?
#include <ayyi/ayyi_utils.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_msg.h>
#include <ayyi/ayyi_dbus.h>
#include <ayyi/engine_proxy.h>
//#include <ayyi/ayyi_log.h>
#include <ayyi/ayyi_dbus.h>

#include <ayyi/ayyi_client.h>

extern struct _ayyi_client ayyi;
#define plugin_path "/usr/lib/ayyi/:/usr/local/lib/ayyi/"

//FIXME doesnt belong here
extern AyyiItem*       ayyi_song_container_next_item        (AyyiContainer*, void* prev);

typedef	AyyiPluginPtr (*infoFunc)();

static void          ayyi_client_ping                 (void (*pong)(const char* s));
static AyyiPluginPtr ayyi_plugin_load                 (const gchar* filename);
static void          ayyi_client_discover_launch_info ();

enum {
    PLUGIN_TYPE_1 = 1,
    PLUGIN_TYPE_MAX
};
#define PLUGIN_API_VERSION 1


AyyiClient*
ayyi_client_init()
{
	if(ayyi.priv){ gwarn("already initialised!"); return &ayyi; }

	ayyi_log_init();

	ayyi.got_shm = 0;     //before this is set, we must have _all_ required shm pointers.

	ayyi.priv           = g_new0(AyyiClientPrivate, 1);
	ayyi.priv->on_shm   = NULL;
	ayyi.priv->trans_id = 1;
	ayyi.priv->responders = g_hash_table_new(g_str_hash, g_str_equal);

	ayyi_shm_init ();

 	ayyi.service = &known_services[AYYI_SERVICE_AYYID1];
	ayyi.service->priv = g_new0(AyyiServicePrivate, 1);
	ayyi.service->ping = ayyi_client_ping;

	ayyi_client_discover_launch_info();

	return &ayyi;
}


gboolean
ayyi_client_connect(Service* server, GError** error)
{
	return ayyi_client__dbus_connect(server, error);
}


static void
ayyi_client_ping(void (*pong)(const char* s))
{
	ayyi_dbus_ping(ayyi.service, pong);
}


void
ayyi_client_load_plugins()
{
	AyyiPluginPtr plugin = NULL;
	GError* error = NULL;

	if(!g_module_supported()) g_error("Modules not supported! (%s)", g_module_error());

	int found = 0;

	gchar** paths = g_strsplit(plugin_path, ":", 0);
	char* path;
	int i = 0;
	while((path = paths[i++])){
		if(!g_file_test(path, G_FILE_TEST_EXISTS)) continue;

		dbg(2, "scanning for plugins (%s) ...", path);
		GDir* dir = g_dir_open(path, 0, &error);
		if (!error) {
			const gchar* filename = g_dir_read_name(dir);
			while (filename) {
				dbg(1, "testing %s...", filename);
				gchar* filepath = g_build_filename(path, filename, NULL);
				// filter files with correct library suffix
				if(!strncmp(G_MODULE_SUFFIX, filename + strlen(filename) - strlen(G_MODULE_SUFFIX), strlen(G_MODULE_SUFFIX))) {
					// If we find one, try to load plugin info and if this was successful try to invoke the specific plugin
					// type loader. If the second loading went well add the plugin to the plugin list.
					if (!(plugin = ayyi_plugin_load(filepath))) {
						dbg(1, "'%s' failed to load.", filename);
					} else {
						found++;
						ayyi.priv->plugins = g_slist_append(ayyi.priv->plugins, plugin);
					}
				} else {
					dbg(2, "-> no library suffix");
				}
				g_free(filepath);
				filename = g_dir_read_name(dir);
			}
			g_dir_close(dir);
		} else {
			gwarn("dir='%s' failed. %s", path, error->message);
			g_error_free(error);
			error = NULL;
		}
	}
	g_strfreev(paths);

	log_print(0, "ayyi plugins loaded: %i.", found);
}


AyyiPluginPtr
ayyi_client_get_plugin(const char* name)
{
	//lookup the plugin pointer for a loaded plugin.

	static int count = -1;
	count++;

	GSList* l = ayyi.priv->plugins;
	for(;l;l=l->next){
		AyyiPluginPtr plugin = l->data;
		if(!count) dbg(2, "  %s", plugin->name);
		if(!strcmp(name, plugin->name)) return plugin;
	}
	return NULL;
}


static AyyiPluginPtr
ayyi_plugin_load(const gchar* filepath)
{
	AyyiPluginPtr plugin = NULL;
	gboolean success = FALSE;

#if GLIB_CHECK_VERSION(2,3,3)
	GModule* handle = g_module_open(filepath, G_MODULE_BIND_LOCAL);
#else
	GModule* handle = g_module_open(filepath, 0);
#endif

	if(!handle) {
		gwarn("cannot open %s (%s)!", filepath, g_module_error());
		return NULL;
	}

	infoFunc plugin_get_info;
	if(g_module_symbol(handle, "plugin_get_info", (void*)&plugin_get_info)) {
		// load generic plugin info
		if(NULL != (plugin = (*plugin_get_info)())) {
			// check plugin version
			if(PLUGIN_API_VERSION != plugin->api_version){
				dbg(0, "API version mismatch: \"%s\" (%s, type=%d) has version %d should be %d", plugin->name, filepath, plugin->type, plugin->api_version, PLUGIN_API_VERSION);
			}

			// try to load specific plugin type symbols
			switch(plugin->type) {
				default:
					if(plugin->type >= PLUGIN_TYPE_MAX) {
						dbg(0, "Unknown or unsupported plugin type: %s (%s, type=%d)", plugin->name, filepath, plugin->type);
					} else {
						dbg(0, "name='%s' %s", plugin->name, plugin->service_name);

						Service pservice = {plugin->service_name, plugin->app_path, plugin->interface, NULL};
						pservice.priv = g_new0(AyyiServicePrivate, 1);
						GError* error = NULL;
						if((ayyi_client__dbus_connect(&pservice, &error))){
							dbg(0, "plugin dbus connection ok.");
							success = TRUE; // no special initialization

							//dbus_register_signals();
							//dbus_server_get_shm(&engine);  //.....err shouldnt we be using this?

							//get shm address:

							plugin->client_data = (struct _spec_shm*)NULL;

							shm_seg* seg = ayyi_shm_seg_new(0, AYYI_SEG_TYPE_PLUGIN);

							if(dbus_g_proxy_call(pservice.priv->proxy, "GetShmSingle", &error, G_TYPE_STRING, "", G_TYPE_INVALID, G_TYPE_UINT, &seg->id, G_TYPE_INVALID)){
								ayyi_shm_import();
								plugin->client_data = seg->address; //TODO does it need to be translated? no, dont think so.
								dbg(2, "client_data=%p offset=%i", plugin->client_data, (uintptr_t)plugin->client_data - (uintptr_t)plugin->client_data);
							} else {
								if(error){
									dbg (0, "GetShm: %s\n", error->message);
									g_error_free(error);
								}
								success = FALSE;
							}
						}
						else{
                    if(error){
                      switch(error->code){
                        case 98742:
                          log_print(0, "plugin service not available: %s", plugin->service_name);
                          break;
                        default:
                          warn_gerror("plugin dbus connection failed", &error);
                          break;
                      }
                    } else log_print(0, "plugin connect failed, but no error set: %s", plugin->service_name);
                  }
					}
					break;
			}
		}
	} else {
		gwarn("File '%s' is not a valid Ayyi plugin!", filepath);
	}
	
	if(!success) {
		g_module_close(handle);
		return NULL;
	}
		
	return plugin;
}


static void
ayyi_client_discover_launch_info()
{
	#define ayyi_application_path "/usr/bin/ayyi/clients/:/usr/local/bin/ayyi/clients/"
	gchar** paths = g_strsplit(ayyi_application_path, ":", 0);
	GError* error = NULL;
	char* path;
	int i = 0;
	while((path = paths[i++])){
		if(!g_file_test(path, G_FILE_TEST_EXISTS)) continue;

		dbg(2, "scanning for ayyi applications (%s) ...", path);
		GDir* dir = g_dir_open(path, 0, &error);
		if (!error) {
			gchar* filename;
			while((filename = (gchar*)g_dir_read_name(dir))){
				dbg(2, "testing %s...", filename);
				if(!g_strrstr(filename, ".desktop")) continue;

				gchar* filepath = g_build_filename(path, filename, NULL);

				GError* error = NULL;
				GKeyFile* key_file = g_key_file_new();
				if(g_key_file_load_from_file(key_file, filepath, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)){
					gchar* groupname = g_key_file_get_start_group(key_file);
					if(!strcmp(groupname, "Desktop Entry")){
						AyyiLaunchInfo* info = g_new0(AyyiLaunchInfo, 1);
						gchar* keyval;
						if((keyval = g_key_file_get_string(key_file, groupname, "Name", &error))){
							info->name = keyval;
							if((keyval = g_key_file_get_string(key_file, groupname, "Exec", &error))){
								info->exec = keyval;
							}
						}
						if(true){
							ayyi.launch_info = g_list_append(ayyi.launch_info, info);
						} else g_free(info);
					}
				}else{
					printf("unable to load desktop file: %s.\n", error->message);
					g_error_free(error);
					error = NULL;
				}

				g_free(filepath);
			}
			g_dir_close(dir);
		} else {
			gwarn("dir='%s' failed. %s", path, error->message);
			g_error_free(error);
			error = NULL;
		}
	}
}


void
ayyi_client_set_signal_reg(int signals)
{
#ifdef USE_DBUS
	ayyi_dbus__register_signals(signals);
#endif
}


void
ayyi_object_set_bool(AyyiObjType object_type, AyyiIdx obj_idx, int prop, gboolean val, AyyiAction* action)
{
#ifdef USE_DBUS
	dbus_set_prop_bool(action, object_type, prop, obj_idx, val);
#endif
}


gboolean
ayyi_object_is_deleted(AyyiRegionBase* object)
{
	dbg(3, "deleted=%i (%i)", (object->flags & deleted), object->flags);
	return (object->flags & deleted);
}


#define MAKE_RESPONDER_KEY(object_type, op_type, object_idx) g_strdup_printf("%i-%i-%i", object_type, op_type, object_idx);


void
ayyi_client_add_responder(AyyiIdent object, AyyiOpType op_type, AyyiHandler callback, gpointer user_data)
{
	char* key = MAKE_RESPONDER_KEY(object.type, op_type, object.idx);
	dbg(2, "key=%s", key);
	Responder* responder = g_new0(Responder, 1);
	responder->callback = callback;
	responder->user_data = user_data;

	GList* responders = g_list_append(g_hash_table_lookup(ayyi.priv->responders, key), responder);
	g_hash_table_insert(ayyi.priv->responders, key, responders);

#if 0
	{
		GList* handlers = (GList*)ayyi_client_get_responders(object.type, op_type, object.idx);
		dbg(0, "%s n_handlers=%i", ayyi_print_object_type(object.type), g_list_length(handlers));
	}
#endif
}


void
ayyi_client_add_onetime_responder(AyyiIdent id, AyyiOpType op_type, AyyiHandler user_handler, gpointer user_data)
{
	//deletes the handler after first response.
	//TODO does it need a timeout?

	struct _data
	{
		AyyiHandler handler;
		AyyiHandler user_handler;
		void*        user_data;
		AyyiOpType   op_type;
	};

	void onetime_responder_callback(AyyiIdent obj, GError** error, gpointer _data)
	{
		struct _data* data = (struct _data*)_data;

		call(data->user_handler, obj, error, data->user_data);

		if(!ayyi_client_remove_responder(obj.type, data->op_type, obj.idx, data->handler)){
			gwarn("responder not removed.");
		}

		g_free(data);
	}

	struct _data* data = g_new0(struct _data, 1);
	data->handler = onetime_responder_callback;
	data->user_handler = user_handler;
	data->user_data = user_data;
	data->op_type = op_type;

	ayyi_client_add_responder(id, op_type, onetime_responder_callback, data);
}


const GList*
ayyi_client_get_responders(AyyiObjType object_type, AyyiOpType op_type, AyyiIdx object_index)
{
	//the returned list shoulds NOT be freed.

#if 0
	{
		GHashTableIter iter;
		gpointer key, value;
		g_hash_table_iter_init (&iter, ayyi.priv->responders);
		while (g_hash_table_iter_next (&iter, &key, &value)){
			dbg(0, "  %s=%p", (char*)key, value);
		}
	}
#endif

	char* key = MAKE_RESPONDER_KEY(object_type, op_type, object_index);
	dbg(2, "key=%s %s", key, ayyi_print_optype(op_type));
	return g_hash_table_lookup(ayyi.priv->responders, key);
}


gboolean
ayyi_client_remove_responder(AyyiObjType object_type, AyyiOpType op_type, AyyiIdx object_idx, AyyiHandler callback)
{
	//remove the responder that matches AyyiObjType, AyyiOpType, and callback.

	char* key = MAKE_RESPONDER_KEY(object_type, op_type, object_idx);
	GList* responders = g_hash_table_lookup(ayyi.priv->responders, key);
	if(!responders){
		gwarn("not found: %i %i %i", object_type, op_type, object_idx);
		return FALSE;
	}

	gboolean found = FALSE;
	int length = g_list_length(responders);

	GList* l = responders;
	for(;l;l=l->next){
		Responder* responder = l->data;
		if(responder->callback == callback){
			g_free(responder);
			responders = g_list_remove(responders, responder);
			g_hash_table_replace(ayyi.priv->responders, key, responders);
			found = TRUE;
			break;
		}
	}
	dbg(2, "key=%s found=%i n_handlers=%i", key, found, g_list_length(responders));
	if(g_list_length(responders) != length - 1) gwarn("handler not removed?");

	if(!g_list_length(responders)){
		dbg(2, "no more handlers - removing responder... %s", ayyi_print_object_type(object_type), ayyi_print_optype(op_type));
		g_list_free(responders); //not needed?
		g_hash_table_remove(ayyi.priv->responders, key);
	}
	return TRUE;
}


void
ayyi_client_print_responders()
{
	char* find_first(char* key, char** pos)
	{
		char* s = key + 1;
		char* e = strstr(s, "-");
		*pos = e + 1;
		char* a = g_strndup(key, e - key);
		return a;
	}

	GHashTableIter iter;
	gpointer _key, value;
	g_hash_table_iter_init (&iter, ayyi.priv->responders);
	while (g_hash_table_iter_next (&iter, &_key, &value)){
		char* key = _key;
		char* pos;
		char* a = find_first(key, &pos);
		int object_type = atoi(a);
		dbg(0, "  %s %s", (char*)key, object_type > -1 ? ayyi_print_object_type(object_type) : "ALL");

		char* b = find_first(pos, &pos);
		//AyyiOp op_type = atoi(b);

		g_free(a);
		g_free(b);
	}
}


void
ayyi_discover_clients()
{
	PF;
}


AyyiIdx
ayyi_ident_get_idx(AyyiIdent ident)
{
	return ident.idx;
}


AyyiObjType
ayyi_ident_get_type(AyyiIdent ident)
{
	return ident.type;
}


void
ayyi_launch(AyyiLaunchInfo* info, gchar** argv)
{
	//argv - use NULL to launch the app without extra arguments.

	g_return_if_fail(info);
	dbg(0, "launching application: %s", info->name);

	int len = argv ? g_strv_length(argv) : 0;
	gchar** v = g_new0(gchar*, len + 2);
	v[0] = info->exec;

	if(argv){
		int i; for(i=0;i<len;i++){
			v[i+1] = argv[i];
		}
		dbg(0, "'%s'", g_strjoinv(" ", v));
	}else{
		//gchar args[2][256] = {"", ""};
		//argv = (char**)args;
		//argv[0] = info->exec;
		//v[1] = NULL;
	}

	GError* error = NULL;
	if(!g_spawn_async(NULL, v, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)){
		log_print(LOG_FAIL, "launch %s", info->name);
		GERR_WARN;
	}
	else log_print(0, "launched: %s", v[0]);

	g_free(v);
}


void
ayyi_launch_by_name(const char* name, gchar** argv)
{
	GList* l = ayyi.launch_info;
	for(;l;l=l->next){
		AyyiLaunchInfo* info = l->data;
		if(!strcmp(info->name, name)){
			ayyi_launch(info, argv);
			return;
		}
	}
	gwarn("not found");
}


gboolean
ayyi_verify_playlists()
{
	//TODO shm specific stuff not supposed to be here
	#define ayyi_song__playlist_next(A) (AyyiPlaylist*)ayyi_song_container_next_item(&ayyi.service->song->playlists, A)
	#define ayyi_song__audio_track_next(A) (AyyiTrack*)ayyi_song_container_next_item(&ayyi.service->song->audio_tracks, A)
	#define ayyi_song__midi_track_next(A) (AyyiMidiTrack*)ayyi_song_container_next_item(&ayyi.service->song->midi_tracks, A)
	extern AyyiTrack* ayyi_song__get_track_by_playlist (AyyiPlaylist*);

	AyyiPlaylist* playlist = NULL;
	while((playlist = ayyi_song__playlist_next(playlist))){
		AyyiTrack* track = ayyi_song__get_track_by_playlist(playlist);
		if(!track){
			return false;
			dbg(0, "failed for playlist: %s", playlist->name);
		}
	}
	return true;
}


const char*
ayyi_print_object_type (AyyiObjType object_type)
{

	static char unk[32];
  
	switch (object_type){
#define CASE(x) case AYYI_OBJECT_##x: return "AYYI_OBJECT_"#x
		CASE (EMPTY);
		CASE (TRACK);
		CASE (AUDIO_TRACK);
		CASE (MIDI_TRACK);
		CASE (CHAN);
		CASE (AUX);
		CASE (PART);
		CASE (AUDIO_PART);
		CASE (MIDI_PART);
		CASE (EVENT);
		CASE (RAW);
		CASE (STRING);
		CASE (ROUTE);
		CASE (FILE);
		CASE (LIST);
		CASE (MIDI_NOTE);
		CASE (SONG);
		CASE (TRANSPORT);
		CASE (AUTO);
		CASE (UNSUPPORTED);
		CASE (ALL);
#undef CASE
		default:
			snprintf (unk, 31, "UNKNOWN OBJECT TYPE (%d)", object_type);
			return unk;
	}
	return NULL;
}


const char*
ayyi_print_optype (AyyiOp op)
{
	static char* ops[] = {"", "AYYI_NEW", "AYYI_DEL", "AYYI_GET", "AYYI_SET", "AYYI_UNDO", "BAD OPTYPE"};
	return ops[MIN(op, 6)];
}


const char*
ayyi_print_prop_type (uint32_t prop_type)
{
	static char unk[32];

	switch (prop_type){
#define CASE(x) case AYYI_##x: return "AYYI_"#x
		CASE (NO_PROP);
		CASE (NAME);
		CASE (STIME);
		CASE (LENGTH);
		CASE (HEIGHT);
		CASE (COLOUR);
		CASE (END);
		CASE (TRACK);
		CASE (MUTE);
		CASE (ARM);
		CASE (SOLO);
		CASE (SDEF);
		CASE (INSET);
		CASE (FADEIN);
		CASE (FADEOUT);
		CASE (INPUT);
		CASE (OUTPUT);
		CASE (AUX1_OUTPUT);
		CASE (AUX2_OUTPUT);
		CASE (AUX3_OUTPUT);
		CASE (AUX4_OUTPUT);
		CASE (PREPOST);
		CASE (SPLIT);
		CASE (PB_LEVEL);
		CASE (PB_PAN);
		CASE (PB_DELAY_MU);
		CASE (PLUGIN_SEL);
		CASE (PLUGIN_BYPASS);
		CASE (CHAN_LEVEL);
		CASE (CHAN_PAN);
		CASE (TRANSPORT_PLAY);
		CASE (TRANSPORT_STOP);
		CASE (TRANSPORT_REW);
		CASE (TRANSPORT_FF);
		CASE (TRANSPORT_REC);
		CASE (TRANSPORT_LOCATE);
		CASE (TRANSPORT_CYCLE);
		CASE (TRANSPORT_LOCATOR);
		CASE (AUTO_PT);
		CASE (ADD_POINT);
		CASE (DEL_POINT);
		CASE (TEMPO);
		CASE (HISTORY);
		CASE (LOAD_SONG);
		CASE (SAVE);
		CASE (NEW_SONG);
#undef CASE
		default:
			snprintf (unk, 31, "UNKNOWN PROPERTY (%d)", prop_type);
	}
	return unk;
}


const char*
ayyi_print_media_type (uint32_t type)
{
	static char* types[] = {"[Media type not set]", "AUDIO", "MIDI"};
	if(G_N_ELEMENTS(types) != AYYI_MIDI + 1) gerr("!!");
	return types[type];
}


