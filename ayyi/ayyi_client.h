#ifndef __ayyi_client_h__
#define __ayyi_client_h__

struct _ayyi_client
{
	GList*                    segs;
	struct _shm_song*         song;
	struct _shm_seg_mixer*    amixer;

	int                       got_song;   //TRUE when we must have _all_ required shm pointers.
};

typedef struct plugin {
	guint		api_version;
	gchar*      name;
	guint       type;          // plugin type (e.g. renderer, parser or feed list provider)

	gchar*      service_name;
	gchar*      app_path;
	gchar*      interface;

	void*       symbols;       // plugin type specific symbol table

	void*       client_data;   // currently used to store the local shm address;
} *pluginPtr;


void      ayyi_client_init         ();
void      ayyi_client_load_plugins ();
pluginPtr ayyi_client_get_plugin   (const char*);

void      ayyi_object_set_bool     (uint32_t object_type, uint32_t _obj_id, int _prop, gboolean val, struct _ayyi_action*);

const char* ayyi_print_object_type (uint32_t type);
const char* ayyi_print_prop_type   (uint32_t type);
const char* ayyi_print_media_type  (uint32_t type);

#ifdef _ayyi_client_c_
struct _ayyi_client ayyi;
#else
extern struct _ayyi_client ayyi;
#endif

typedef struct {
    guint       api_version;
    char*       name;
    char*       (*get_hello)();
	float       (*get_meterlevel)();
} SpectrogramSymbols;

#endif //__ayyi_client_h__
