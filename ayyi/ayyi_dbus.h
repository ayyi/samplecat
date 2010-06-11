#ifndef __ayyi_dbus_h__
#define __ayyi_dbus_h__

#include "ayyi/ayyi_client.h"
#include <dbus/dbus-glib-bindings.h>
#define DBUS_STRUCT_UINT_BYTE_BYTE_UINT_UINT (dbus_g_type_get_struct ("GValueArray", G_TYPE_UINT, G_TYPE_UCHAR, G_TYPE_UCHAR, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID))

#define P_IN printf("%s--->%s ", green, white)
#define SIG_IN printf("%s--->%s %s\n", bold, white, __func__)
#define SIG_IN1 printf("%s--->%s ", bold, white)
#define SIG_OUT printf("%s(): %s--->%s\n", __func__, yellow, white)
#define UNEXPECTED_PROPERTY gwarn("unexpected property: obj_type=%s prop=%s", ayyi_print_object_type(object_type), ayyi_print_prop_type(prop));

/*
struct _service
{
	char*       service;
	char*       path;
	char*       interface;

	DBusGProxy* proxy;

	void        (*on_shm)    ();
	void        (*on_new)    (int object_type, AyyiIdx);
	void        (*on_delete) (int object_type, AyyiIdx);
	void        (*on_change) (int object_type, AyyiIdx);

	//instance values (service is singleton, so class and instance are the same):

	struct _shm_seg_mixer*  amixer;
	struct _shm_song*       song;
};
*/

#ifndef __ayyi_dbus_c__
extern Service known_services[];
#endif
#define AYYI_SERVICE_AYYID1 0

gboolean        ayyi_client__dbus_connect (Service*, GError** error);
gboolean        ayyi_client__dbus_get_shm (Service*);

DBusGProxyCall* dbus_set_notelistX        (AyyiAction*, uint32_t object_idx, const GList*);
DBusGProxyCall* dbus_set_notelist         (AyyiAction*, uint32_t part_idx, const GPtrArray* notes);

#endif //__ayyi_dbus_h__
