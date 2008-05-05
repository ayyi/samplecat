#include "stdint.h"

#define dbg(A, B, ...) debug_printf(__func__, A, B, ##__VA_ARGS__)
#define perr(A, ...) errprintf2(__func__, A, ##__VA_ARGS__)
#define PF printf("%s()...\n", __func__);
#define PF_DONE printf("%s(): done.\n", __func__);
#define ASSERT_POINTER(A, B) if((unsigned)A < 1024){ errprintf2(__func__, "bad %s pointer (%p).\n", B, A); return; }
#define ASSERT_POINTER_FALSE(A, B) if(GPOINTER_TO_UINT(A) < 1024){ errprintf2(__func__, "bad %s pointer (%p).\n", B, A); return FALSE; } 
#define GERR_WARN if(error){ warnprintf("%s\n", error->message); g_error_free(error); error = NULL; }

#define HAS_ALPHA_FALSE 0
#define BITS_PER_CHAR_8 8

#define A_SIZE(A) sizeof(A)/sizeof(A[0])

typedef struct _rect {
  double x1;
  double y1;
  double x2;
  double y2;
} rect;

typedef enum {
	SORT_NONE = 0,
	SORT_NAME,
	SORT_TYPE,
	SORT_DATE,
	SORT_SIZE,
	SORT_OWNER,
	SORT_GROUP,
	SORT_NUMBER,
	SORT_PATH
} SortType;

void         errprintf(char* fmt, ...);
void         errprintf2(const char* func, char* format, ...);
void         warnprintf(char* format, ...);
void         debug_printf(const char* func, int level, const char* format, ...);

void         samplerate_format(char* str, int samplerate);
//gint         strcmp2(gconstpointer a, gconstpointer b);
//GPtrArray*   list_dir(const guchar *path);
gboolean     file_exists(const char *path);
gboolean     is_dir(const char *path);
gboolean     dir_is_empty(const char *path);
void         file_extension(const char* path, char* extn);
gboolean     is_sub_dir(const char *sub_obj, const char *parent);
void         file_move(const char* path, const char* dest);
char*        fork_exec_wait(const char **argv);

//-----------------------------------------------------------------

gchar*       uri_text_from_list(GList*, gint* len, gint plain_text);
GList*       uri_list_from_text(gchar* data, gint files_only);
gchar*       uri_text_escape(const gchar* text);
void         uri_text_decode(gchar* text);

void         pixbuf_clear(GdkPixbuf*, GdkColor*);
#ifdef OLD
void         pixbuf_draw_line(GdkPixbuf *pixbuf, struct _ArtDRect *pts, double line_width, GdkColor *colour);
#else
void         pixbuf_draw_line(cairo_t*, rect *pts, double line_width, GdkColor *colour);
#endif
//GdkPixbuf*   scale_pixbuf(GdkPixbuf *src, int max_w, int max_h);
//GdkPixbuf*   scale_pixbuf_up(GdkPixbuf *src, int max_w, int max_h);
GdkPixbuf*   create_spotlight_pixbuf(GdkPixbuf *src, GdkColor*);

void         colour_get_style_fg(GdkColor *color, int state);
void         colour_get_style_bg(GdkColor *color, int state);
void         colour_get_style_base(GdkColor *color, int state);
void         colour_get_style_text(GdkColor *color, int state);
void         colour_get_float(GdkColor*, float* r, float* g, float* b, const unsigned char alpha);
gchar*       gdkcolor_get_hexstring(GdkColor* c);
void         hexstring_from_gdkcolor(char* hexstring, GdkColor* c);
void         color_rgba_to_gdk(GdkColor* colour, uint32_t rgba);
gboolean     colour_lighter(GdkColor* lighter, GdkColor* colour);
gboolean     colour_darker(GdkColor* lighter, GdkColor* colour);
gboolean     is_black(GdkColor* colour);
gboolean     is_white(GdkColor* colour);
gboolean     is_similar(GdkColor* colour1, GdkColor* colour2, char min_diff);
gboolean     is_similar_rgb(unsigned colour1, unsigned colour2);

void         format_time(char* length, char* milliseconds);
void         format_time_int(char* length, int milliseconds);

gint         treecell_get_row(GtkWidget *widget, GdkRectangle *cell_area);
void         statusbar_print(int n, char *s);
void         statusbar_printf(int n, char* fmt, ...);

GList*       uri_list_to_glist(const char *uri_list);
void         uri_list_free(GList*);
const gchar* vfs_get_method_string(const gchar *substring, gchar **method_string);
char*        vfs_unescape_string (const gchar *escaped_string, const gchar *illegal_characters);

