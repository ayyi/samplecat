#include "stdint.h"

#if 0
#define HAS_ALPHA_FALSE 0

typedef struct _rect {
  double x1;
  double y1;
  double x2;
  double y2;
} rect;

void         errprintf(char* fmt, ...);
void         errprintf2(const char* func, char* format, ...);
void         warnprintf(char* format, ...);
void         debug_printf(const char* func, int level, const char* format, ...);

void         samplerate_format(char* str, int samplerate);
gint         strcmp2(gconstpointer a, gconstpointer b);
GPtrArray*   list_dir(const guchar *path);
gboolean     file_exists(const char *path);
gboolean     is_dir(const char *path);
gboolean     dir_is_empty(const char *path);
void         file_extension(const char* path, char* extn);
#endif
gboolean     is_sub_dir(const char *sub_obj, const char *parent);
guchar*      file_copy(const guchar* from, const guchar* to);
void         file_move(const char* path, const char* dest);
char*        fork_exec_wait(const char** argv);

//-----------------------------------------------------------------

gchar*       uri_text_from_list(GList*, gint* len, gint plain_text);
GList*       uri_list_from_text(gchar* data, gint files_only);
gchar*       uri_text_escape(const gchar* text);
void         uri_text_decode(gchar* text);

#if 0
void         pixbuf_clear(GdkPixbuf*, GdkColor*);
#ifdef OLD
void         pixbuf_draw_line(GdkPixbuf *pixbuf, struct _ArtDRect *pts, double line_width, GdkColor *colour);
#else
void         pixbuf_draw_line(cairo_t*, rect *pts, double line_width, GdkColor *colour);
#endif
GdkPixbuf*   scale_pixbuf(GdkPixbuf *src, int max_w, int max_h);
GdkPixbuf*   scale_pixbuf_up(GdkPixbuf *src, int max_w, int max_h);
#endif
GdkPixbuf*   create_spotlight_pixbuf(GdkPixbuf *src, GdkColor*);

#if 0
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
#endif //0

gchar* path_to_utf8(const gchar *path);

void   fm__escape_for_menu (char*);

