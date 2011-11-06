#ifndef __ayyi_dbus_h__
#define __ayyi_dbus_h__

#include "ayyi/ayyi_client.h"
#include <dbus/dbus-glib-bindings.h>
#define DBUS_STRUCT_UINT_BYTE_BYTE_UINT_UINT (dbus_g_type_get_struct ("GValueArray", G_TYPE_UINT, G_TYPE_UCHAR, G_TYPE_UCHAR, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID))

#define P_IN {if(ayyi.log.to_stdout) printf("%s--->%s ", green, white); }
#define SIG_IN {if(ayyi.log.to_stdout) printf("%s--->%s %s\n", bold, white, __func__); }
#define SIG_IN1 {if(ayyi.debug) printf("%s--->%s ", bold, white); }
#define SIG_OUT {if(ayyi.debug) printf("%s(): %s--->%s\n", __func__, yellow, white); }

#ifndef __ayyi_dbus_c__
extern Service known_services[];
#endif

#ifdef __ayyi_client_c__
gboolean        ayyi_client__dbus_connect (Service*, GError** error);
void            ayyi_dbus_ping            (Service*, void (*pong)(const char* s));
#endif
gboolean        ayyi_client__dbus_get_shm (Service*);
#ifdef __ayyi_private__
void            ayyi_dbus__register_signals(int);
#endif

DBusGProxyCall* dbus_set_notelistX        (AyyiAction*, uint32_t object_idx, const GList*);
DBusGProxyCall* dbus_set_notelist         (AyyiAction*, uint32_t part_idx, const GPtrArray* notes);

#endif //__ayyi_dbus_h__
