#ifndef __ayyi_client_h__
#define __ayyi_client_h__

#include "ayyi_log.h"

#define ASSERT_SONG_SHM  if(!ayyi.got_shm){ errprintf2(__func__, "no song shm.\n"); return; }
#define ASSERT_SONG_SHM_RET_FALSE  if(!ayyi.got_shm){ errprintf2(__func__, "no song shm.\n"); return FALSE; }

#if (defined __ayyi_client_c__ || defined __ayyi_action_c__ || defined __ayyi_shm_c__ || defined __am_message_c__ )
struct _AyyiClientPrivate
{
	GSList*                 plugins;    // loaded Ayyi plugins

	GList*                  actionlist; // list of AyyiAction*
	unsigned                trans_id;

	gboolean                (*on_shm)(struct _shm_seg*); //used for segment verification.

	GHashTable*             responders;
};
#endif

#if (defined __ayyi_dbus_c__ || defined __ayyi_client_c__ || defined __engine_proxy_c__ )
#include <dbus/dbus-glib-bindings.h>
struct _AyyiServicePrivate
{
	DBusGProxy*             proxy;
};
#endif

typedef struct _AyyiClientPrivate AyyiClientPrivate;
typedef struct _AyyiServicePrivate AyyiServicePrivate;

struct _ayyi_client
{
	Service*                service;    // will change to a list later.

	int                     got_shm;    // TRUE when we must have _all_ required shm pointers.
	gboolean                offline;    // TRUE if we are not connected and should not attempt to connect to remote service.

	int                     debug;      // debug level. 0=off.
	struct _log             log;

	GList*                  launch_info;// type AyyiLaunchInfo

	AyyiClientPrivate*      priv;
};

struct _service
{
	char*                   service;
	char*                   path;
	char*                   interface;

	void                    (*on_shm)    (); // application level callback
	void                    (*on_new)    (AyyiObjType, AyyiIdx);
	void                    (*on_delete) (AyyiObjType, AyyiIdx);
	void                    (*on_change) (AyyiObjType, AyyiIdx);

	//instance values (service is singleton, so class and instance are the same):

	GList*                  segs;
	struct _shm_seg_mixer*  amixer;
	struct _shm_song*       song;

	AyyiServicePrivate*     priv;
};

typedef struct plugin {
	guint       api_version;
	gchar*      name;
	guint       type;          // plugin type (e.g. renderer, parser or feed list provider)

	gchar*      service_name;
	gchar*      app_path;
	gchar*      interface;

	void*       symbols;       // plugin type specific symbol table

	void*       client_data;   // currently used to store the local shm address;
} *AyyiPluginPtr;

typedef struct _responder
{
	AyyiHandler callback;
	gpointer    user_data;
} Responder;


AyyiClient*  ayyi_client_init                 ();
void         ayyi_client_load_plugins         ();
AyyiPluginPtr ayyi_client_get_plugin          (const char*);

void         ayyi_object_set_bool             (AyyiObjType, AyyiIdx, int prop, gboolean val, AyyiAction*);
gboolean     ayyi_object_is_deleted           (AyyiRegionBase*);

void         ayyi_client_add_responder        (AyyiObjType, AyyiOpType, AyyiIdx, AyyiHandler, gpointer);
void         ayyi_client_add_onetime_responder(AyyiIdent, AyyiOpType, AyyiHandler, gpointer);
const GList* ayyi_client_get_responders       (AyyiObjType, AyyiOpType, AyyiIdx);
gboolean     ayyi_client_remove_responder     (AyyiObjType, AyyiOpType, AyyiIdx, AyyiHandler);

void         ayyi_discover_clients            ();

AyyiIdx      ayyi_ident_get_idx               (AyyiIdent);
AyyiObjType  ayyi_ident_get_type              (AyyiIdent);

void         ayyi_launch                      (AyyiLaunchInfo*, gchar** argv);

const char*  ayyi_print_object_type           (AyyiObjType);
const char*  ayyi_print_optype                (AyyiOpType);
const char*  ayyi_print_prop_type             (AyyiPropType);
const char*  ayyi_print_media_type            (MediaType);

#ifdef __ayyi_client_c__
struct _ayyi_client ayyi;
#else
extern struct _ayyi_client ayyi;
#endif

#endif //__ayyi_client_h__
