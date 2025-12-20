/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2025-2026 Tim Orford <tim@orford.org>                  |
 | copyright (C) 2020 GNOME Foundation                                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <gio/gio.h>
#include "debug/debug.h"
#include <atspi/atspi-constants.h>
#include "atspi/application.h"
#include "atspi/accessible.h"
#include "atspi.h"

#define ATSPI_VERSION        "2.1"

#define ATSPI_PATH_PREFIX    "/org/a11y/atspi"
#define ATSPI_ROOT_PATH      ATSPI_PATH_PREFIX "/accessible/root"
#define ATSPI_CACHE_PATH     ATSPI_PATH_PREFIX "/cache"
#define ATSPI_REGISTRY_PATH  ATSPI_PATH_PREFIX "/registry"
#define base_path            "/org/ayyi/application/samplecatgl/a11y"

static AGlScene* scene;
static GDBusConnection* connection;
static guint register_id;
static char* desktop_name;
static char* desktop_path;
static char* bus_address;
static gint32 application_id;
static GHashTable* event_listeners; // HashTable<str, uint>
static char* context_path;

static void destructor (void) __attribute__ ((destructor));


static char*
get_bus_address ()
{
	dbg(0, "Acquiring a11y bus via DBus...");

	g_autoptr(GError) error = NULL;
	g_autoptr(GDBusConnection) connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);

	if (error) {
		g_warning ("Unable to acquire session bus: %s", error->message);
		g_error_free (error);
		return NULL;
	}

	GVariant* res = g_dbus_connection_call_sync (connection, "org.a11y.Bus", "/org/a11y/bus", "org.a11y.Bus", "GetAddress", NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if (error) {
		pwarn("Unable to acquire the address of the accessibility bus: %s", error->message);
	}

	char* address = NULL;
	if (res) {
		g_variant_get (res, "(s)", &address);
		g_variant_unref (res);
    }

	return address;
}

static void
handle_application_method (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name, const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data)
{
	if (g_strcmp0 (method_name, "GetLocale") == 0) {

		int types[] = {/* LC_MESSAGES, LC_COLLATE, LC_CTYPE, LC_MONETARY, LC_NUMERIC, LC_TIME*/ };

		guint lctype;
		g_variant_get (parameters, "(u)", &lctype);
		if (lctype >= G_N_ELEMENTS (types)) {
			g_dbus_method_invocation_return_error (invocation, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "Not a known locale facet: %u", lctype);
			return;
		}

#if 0
		const char* locale = setlocale (types[lctype], NULL);
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", locale));
#endif
	}
}

static GVariant*
handle_application_get_property (GDBusConnection* connection, const gchar* sender, const gchar* object_path, const gchar* interface_name, const gchar* property_name, GError** error, gpointer user_data)
{
	GVariant* res = NULL;

	if (g_strcmp0 (property_name, "Id") == 0)
		res = g_variant_new_int32 (application_id);
	else if (g_strcmp0 (property_name, "ToolkitName") == 0)
		res = g_variant_new_string ("AGl");
	else if (g_strcmp0 (property_name, "Version") == 0)
		res = g_variant_new_string ("0.0.1");
	else if (g_strcmp0 (property_name, "AtspiVersion") == 0)
		res = g_variant_new_string (ATSPI_VERSION);
  else
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Unknown property '%s'", property_name);

	return res;
}

static gboolean
handle_application_set_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name, const gchar *property_name, GVariant *value, GError **error, gpointer user_data)
{
	if (g_strcmp0 (property_name, "Id") == 0) {
		g_variant_get (value, "i", &application_id);
	} else {
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Invalid property '%s'", property_name);
		return FALSE;
	}

	return TRUE;
}

static void
handle_accessible_method (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name, const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data)
{
	if (g_strcmp0 (method_name, "GetRole") == 0)
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(u)", ATSPI_ROLE_APPLICATION));
	else if (g_strcmp0 (method_name, "GetRoleName") == 0)
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", "application"));
	else if (g_strcmp0 (method_name, "GetLocalizedRoleName") == 0)
	    g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", "accessibility application"));
	else if (g_strcmp0 (method_name, "GetState") == 0) {
		GVariantBuilder builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE ("(au)"));

		g_variant_builder_open (&builder, G_VARIANT_TYPE ("au"));
		g_variant_builder_add (&builder, "u", 0);
		g_variant_builder_add (&builder, "u", 0);
		g_variant_builder_close (&builder);

		g_dbus_method_invocation_return_value (invocation, g_variant_builder_end (&builder));

	} else if (g_strcmp0 (method_name, "GetAttributes") == 0) {
		GVariantBuilder builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE ("(a{ss})"));

		g_variant_builder_open (&builder, G_VARIANT_TYPE ("a{ss}"));
		g_variant_builder_add (&builder, "{ss}", "toolkit", "AGl");
		g_variant_builder_close (&builder);

		g_dbus_method_invocation_return_value (invocation, g_variant_builder_end (&builder));

	} else if (g_strcmp0 (method_name, "GetApplication") == 0) {
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("((so))", desktop_name, desktop_path));
	} else if (g_strcmp0 (method_name, "GetChildAtIndex") == 0) {
		int idx;
		g_variant_get (parameters, "(i)", &idx);

		guint n_toplevels = 1;
		if (idx >= n_toplevels)
			return;

		const char* name = g_dbus_connection_get_unique_name (connection);

		g_dbus_method_invocation_return_value (invocation, g_variant_new ("((so))", name, context_path));

	} else if (g_strcmp0 (method_name, "GetChildren") == 0) {
		GVariantBuilder builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE ("a(so)"));

		const char* name = g_dbus_connection_get_unique_name (connection);

		g_variant_builder_add (&builder, "(so)", name, context_path);

		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(a(so))", &builder));
	} else if (g_strcmp0 (method_name, "GetIndexInParent") == 0) {
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(i)", -1));
	} else if (g_strcmp0 (method_name, "GetRelationSet") == 0) {
		GVariantBuilder builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE ("a(ua(so))"));
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(a(ua(so)))", &builder));
	} else if (g_strcmp0 (method_name, "GetInterfaces") == 0) {
		GVariantBuilder builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE ("as"));

		g_variant_builder_add (&builder, "s", atspi_accessible_interface.name);
		g_variant_builder_add (&builder, "s", atspi_application_interface.name);

		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(as)", &builder));
	}
}

static GVariant *
handle_accessible_get_property (GDBusConnection* connection, const gchar* sender, const gchar* object_path, const gchar* interface_name, const gchar* property_name, GError** error, gpointer user_data)
{
	GVariant* res = NULL;

	if (g_strcmp0 (property_name, "Name") == 0)
		res = g_variant_new_string ("samplecatgl");

	else if (g_strcmp0 (property_name, "Description") == 0)
		res = g_variant_new_string (g_get_application_name () ? g_get_application_name () : "No description");

	else if (g_strcmp0 (property_name, "Locale") == 0)
		res = g_variant_new_string ("");

	else if (g_strcmp0 (property_name, "AccessibleId") == 0) {
		const char *id = NULL;
		GApplication *application = g_application_get_default ();
		if (application)
			id = g_application_get_application_id (application);

		res = g_variant_new_string (id ? id : "");

	} else if (g_strcmp0 (property_name, "Parent") == 0)
		res = g_variant_new ("(so)", desktop_name, desktop_path);

	else if (g_strcmp0 (property_name, "ChildCount") == 0) {
		res = g_variant_new_int32 (g_list_length(((AGlActor*)scene)->children));

	} else
	    g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Unknown property '%s'", property_name);

	return res;
}

static const GDBusInterfaceVTable root_application_vtable = {
	handle_application_method,
	handle_application_get_property,
	handle_application_set_property,
};

static const GDBusInterfaceVTable root_accessible_vtable = {
	handle_accessible_method,
	handle_accessible_get_property,
	NULL,
};

static void
on_event_listener_registered (GDBusConnection *connection, const char *sender_name, const char *object_path, const char *interface_name, const char *signal_name, GVariant *parameters, gpointer user_data)
{
	if (g_strcmp0 (object_path, ATSPI_REGISTRY_PATH) == 0 &&
		g_strcmp0 (interface_name, "org.a11y.atspi.Registry") == 0 &&
		g_strcmp0 (signal_name, "EventListenerRegistered") == 0)
    {

		if (!event_listeners)
			event_listeners = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

		const char *sender = NULL;
		const char *event_name = NULL;
		g_variant_get (parameters, "(&s&sas)", &sender, &event_name, NULL);

		unsigned int* count = g_hash_table_lookup (event_listeners, sender);
		if (count == NULL) {
			dbg (0, "Registering event listener (%s, %s) on the a11y bus", sender, event_name[0] != 0 ? event_name : "(none)");
			count = g_new (unsigned int, 1);
			*count = 1;
			g_hash_table_insert (event_listeners, g_strdup (sender), count);

		} else if (*count == G_MAXUINT) {
			dbg (0, "Reference count for event listener %s reached saturation", sender);

		} else {
          dbg(0, "Incrementing refcount for event listener %s", sender);
          *count += 1;
        }
    }
#if 0
#endif
}

static void
on_event_listener_deregistered (GDBusConnection *connection, const char *sender_name, const char *object_path, const char *interface_name, const char *signal_name, GVariant *parameters, gpointer user_data)
{
	if (g_strcmp0 (object_path, ATSPI_REGISTRY_PATH) == 0 &&
		g_strcmp0 (interface_name, "org.a11y.atspi.Registry") == 0 &&
		g_strcmp0 (signal_name, "EventListenerDeregistered") == 0)
	{
		const char *sender = NULL;
		const char *event = NULL;
		unsigned int *count;

		g_variant_get (parameters, "(&s&s)", &sender, &event);

		if (G_UNLIKELY (event_listeners == NULL)) {
			dbg (0, "Received org.a11y.atspi.Registry::EventListenerDeregistered for sender (%s, %s) without a corresponding EventListenerRegistered signal.", sender, event[0] != '\0' ? event : "(no event)");
			return;
		}

		count = g_hash_table_lookup (event_listeners, sender);
		if (G_UNLIKELY (count == NULL)) {
			dbg(0, "Received org.a11y.atspi.Registry::EventListenerDeregistered for sender (%s, %s) without a corresponding EventListenerRegistered signal.", sender, event[0] != '\0' ? event : "(no event)");
		} else if (*count > 1) {
			dbg(0, "Decreasing refcount for listener %s", sender);
			*count -= 1;
		} else {
			dbg (0, "Deregistering event listener %s on the a11y bus", sender);
			g_hash_table_remove (event_listeners, sender);
		}
	}
}

static void
on_registered_events_reply (GObject* gobject, GAsyncResult* result, gpointer data)
{
	g_autoptr(GError) error = NULL;
	GVariant* reply = g_dbus_connection_call_finish (G_DBUS_CONNECTION (gobject), result, &error);
	if (error) {
		g_critical ("Unable to get the list of registered event listeners: %s", error->message);
		return;
	}

	GVariant* listeners = g_variant_get_child_value (reply, 0);

	if (!event_listeners)
		event_listeners = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	GVariantIter *iter;
	g_variant_get (listeners, "a(ss)", &iter);
	const char *sender, *event_name;
	while (g_variant_iter_loop (iter, "(&s&s)", &sender, &event_name)) {
		dbg(0, "Registering event listener (%s, %s) on the a11y bus", sender, event_name[0] != 0 ? event_name : "(none)");

		unsigned int* count = g_hash_table_lookup (event_listeners, sender);
		if (!count) {
			count = g_new (unsigned int, 1);
			*count = 1;
			g_hash_table_insert (event_listeners, g_strdup (sender), count);
		} else if (*count == G_MAXUINT) {
			g_critical ("Reference count for event listener %s reached saturation", sender);
        } else
			*count += 1;
    }

	g_variant_iter_free (iter);
	g_variant_unref (listeners);
	g_variant_unref (reply);
}

static void
on_registration_reply (GObject* gobject, GAsyncResult* result, gpointer user_data)
{
	GError* error = NULL;
	GVariant* reply = g_dbus_connection_call_finish (G_DBUS_CONNECTION (gobject), result, &error);

	register_id = 0;

	if (error) {
		g_critical ("Unable to register the application: %s", error->message);
		g_error_free (error);
		return;
	}

	if (reply) {
		g_variant_get (reply, "((so))", &desktop_name, &desktop_path);
		g_variant_unref (reply);

		dbg (0, "Connected to the a11y registry at (%s, %s)", desktop_name, desktop_path);
	}

	/* Subscribe to notifications on the registered event listeners */
	g_dbus_connection_signal_subscribe (connection,
                                      "org.a11y.atspi.Registry",
                                      "org.a11y.atspi.Registry",
                                      "EventListenerRegistered",
                                      ATSPI_REGISTRY_PATH,
                                      NULL,
                                      G_DBUS_SIGNAL_FLAGS_NONE,
                                      on_event_listener_registered,
                                      user_data,
                                      NULL);
	g_dbus_connection_signal_subscribe (connection,
                                      "org.a11y.atspi.Registry",
                                      "org.a11y.atspi.Registry",
                                      "EventListenerDeregistered",
                                      ATSPI_REGISTRY_PATH,
                                      NULL,
                                      G_DBUS_SIGNAL_FLAGS_NONE,
                                      on_event_listener_deregistered,
                                      user_data,
                                      NULL);

	/* Get the list of ATs listening to events, in case they were started
	 * before the application; we want to delay the D-Bus traffic as much
	 * as possible until we know something is listening on the accessibility
	 * bus
	 */
	g_dbus_connection_call (connection,
                          "org.a11y.atspi.Registry",
                          ATSPI_REGISTRY_PATH,
                          "org.a11y.atspi.Registry",
                          "GetRegisteredEvents",
                          g_variant_new ("()"),
                          G_VARIANT_TYPE ("(a(ss))"),
                          G_DBUS_CALL_FLAGS_NONE, -1,
                          NULL,
                          on_registered_events_reply,
                          user_data);
}

static gboolean
root_register (gpointer user_data)
{
	/* Register the root element; every application has a single root, so we only
	 * need to do this once.
	 *
	 * The root element is used to advertise our existence on the accessibility
	 * bus, and it's the entry point to the accessible objects tree.
	 *
	 * The announcement is split into two phases:
	 *
	 *  1. register the org.a11y.atspi.Application and org.a11y.atspi.Accessible
	 *     interfaces at the well-known object path
	 *  2. invoke the org.a11y.atspi.Socket.Embed method with the connection's
	 *     unique name and the object path
	 *  3. the ATSPI registry daemon will set the org.a11y.atspi.Application.Id
	 *     property on the given object path
	 *  4. the registration concludes when the Embed method returns us the desktop
	 *     name and object path
	 */

	const char* unique_name = g_dbus_connection_get_unique_name(connection);

	g_dbus_connection_register_object(connection, ATSPI_ROOT_PATH, (GDBusInterfaceInfo *) &atspi_application_interface, &root_application_vtable, NULL, NULL, NULL);
	g_dbus_connection_register_object(connection, ATSPI_ROOT_PATH, (GDBusInterfaceInfo *) &atspi_accessible_interface, &root_accessible_vtable, NULL, NULL, NULL);

	dbg(0, "Registering (%s, %s) on the a11y bus", unique_name, ATSPI_ROOT_PATH);

	g_dbus_connection_call (connection,
                          "org.a11y.atspi.Registry",
                          ATSPI_ROOT_PATH,
                          "org.a11y.atspi.Socket",
                          "Embed",
                          g_variant_new ("((so))", unique_name, ATSPI_ROOT_PATH),
                          G_VARIANT_TYPE ("((so))"),
                          G_DBUS_CALL_FLAGS_NONE, -1,
                          NULL,
                          on_registration_reply,
                          user_data);

	return G_SOURCE_REMOVE;
}

void
atspi_register (AGlScene* _scene)
{
	scene = _scene;
	bus_address = get_bus_address();

	g_autoptr(GError) error = NULL;
	connection = g_dbus_connection_new_for_address_sync(bus_address, G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION, NULL, NULL, &error);

	if (error) {
		g_critical ("Unable to connect to the accessibility bus at '%s': %s", bus_address, error->message);
		return;
	}

#if 1
	pid_t pid = getpid();
	context_path = g_strdup_printf ("%s/%d", base_path, pid);
#else
	char* uuid = g_uuid_string_random ();
	size_t len = strlen (uuid);
	for (size_t i = 0; i < len; i++) {
		if (uuid[i] == '-') uuid[i] = '_';
	}

	context_path = g_strconcat (base_path, "/", uuid, NULL);

	g_free (uuid);
#endif

	register_id = g_idle_add (root_register, NULL);
}

static void
destructor ()
{
	g_clear_object (&connection);

	g_clear_handle_id (&register_id, g_source_remove);
	g_clear_pointer (&event_listeners, g_hash_table_unref);

	g_free (bus_address);
	g_free (base_path);
	g_free (desktop_name);
	g_free (desktop_path);
}
