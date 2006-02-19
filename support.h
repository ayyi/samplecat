
void         errprintf(char *fmt, ...);
void         warnprintf(char *format, ...);

void         samplerate_format(char* str, int samplerate);
gint         strcmp2(gconstpointer a, gconstpointer b);
GPtrArray*   list_dir(const guchar *path);
gboolean     file_exists(const char *path);

//-----------------------------------------------------------------

//gnome vfs:

#ifdef NEVER
// a URI element.
typedef struct GnomeVFSURI {
    /* Reference count.  */
    guint ref_count;

    /* Text for the element: eg. some/path/name.  */
    gchar *text;

    /* Text for uri fragment: eg, #anchor  */
    gchar *fragment_id;

    /* Method string: eg. `gzip', `tar', `http'.  This is necessary as
       one GnomeVFSMethod can be used for different method strings
       (e.g. extfs handles zip, rar, zoo and several other ones).  */
    gchar *method_string;

    /* VFS method to access the element.  */
    struct GnomeVFSMethod *method;

    /* Pointer to the parent element, or NULL for toplevel elements.  */
    struct GnomeVFSURI *parent;

    /* Reserved to avoid future breaks in ABI compatibility */
    void *reserved1;
    void *reserved2;
} GnomeVFSURI;


GList*       vfs_uri_list_parse(const gchar* uri_list);
void         gnome_vfs_uri_list_free(GList *list);
GList*       gnome_vfs_uri_list_unref(GList *list);
void         gnome_vfs_uri_unref(GnomeVFSURI *uri);
static void  destroy_element(GnomeVFSURI *uri);
GnomeVFSToplevelURI *gnome_vfs_uri_get_toplevel(const GnomeVFSURI *uri);
const gchar* vfs_get_method_string(const gchar *substring, gchar **method_string);
#endif

void         pixbuf_clear(GdkPixbuf *pixbuf, GdkColor *colour);
void         pixbuf_draw_line(GdkPixbuf *pixbuf, struct _ArtDRect *pts, double line_width, GdkColor *colour);
GdkPixbuf*   scale_pixbuf(GdkPixbuf *src, int max_w, int max_h);
GdkPixbuf*   scale_pixbuf_up(GdkPixbuf *src, int max_w, int max_h);

void         colour_get_style_fg(GdkColor *color, int state);
void         colour_get_style_bg(GdkColor *color, int state);

void         format_time(char* length, char* milliseconds);
void         format_time_int(char* length, int milliseconds);

