#define _ayyi_client_c_
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <glib.h>
#include <gmodule.h>

typedef void            action;
typedef struct _shm_seg shm_seg;
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/ayyi_msg.h>
#include <ayyi/ayyi_dbus.h>

#include <ayyi/ayyi_client.h>

extern struct _ayyi_client ayyi;
#define plugin_path "/usr/lib/ayyi/"

static GSList* plugins = NULL; // loaded plugins

typedef	pluginPtr (*infoFunc)();

static pluginPtr plugin_load(const gchar* filename);

enum {
    PLUGIN_TYPE_1 = 1,
    PLUGIN_TYPE_MAX
};
#define PLUGIN_API_VERSION 1

SpectrogramSymbols* spectrogram_symbols = NULL;


void
ayyi_client_init()
{
	ayyi.song = NULL;
}


void
ayyi_client_load_plugins()
{
	pluginPtr plugin = NULL;
	GError* error = NULL;

	if(!g_module_supported()) g_error("Modules not supported! (%s)", g_module_error());

	if(!g_file_test(plugin_path, G_FILE_TEST_EXISTS)) return;

	int found = 0;
	dbg(0, "scanning for plugins (%s) ...", plugin_path);
	GDir* dir = g_dir_open(plugin_path, 0, &error);
	if (!error) {
		gchar* filename = (gchar*)g_dir_read_name(dir);
		while (filename) {
			dbg(2, "testing %s...", filename);
			// filter files with correct library suffix
			if(!strncmp(G_MODULE_SUFFIX, filename + strlen(filename) - strlen(G_MODULE_SUFFIX), strlen(G_MODULE_SUFFIX))) {
				// If we find one, try to load plugin info and if this was successful try to invoke the specific plugin
				// type loader. If the second loading went well add the plugin to the plugin list.
				if (!(plugin = plugin_load(filename))) {
					dbg(2, "-> %s not valid plugin! (plugin_load() failed.)", filename);
				} else {
					//dbg(0, "-> %s (%s, type=%d)", plugin->name, filename, plugin->type);
					found++;
					plugins = g_slist_append(plugins, plugin);
				}
			} else {
				dbg(2, "-> no library suffix");
			}
			filename = (gchar *)g_dir_read_name(dir);
		}
		g_dir_close(dir);
	} else {
		gwarn("dir='%s' failed. %s", plugin_path, error->message );
		g_error_free(error);
		error = NULL;
	}
	dbg(0, "ayyi plugins found: %i.", found);
}


pluginPtr
ayyi_client_get_plugin(const char* name)
{
	//lookup the plugin pointer for a loaded plugin.

	static int count = -1;
	count++;

	GSList* l = plugins;
	for(;l;l=l->next){
		pluginPtr plugin = l->data;
		if(!count) dbg(2, "  %s", plugin->name);
		if(!strcmp(name, plugin->name)) return plugin;
	}
	return NULL;
}


static pluginPtr
plugin_load(const gchar* filename)
{
	pluginPtr plugin = NULL;
	GModule*  handle = NULL;
	infoFunc  plugin_get_info;
	gboolean  success = FALSE;

	gchar* path = g_strdup_printf(plugin_path G_DIR_SEPARATOR_S "%s", filename);

#if GLIB_CHECK_VERSION(2,3,3)
	handle = g_module_open(path, G_MODULE_BIND_LOCAL);
#else
	handle = g_module_open(path, 0);
#endif

	g_free(path);

	if(!handle) {
		gwarn("Cannot open %s%s (%s)!", plugin_path G_DIR_SEPARATOR_S, filename, g_module_error());
		return NULL;
	}

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
						dbg(2, "name='%s' %s", plugin->name, plugin->service_name);
						spectrogram_symbols = plugin->symbols;
						dbg(2, "hello=%s", spectrogram_symbols->get_hello());

						Service pservice = {plugin->service_name, plugin->app_path, plugin->interface, NULL};
						GError* error = NULL;
						if((dbus_server_connect(&pservice, &error))){
							dbg(0, "plugin dbus connection ok.");
							success = TRUE; // no special initialization

							//dbus_register_signals();
							//dbus_server_get_shm(&engine);  //.....err shouldnt we be using this?

							//get shm address:

							plugin->client_data = (struct _spec_shm*)NULL;

							int type = 3;
							shm_seg* seg = shm_seg_new(0, type);

							if(dbus_g_proxy_call(pservice.proxy, "GetShmSingle", &error, G_TYPE_STRING, "", G_TYPE_INVALID, G_TYPE_UINT, &seg->id, G_TYPE_INVALID)){
								ayyi_shm_import();
								plugin->client_data = seg->address; //TODO does it need to be translated? no, dont think so.
								dbg(0, "client_data=%p offset=%i", plugin->client_data, (unsigned)plugin->client_data - (unsigned)plugin->client_data);
							} else {
								if(error){
									dbg (0, "GetShm: %s\n", error->message);
									g_error_free(error);
								}
								success = FALSE;
							}
						}
						else warn_gerror("plugin dbus connection failed", &error);
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

void
ayyi_object_set_bool(uint32_t object_type, uint32_t obj_id, int prop, gboolean val, struct _ayyi_action* action)
{
#ifdef USE_DBUS
	dbus_set_prop_bool(action, object_type, prop, obj_id, val);
	return 0;
#endif
}


const char*
ayyi_print_object_type (uint32_t object_type)
{
	static char* types[] = {"AYYI_OBJECT_EMPTY", "AYYI_OBJECT_TRACK", "AYYI_OBJECT_AUDIO_TRACK", "AYYI_OBJECT_MIDI_TRACK", "AYYI_OBJECT_CHAN", "AYYI_OBJECT_AUX", "AYYI_OBJECT_PART", "AYYI_OBJECT_AUDIO_PART", "AYYI_OBJECT_MIDI_PART", "AYYI_OBJECT_EVENT", "AYYI_OBJECT_RAW", "AYYI_OBJECT_STRING", "AYYI_OBJECT_ROUTE", "AYYI_OBJECT_FILE", "AYYI_OBJECT_LIST", "AYYI_OBJECT_MIDI_NOTE", "AYYI_OBJECT_SONG", "AYYI_OBJECT_TRANSPORT", "AYYI_OBJECT_AUTO", "AYYI_OBJECT_UNSUPPORTED", "AYYI_OBJECT_ALL"};
	if(G_N_ELEMENTS(types) != AYYI_OBJECT_ALL + 1) gerr("!!");
	return types[object_type];
}


const char*
ayyi_print_prop_type (uint32_t prop_type)
{
	static char* types[] = {"AYYI_NO_PROP", "AYYI_NAME", "AYYI_STIME", "AYYI_LENGTH", "AYYI_HEIGHT", "AYYI_END", "AYYI_TRACK", "AYYI_MUTE", "AYYI_ARM", "AYYI_SOLO", 
	                        "AYYI_SDEF", "AYYI_INSET", "AYYI_FADEIN", "AYYI_FADEOUT", "AYYI_OUTPUT",
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
	if(G_N_ELEMENTS(types) != AYYI_NEW_SONG + 1) gerr("!!");
	return types[prop_type];
}


const char*
ayyi_print_media_type (uint32_t type)
{
	static char* types[] = {"[Media type not set]", "AUDIO", "MIDI"};
	if(G_N_ELEMENTS(types) != AYYI_MIDI + 1) gerr("!!");
	return types[type];
}


