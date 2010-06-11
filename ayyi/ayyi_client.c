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
#define __ayyi_client_c__
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
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/ayyi_msg.h>
#include <ayyi/ayyi_dbus.h>
#include <ayyi/ayyi_log.h>

#include <ayyi/ayyi_client.h>

extern struct _ayyi_client ayyi;
#define plugin_path "/usr/lib/ayyi/:/usr/local/lib/ayyi/"

typedef	AyyiPluginPtr (*infoFunc)();

static AyyiPluginPtr plugin_load(const gchar* filename);
static void          ayyi_client_get_launch_info();

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

	ayyi_client_get_launch_info();

	return &ayyi;
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
			gchar* filename = (gchar*)g_dir_read_name(dir);
			while (filename) {
				dbg(0, "testing %s...", filename);
				// filter files with correct library suffix
				if(!strncmp(G_MODULE_SUFFIX, filename + strlen(filename) - strlen(G_MODULE_SUFFIX), strlen(G_MODULE_SUFFIX))) {
					// If we find one, try to load plugin info and if this was successful try to invoke the specific plugin
					// type loader. If the second loading went well add the plugin to the plugin list.
					if (!(plugin = plugin_load(filename))) {
						dbg(0, "-> %s not valid plugin! (plugin_load() failed.)", filename);
					} else {
						//dbg(0, "-> %s (%s, type=%d)", plugin->name, filename, plugin->type);
						found++;
						ayyi.priv->plugins = g_slist_append(ayyi.priv->plugins, plugin);
					}
				} else {
					dbg(2, "-> no library suffix");
				}
				filename = (gchar*)g_dir_read_name(dir);
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
plugin_load(const gchar* filename)
{
	AyyiPluginPtr plugin = NULL;
	gboolean success = FALSE;

	gchar* path = g_strdup_printf(plugin_path G_DIR_SEPARATOR_S "%s", filename);

#if GLIB_CHECK_VERSION(2,3,3)
	GModule* handle = g_module_open(path, G_MODULE_BIND_LOCAL);
#else
	GModule* handle = g_module_open(path, 0);
#endif

	g_free(path);

	if(!handle) {
		gwarn("Cannot open %s%s (%s)!", plugin_path G_DIR_SEPARATOR_S, filename, g_module_error());
		return NULL;
	}

	infoFunc plugin_get_info;
	if(g_module_symbol(handle, "plugin_get_info", (void*)&plugin_get_info)) {
		// load generic plugin info
		if(NULL != (plugin = (*plugin_get_info)())) {
			// check plugin version
			if(PLUGIN_API_VERSION != plugin->api_version){
				dbg(0, "API version mismatch: \"%s\" (%s, type=%d) has version %d should be %d", plugin->name, filename, plugin->type, plugin->api_version, PLUGIN_API_VERSION);
			}

			// try to load specific plugin type symbols
			switch(plugin->type) {
				//case PLUGIN_TYPE_1:
				//	success = htmlview_plugin_register (plugin, handle);
				//	break;
				default:
					if(plugin->type >= PLUGIN_TYPE_MAX) {
						dbg(0, "Unknown or unsupported plugin type: %s (%s, type=%d)", plugin->name, filename, plugin->type);
					} else {
						dbg(0, "name='%s' %s", plugin->name, plugin->service_name);

						Service pservice = {plugin->service_name, plugin->app_path, plugin->interface, NULL};
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
								dbg(2, "client_data=%p offset=%i", plugin->client_data, (unsigned)plugin->client_data - (unsigned)plugin->client_data);
							} else {
								if(error){
									dbg (0, "GetShm: %s\n", error->message);
									g_error_free(error);
								}
								success = FALSE;
							}
						}
						else{
                    switch(error->code){
                      case 98742:
                        log_print(0, "plugin service not available: %s", plugin->service_name);
                        break;
                      default:
                        warn_gerror("plugin dbus connection failed", &error);
                        break;
                    }
                  }
					}
					break;
			}
		}
	} else {
		gwarn("File '%s' is not a valid Ayyi plugin!", filename);
	}
	
	if(!success) {
		g_module_close(handle);
		return NULL;
	}
		
	return plugin;
}


static void
ayyi_client_get_launch_info()
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
ayyi_object_set_bool(AyyiObjType object_type, AyyiIdx obj_idx, int prop, gboolean val, AyyiAction* action)
{
#ifdef USE_DBUS
	dbus_set_prop_bool(action, object_type, prop, obj_idx, val);
	return 0;
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
ayyi_client_add_responder(AyyiObjType object_type, AyyiOpType op_type, AyyiIdx object_index, AyyiHandler callback, gpointer user_data)
{
	char* key = MAKE_RESPONDER_KEY(object_type, op_type, object_index);
	dbg(2, "key=%s", key);
	Responder* responder = g_new0(Responder, 1);
	responder->callback = callback;
	responder->user_data = user_data;

	GList* responders = g_list_append(g_hash_table_lookup(ayyi.priv->responders, key), responder);
	g_hash_table_insert(ayyi.priv->responders, key, responders);

#if 0
	{
		GList* handlers = (GList*)ayyi_client_get_responders(object_type, op_type, object_index);
		dbg(0, "%s n_handlers=%i", ayyi_print_object_type(object_type), g_list_length(handlers));
	}
#endif
}


void
ayyi_client_add_onetime_responder(AyyiIdent id, AyyiOpType op_type, AyyiHandler handler, gpointer user_data)
{
	//deletes the handler after first response.
	//TODO does it need a timeout?

	struct _data
	{
		AyyiHandler handler;
		void*       user_data;
		AyyiOpType  op_type;
	};

	void onetime_responder_callback(AyyiObjType o, AyyiIdx i, gpointer _data)
	{
		struct _data* data = (struct _data*)_data;

		AyyiHandler handler = data->handler;
		if(handler) handler(o, i, data->user_data);

		if(!ayyi_client_remove_responder(o, data->op_type, i, handler)){
			gwarn("responder not removed.");
		}

		g_free(data);
	}

	struct _data* data = g_new0(struct _data, 1);
	data->handler = onetime_responder_callback;
	data->user_data = user_data;
	data->op_type = op_type;

	ayyi_client_add_responder(id.type, op_type, id.idx, handler, data);
}


const GList*
ayyi_client_get_responders(AyyiObjType object_type, AyyiOpType op_type, AyyiIdx object_index)
{
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
	if(!responders) return FALSE;

	gboolean found = FALSE;
	int length = g_list_length(responders);

	GList* l = responders;
	for(;l;l=l->next){
		Responder* responder = l->data;
		if(responder->callback == callback){
			g_free(responder);
			responders = g_list_remove(responders, responder);
			found = TRUE;
			break;
		}
	}
	dbg(0, "key=%s found=%i n_handlers=%i", key, found, g_list_length(responders));
	if(g_list_length(responders) != length - 1) gwarn("handler not removed?");

	if(!g_list_length(responders)){
		dbg(0, "no more handlers - removing responder... %s", ayyi_print_object_type(object_type), ayyi_print_optype(op_type));
		g_list_free(responders); //not needed?
		g_hash_table_remove(ayyi.priv->responders, key);
	}
	return TRUE;
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

	if(!argv){
		gchar args[2][256] = {"", ""};
		argv = (char**)args;
		argv[0] = info->exec;
		argv[1] = NULL;
	}

    GError* error = NULL;
    if(!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)){
      log_print(LOG_FAIL, "launch %s", info->name);
      GERR_WARN;
    }
	else log_print(0, "launched: %s", argv[0]);
}


const char*
ayyi_print_object_type (AyyiObjType object_type)
{
	static char* types[] = {"AYYI_OBJECT_EMPTY", "AYYI_OBJECT_TRACK", "AYYI_OBJECT_AUDIO_TRACK", "AYYI_OBJECT_MIDI_TRACK", "AYYI_OBJECT_CHAN", "AYYI_OBJECT_AUX", "AYYI_OBJECT_PART", "AYYI_OBJECT_AUDIO_PART", "AYYI_OBJECT_MIDI_PART", "AYYI_OBJECT_EVENT", "AYYI_OBJECT_RAW", "AYYI_OBJECT_STRING", "AYYI_OBJECT_ROUTE", "AYYI_OBJECT_FILE", "AYYI_OBJECT_LIST", "AYYI_OBJECT_MIDI_NOTE", "AYYI_OBJECT_SONG", "AYYI_OBJECT_TRANSPORT", "AYYI_OBJECT_AUTO", "AYYI_OBJECT_UNSUPPORTED", "AYYI_OBJECT_ALL"};
	if(G_N_ELEMENTS(types) != AYYI_OBJECT_ALL + 1) gerr("!!");
	return types[object_type];
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
	if(G_N_ELEMENTS(types) != AYYI_NEW_SONG + 1) gerr("size mismatch!!");
	return types[prop_type];
}


const char*
ayyi_print_media_type (uint32_t type)
{
	static char* types[] = {"[Media type not set]", "AUDIO", "MIDI"};
	if(G_N_ELEMENTS(types) != AYYI_MIDI + 1) gerr("!!");
	return types[type];
}


