#ifndef __SUPPORT_H_
#define __SUPPORT_H_

#include <stdint.h>
#include <gtk/gtk.h>
#include "typedefs.h"
#include "utils/ayyi_utils.h"

#define dbg(A, B, ...) debug_printf(__func__, A, B, ##__VA_ARGS__)
#define perr(A, ...) errprintf2(__func__, A, ##__VA_ARGS__)
#define pwarn(A, ...) warnprintf2(__func__, A, ##__VA_ARGS__)
#ifndef __ayyi_h__
#define PF {if(debug) printf("%s()...\n", __func__);}
#define PF_DONE printf("%s(): done.\n", __func__);
#define gwarn(A, ...) g_warning("%s(): "A, __func__, ##__VA_ARGS__);
#define GERR_WARN if(error){ gwarn("%s", error->message); g_error_free(error); error = NULL; }
#endif
#define g_error_clear(E) { if(E){ g_error_free(E); E = NULL; }}
#define list_clear(L) g_list_free(L); L = NULL;
#define call(FN, A, ...) if(FN) (FN)(A, ##__VA_ARGS__)

#define HAS_ALPHA_FALSE 0
#define HAS_ALPHA_TRUE 1
#define IDLE_STOP FALSE
#define TIMER_CONTINUE TRUE
#define HANDLED TRUE
#define NOT_HANDLED FALSE

typedef struct _rect {
  double x1;
  double y1;
  double x2;
  double y2;
} drect;

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

void         log_handler               (const gchar* log_domain, GLogLevelFlags, const gchar* message, gpointer);
void         p_                        (int level, const char* format, ...);

void         samplerate_format         (char*, int samplerate);
void         bitrate_format            (char* str, int bitdepth);
void         bitdepth_format           (char* str, int bitdepth);
gchar*       dir_format                (char*);
gchar*       channels_format           (int);
void         smpte_format              (char*, int64_t);
//gint         strcmp2(gconstpointer a, gconstpointer b);
//GPtrArray*   list_dir(const guchar *path);
gboolean     file_exists               (const char*);
time_t       file_mtime                (const char *path);
gboolean     is_dir                    (const char*);
gboolean     dir_is_empty              (const char*);
void         file_extension            (const char*, char* extn);
gboolean     ensure_config_dir         ();

//-----------------------------------------------------------------

void         pixbuf_clear              (GdkPixbuf*, GdkColor*);
#ifdef OLD
void         pixbuf_draw_line          (GdkPixbuf*, struct _ArtDRect *pts, double line_width, GdkColor *colour);
#else
void         draw_cairo_line           (cairo_t*, drect*, double line_width, GdkColor *colour);
#endif
//GdkPixbuf*   scale_pixbuf(GdkPixbuf *src, int max_w, int max_h);
//GdkPixbuf*   scale_pixbuf_up(GdkPixbuf *src, int max_w, int max_h);
//GdkPixbuf*   create_spotlight_pixbuf   (GdkPixbuf *src, GdkColor*);

void         colour_get_style_fg       (GdkColor*, int state);
void         colour_get_style_bg       (GdkColor*, int state);
void         colour_get_style_base     (GdkColor*, int state);
void         colour_get_style_text     (GdkColor*, int state);
void         colour_get_float          (GdkColor*, float* r, float* g, float* b, const unsigned char alpha);
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

GList*       uri_list_to_glist         (const char* uri_list);
void         uri_list_free             (GList*);
const gchar* vfs_get_method_string     (const gchar* substring, gchar** method_string);
char*        vfs_unescape_string       (const gchar* escaped_string, const gchar* illegal_characters);

float        gain2db                   (float);
char*        gain2dbstring             (float);

void         show_widget_if            (GtkWidget*, gboolean);

GdkPixbuf*   get_iconbuf_from_mimetype (char* mimetype);

uint8_t*     pixbuf_to_blob            (GdkPixbuf* in, guint* len);
#endif
