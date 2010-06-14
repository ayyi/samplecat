#include "stdint.h"

#define dbg(A, B, ...) debug_printf(__func__, A, B, ##__VA_ARGS__)
#define perr(A, ...) errprintf2(__func__, A, ##__VA_ARGS__)
#define pwarn(A, ...) warnprintf2(__func__, A, ##__VA_ARGS__)
#ifndef __ayyi_h__
#define PF {if(debug) printf("%s()...\n", __func__);}
#define PF_DONE printf("%s(): done.\n", __func__);
#define ASSERT_POINTER(A, B) if((unsigned)A < 1024){ errprintf2(__func__, "bad %s pointer (%p).\n", B, A); return; }
#define gwarn(A, ...) g_warning("%s(): "A, __func__, ##__VA_ARGS__);
#define ASSERT_POINTER_FALSE(A, B) if(GPOINTER_TO_UINT(A) < 1024){ errprintf2(__func__, "bad %s pointer (%p).\n", B, A); return FALSE; } 
#define GERR_WARN if(error){ gwarn("%s", error->message); g_error_free(error); error = NULL; }
#define P_GERR if(error){ errprintf2(__func__, "%s\n", error->message); g_error_free(error); error = NULL; }
#endif
#define g_error_clear(E) { if(E){ g_error_free(E); E = NULL; }}
#define list_clear(L) g_list_free(L); L = NULL;

#define HAS_ALPHA_FALSE 0
#define HAS_ALPHA_TRUE 1
#define BITS_PER_CHAR_8 8
#define IDLE_STOP FALSE
#define TIMER_CONTINUE TRUE
#define HANDLED TRUE
#define NOT_HANDLED FALSE

#ifndef false
  #define false FALSE
#endif
#ifndef true
  #define true TRUE
#endif

#define A_SIZE(A) sizeof(A)/sizeof(A[0])

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

void         errprintf                 (char* fmt, ...);
void         errprintf2                (const char* func, char* format, ...);
void         warnprintf                (char* format, ...);
void         warnprintf2               (const char* func, char *format, ...);
void         debug_printf              (const char* func, int level, const char* format, ...);
void         log_handler               (const gchar* log_domain, GLogLevelFlags, const gchar* message, gpointer);

void         samplerate_format         (char*, int samplerate);
gchar*       dir_format                (char*);
gchar*       channels_format           (int);
//gint         strcmp2(gconstpointer a, gconstpointer b);
//GPtrArray*   list_dir(const guchar *path);
gboolean     file_exists               (const char*);
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

void         format_time               (char* length, const char* milliseconds);
void         format_time_int           (char* length, int milliseconds);

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

GList*       uri_list_to_glist         (const char *uri_list);
void         uri_list_free             (GList*);
const gchar* vfs_get_method_string     (const gchar *substring, gchar **method_string);
char*        vfs_unescape_string       (const gchar *escaped_string, const gchar *illegal_characters);

float        gain2db                   (float);
char*        gain2dbstring             (float);

