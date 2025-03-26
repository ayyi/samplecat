#include <stdio.h>
#include <stdarg.h>
#include "gdl-dock-object.h"
#include "gdl-dock-notebook.h"
#include "gdl-dock.h"
#include "debug.h"
#include "gtk/utils.h"
#include "utils.h"

#ifdef DEBUG

int gdl_debug = 0;
int indent = 0;

void
gdl_debug_printf (const char* func, int level, const char* format, ...)
{
	va_list args;

	va_start(args, format);
	if (level <= gdl_debug) {
		fprintf(stderr, "%s(): ", func);
		vfprintf(stderr, format, args);
		fprintf(stderr, "\n");
	}
	va_end(args);
}


void
gdl_debug_printf_colour (const char* func, int level, const char* colour, const char* format, ...)
{
	va_list args;

	va_start(args, format);
	if (level <= gdl_debug) {
		fprintf(stderr, "%s%s():%s ", colour, func, ayyi_white);
		vfprintf(stderr, format, args);
		fprintf(stderr, "\n");
	}
	va_end(args);
}


void
leave (int* i)
{
	indent--;
	indent = MAX(indent, 0);
}


static int rec_depth = 0;

static void
gdl_dock_foreach_object_print (GdlDockObject *object, gpointer user_data)
{
	GHashTable* found = user_data;

	char f[96];
	sprintf(f, "  %%%is %%i %%s <%%s> %%i x %%i (%%i) %%s\n", rec_depth);

	if (!object) {
		printf(f, "", rec_depth, "NULL", "", 0, 0, "");
		return;
	}

	g_hash_table_insert(found, object, object);

	const char* class_name = G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(object));

	g_autofree char* name = NULL;
	g_object_get (object, "name", &name, NULL);

	char attributes[128] = {0,};
	int ai = 0;
	guint n_props;
    GParamSpec** props = g_object_class_list_properties (G_OBJECT_GET_CLASS (object), &n_props);
    GValue attr = { 0, };
    g_value_init (&attr, GDL_TYPE_DOCK_PARAM);
    for (int i = 0; i < n_props; i++) {
        GParamSpec* p = props [i];
		if (!strcmp(p->name, "name")) continue;

        if (p->flags & GDL_DOCK_PARAM_EXPORT) {
            GValue v = { 0, };
            g_value_init (&v, p->value_type);
            g_object_get_property (G_OBJECT (object), p->name, &v);

            if (strcmp (p->name, GDL_DOCK_NAME_PROPERTY) || g_value_get_string (&v)) {
				if (!g_param_value_defaults (p, &v)) {
					if (g_value_transform (&v, &attr)) {
						const char* value = g_value_get_string (&attr);
						int len = strlen(p->name) + strlen(value) + 3;
						snprintf(&attributes[ai], len, " %s=%s", p->name, value);
						ai += len -1;
					}
				}
            }

            /* free the parameter value */
            g_value_unset (&v);
		}
	}
    g_value_unset (&attr);

	printf(f, "", rec_depth, name ? name : "", class_name, gtk_widget_get_width(GTK_WIDGET(object)), gtk_widget_get_height(GTK_WIDGET(object)), G_OBJECT(object)->ref_count, attributes);

	g_return_if_fail (object != NULL && GDL_IS_DOCK_OBJECT (object));

	rec_depth++;
	if (gdl_dock_object_is_compound (object)) {
		gdl_dock_object_foreach_child (object, gdl_dock_foreach_object_print, found);
	}
	rec_depth--;
}

void
gdl_dock_print (GdlDockMaster *master)
{
	cdbg(0, "...");
	GHashTable* found = g_hash_table_new(g_direct_hash, g_direct_equal);

	gdl_dock_master_foreach_toplevel (master, TRUE, (GFunc) gdl_dock_foreach_object_print, found);
	int n = g_hash_table_size(found);

	void find (GtkWidget* widget, GtkWidget** result)
	{
		GtkWidget* child = gtk_widget_get_first_child (widget);
		for (; child; child = gtk_widget_get_next_sibling (child)) {
			if (GDL_IS_DOCK_OBJECT(child)) {
				if (!g_hash_table_lookup(found, child))
					cdbg(0, "--out");
			}
			find(child, result);
		}
	}

	find (GTK_WIDGET(gdl_dock_master_get_controller(master)), NULL);
	if (g_hash_table_size(found) != n) cdbg(0, "orphans=%i", n);

	g_autoptr(GList) named_items = gdl_dock_get_named_items (GDL_DOCK(gdl_dock_master_get_controller(master)));
	cdbg(0, "named=%i", g_list_length(named_items));
	for (GList* l = named_items; l; l=l->next) {
		GdlDockItem* item = l->data;
		if (!g_hash_table_lookup(found, item)) {
			gdl_dock_foreach_object_print (GDL_DOCK_OBJECT(item), found);
		}
	}

	g_hash_table_destroy(found);
}


const char*
gdl_dock_object_id (GdlDockObject* object)
{
	if (!object) return NULL;

	static char* str;

	const char* class_name = G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(object));
	char* name = NULL;
	g_object_get (object, "name", &name, NULL);
    gchar* title = NULL;
    g_object_get (object, "long-name", &title, NULL);
	if (name && title && !strcmp(name, title))
		title = NULL;
	set_str(str, g_strdup_printf("%s%s%s\x1b[38;5;156m%s%s%s%s%s (%i)", ayyi_bold, class_name, ayyi_white, name ? " " : "", name ? name : "", title ? " " : "", title ? title : "", ayyi_white, G_OBJECT(object)->ref_count));

	return str;
}

#endif
