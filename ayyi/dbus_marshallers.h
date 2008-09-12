
#ifndef ___ad_dbus_marshal_MARSHAL_H__
#define ___ad_dbus_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:STRING,INT (dbus_marshallers.list:2) */
extern void _ad_dbus_marshal_VOID__STRING_INT (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

/* VOID:INT,INT (dbus_marshallers.list:3) */
extern void _ad_dbus_marshal_VOID__INT_INT (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data);

/* VOID:INT,INT,INT (dbus_marshallers.list:6) */
extern void _ad_dbus_marshal_VOID__INT_INT_INT (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

G_END_DECLS

#endif /* ___ad_dbus_marshal_MARSHAL_H__ */

