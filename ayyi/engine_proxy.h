#include <dbus/dbus-glib-bindings.h>
#include <ayyi/ayyi_dbus.h>

//void            dbus_object_new     (AyyiAction*, char* name, gboolean from_source, uint32_t src_idx, guint32 parent_idx, struct _song_pos* stime, guint64 len, guint32 inset);
void            dbus_object_new     (struct _ayyi_action*, char* name, gboolean from_source, uint32_t src_idx, guint32 parent_idx, struct _song_pos* stime, guint64 len, guint32 inset);
void            dbus_object_del     (struct _ayyi_action*, int object_type, uint32_t object_idx);

DBusGProxyCall* dbus_get_prop_string(struct _ayyi_action*, int object_type, int property_type);
DBusGProxyCall* dbus_set_prop_int   (struct _ayyi_action*, int object_type, int property_type, uint32_t object_idx, int val);
DBusGProxyCall* dbus_set_prop_int64 (struct _ayyi_action*, int object_type, int property_type, uint32_t object_idx, int64_t val);
DBusGProxyCall* dbus_set_prop_float (struct _ayyi_action*, int object_type, int property_type, uint32_t object_idx, double val);
DBusGProxyCall* dbus_set_prop_bool  (struct _ayyi_action*, int object_type, int property_type, uint32_t object_idx, gboolean val);
DBusGProxyCall* dbus_set_prop_string(struct _ayyi_action*, int object_type, int property_type, uint32_t object_idx, const char* val);
DBusGProxyCall* dbus_set_prop_3i_2f (struct _ayyi_action*, int object_type, int property_type, uint32_t object_idx, uint32_t idx2, uint32_t idx3, double val1, double val2);

void            dbus_undo           (Service*, int n);
