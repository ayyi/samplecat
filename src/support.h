#ifndef __support_h__
#define __support_h__

#include <stdint.h>
#include <gtk/gtk.h>
#include "typedefs.h"
#include "utils/ayyi_utils.h"
#include "utils/mime_type.h"
#include "samplecat/support.h"

#ifndef __ayyi_h__
#define g_error_free0(E) (g_error_free(E), E = NULL)
#define PF_DONE printf("%s(): done.\n", __func__);
#endif
#define list_clear(L) g_list_free(L); L = NULL;
#define call(FN, A, ...) if(FN) (FN)(A, ##__VA_ARGS__)
#ifndef g_free0
#define g_free0(A) (A = (g_free(A), NULL))
#endif
#define g_list_free0(var) ((var == NULL) ? NULL : (var = (g_list_free (var), NULL)))

#define TIMER_CONTINUE TRUE
#define TIMER_STOP FALSE
#define HANDLED TRUE
#define NOT_HANDLED FALSE
#define EXPAND_TRUE 1
#define EXPAND_FALSE 0
#define FILL_TRUE 1
#define FILL_FALSE 0

struct _accel {
	char          name[16];
	GtkStockItem* stock_item;
	struct s_key {
		int        code;
		int        mask;
	}             key[2];
	gpointer      callback;
	gpointer      user_data;
};

//gint         strcmp2(gconstpointer a, gconstpointer b);
//GPtrArray*   list_dir(const guchar *path);

//-----------------------------------------------------------------

void         pixbuf_clear              (GdkPixbuf*, GdkColor*);
#ifdef OLD
void         pixbuf_draw_line          (GdkPixbuf*, struct _ArtDRect *pts, double line_width, GdkColor *colour);
#else
void         draw_cairo_line           (cairo_t*, DRect*, double line_width, GdkColor *colour);
#endif
//GdkPixbuf*   scale_pixbuf(GdkPixbuf *src, int max_w, int max_h);
//GdkPixbuf*   scale_pixbuf_up(GdkPixbuf *src, int max_w, int max_h);
//GdkPixbuf*   create_spotlight_pixbuf   (GdkPixbuf *src, GdkColor*);

void         colour_get_style_fg       (GdkColor*, GtkStateType);
void         colour_get_style_bg       (GdkColor*, int state);
void         colour_get_style_base     (GdkColor*, int state);
void         colour_get_style_text     (GdkColor*, int state);
gchar*       gdkcolor_get_hexstring    (GdkColor*);
void         hexstring_from_gdkcolor   (char* hexstring, GdkColor*);
void         color_rgba_to_gdk         (GdkColor*, uint32_t rgba);
gboolean     colour_lighter            (GdkColor* lighter, GdkColor*);
gboolean     colour_darker             (GdkColor* lighter, GdkColor*);
gboolean     is_black                  (GdkColor*);
gboolean     is_white                  (GdkColor*);
gboolean     is_dark                   (GdkColor*);
gboolean     is_similar                (GdkColor* colour1, GdkColor* colour2, int min_diff);
gboolean     is_similar_rgb            (unsigned colour1, unsigned colour2);

char*        str_array_join            (const char**, const char*);

gint         treecell_get_row          (GtkWidget*, GdkRectangle*);
void         statusbar_print           (int n, char* fmt, ...);

void         make_accels               (GtkAccelGroup*, GimpActionGroup*, struct _accel*, int count, gpointer user_data);

void         add_menu_items_from_defn  (GtkWidget* menu, MenuDef*, int);

const gchar* gimp_get_mod_name_shift   ();
const gchar* gimp_get_mod_name_control ();
const gchar* gimp_get_mod_name_alt     ();
const gchar* gimp_get_mod_separator    ();
const gchar* gimp_get_mod_string       (GdkModifierType modifiers);
gchar*       gimp_strip_uline          (const gchar* str);
gchar*       gimp_get_accel_string     (guint key, GdkModifierType modifiers);

gchar*       str_replace               (const gchar* string, const gchar* search, const gchar* replace);
char*        remove_trailing_slash     (char* path);

GList*       uri_list_to_glist         (const char* uri_list);
void         uri_list_free             (GList*);
const gchar* vfs_get_method_string     (const gchar* substring, gchar** method_string);
char*        vfs_unescape_string       (const gchar* escaped_string, const gchar* illegal_characters);

float        gain2db                   (float);
char*        gain2dbstring             (float);

void         show_widget_if            (GtkWidget*, gboolean);
GtkWidget*   scrolled_window_new       ();

gboolean     keyword_is_dupe           (const char* keyword, const char* existing);

typedef void (*ObjectCallback)         (GObject*, gpointer);

typedef struct {
   guint          id;
   GObject*       object;
   ObjectCallback fn;
   gpointer       user_data;
   ObjectCallback run;
} Idle;

Idle*        idle_new                  (ObjectCallback, gpointer);
void         idle_free                 (Idle*);

#if 0
void         print_widget_tree         (GtkWidget*);
#endif

#endif
