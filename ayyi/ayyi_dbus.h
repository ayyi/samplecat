#ifndef __ayyi_dbus_h__
#define __ayyi_dbus_h__

#include <dbus/dbus-glib-bindings.h>
#define DBUS_STRUCT_UINT_BYTE_BYTE_UINT_UINT (dbus_g_type_get_struct ("GValueArray", G_TYPE_UINT, G_TYPE_UCHAR, G_TYPE_UCHAR, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID))

#define P_IN printf("%s--->%s ", green, white)
#define SIG_IN printf("%s--->%s %s\n", bold, white, __func__)
#define SIG_IN1 printf("%s--->%s ", bold, white)
#define SIG_OUT printf("%s(): %s--->%s\n", __func__, yellow, white)
#define UNEXPECTED_PROPERTY gwarn("unexpected property: obj_type=%s prop=%s", ayyi_print_object_type(object_type), ayyi_print_prop_type(prop));

typedef struct _service
{
	char*       service;
	char*       path;
	char*       interface;

	DBusGProxy* proxy;

	void        (*on_shm)();
} Service;

#ifndef __ayyi_dbus_c__
extern Service known_services[];
#endif
#define AYYI_SERVICE_AYYID1 0

gboolean        dbus_server_connect (Service*, GError** error);
gboolean        dbus_server_get_shm (Service*);

DBusGProxyCall* dbus_set_notelistX  (struct _ayyi_action*, uint32_t object_idx, const GList*);
DBusGProxyCall* dbus_set_notelist   (struct _ayyi_action*, uint32_t part_idx, const GPtrArray* notes);

#endif //__ayyi_dbus_h__
