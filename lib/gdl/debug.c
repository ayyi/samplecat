#include <stdio.h>
#include <stdarg.h>
#include "gdl-dock-object.h"
#include "gdl-dock-notebook.h"
#include "debug.h"
#include "gtk/utils.h"

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


static int rec_depth = 0;

static void
gdl_dock_layout_foreach_object_print (GdlDockObject *object, gpointer user_data)
{
	char f[96];
	sprintf(f, "  %%%is %%i %%s <%%s> %%i x %%i %%s\n", rec_depth);

	if (!object) {
		printf(f, "", rec_depth, "NULL", "", 0, 0, "");
		return;
	}

	const char* class_name = G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(object));

	char* name = NULL;
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

            /* only save the object "name" if it is set (i.e. don't save the empty string) */
            if (strcmp (p->name, GDL_DOCK_NAME_PROPERTY) || g_value_get_string (&v)) {
                if (g_value_transform (&v, &attr)) {
					const char* value = g_value_get_string (&attr);
					int len = strlen(p->name) + strlen(value) + 3;
					snprintf(&attributes[ai], len, " %s=%s", p->name, value);
					//cdbg(0, "  %s=%s (%s) %i", p->name, value, attributes, ai);
					ai += len -1;
				}
            }

            /* free the parameter value */
            g_value_unset (&v);
		}
	}

	printf(f, "", rec_depth, name ? name : "", class_name, gtk_widget_get_allocated_width(GTK_WIDGET(object)), gtk_widget_get_allocated_height(GTK_WIDGET(object)), attributes);

	g_return_if_fail (object != NULL && GDL_IS_DOCK_OBJECT (object));

	rec_depth++;
	if (gdl_dock_object_is_compound (object)) {
		gdl_dock_object_foreach_child (object, gdl_dock_layout_foreach_object_print, NULL);
	}
	rec_depth--;

	if (name) g_free(name);
}

void
gdl_dock_print (GdlDockMaster *master)
{
	cdbg(0, "...");
	gdl_dock_master_foreach_toplevel (master, TRUE, (GFunc) gdl_dock_layout_foreach_object_print, NULL);
}

#endif
