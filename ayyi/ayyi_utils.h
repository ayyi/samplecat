#ifndef __ayyi_utils_h__
#define __ayyi_utils_h__

#include "glib.h"
#include "ayyi/ayyi_typedefs.h"

#define dbg(A, B, ...) debug_printf(__func__, A, B, ##__VA_ARGS__)
#define gerr(A, ...) g_critical("%s(): "A, __func__, ##__VA_ARGS__)
#define gwarn(A, ...) g_warning("%s(): "A, __func__, ##__VA_ARGS__);
#define PF printf("%s()...\n", __func__);
#define PF_DONE printf("%s(): done.\n", __func__);
#define ASSERT_POINTER(A, B) if((unsigned)A < 1024){ errprintf2(__func__, "bad %s pointer (%p).\n", B, A); return; } 
#define ASSERT_POINTER_FALSE(A, B) if(GPOINTER_TO_UINT(A) < 1024){ g_critical("%s(): bad %s pointer (%p).", __func__, B, A); return FALSE; } 
#define ASSERT_POINTER_VAL(A, B, C) if(GPOINTER_TO_UINT(A) < 1024){ errprintf2(__func__, "bad %s pointer (%p).\n", B, A); return C; } 
#define IS_PTR(A) ((guint)A > 1024)
#define P_GERR if(error){ gerr("%s\n", error->message); g_error_free(error); error = NULL; }
#define GERR_INFO if(error){ printf("%s\n", error->message); g_error_free(error); error = NULL; }
#define GERR_WARN if(error){ gwarn("%s", error->message); g_error_free(error); error = NULL; }
#define UNDERLINE printf("-----------------------------------------------------\n")

void        debug_printf             (const char* func, int level, const char *format, ...);
void        errprintf                (char *fmt, ...);
void        errprintf2               (const char* func, char *format, ...);
void        errprintf3               (const char* func, char *format, ...);
void        warnprintf2              (const char* func, char *format, ...);
void        warn_gerror              (const char* msg, GError** error);
void        info_gerror              (const char* msg, GError** error);

gchar*      path_from_utf8           (const gchar* utf8);

GList*      get_dirlist              (const char*);

void        string_increment_suffix  (char* newstr, const char* orig, int new_max);

int         get_terminal_width       ();

gboolean    audio_path_get_leaf      (const char* path, char* leaf);

#ifdef __ayyi_utils_c__
char white    [16] = "\x1b[0;39m";
char bold     [16] = "\x1b[1;39m";
char yellow   [16] = "\x1b[1;33m";
char yellow_r [16] = "\x1b[30;44m";
char blue_r   [16] = "\x1b[30;44m";
char red      [16] = "\x1b[1;31m";
char green    [16] = "\x1b[1;32m";
char green_r  [16] = "\x1b[30;42m";
char ayyi_warn[32] = "\x1b[1;33mwarning:\x1b[0;39m";
char ayyi_err [32] = "\x1b[1;31merror!\x1b[0;39m";
char go_rhs   [32] = "\x1b[A\x1b[50C"; //go up one line, then goto column 60
char ok       [32] = " [ \x1b[1;32mok\x1b[0;39m ]";
char fail     [32] = " [\x1b[1;31mFAIL\x1b[0;39m]";
#else
extern char white    [16];
extern char bold     [16];
extern char yellow   [16];
extern char yellow_r [16];
extern char blue_r   [16];
extern char red      [16];
extern char green    [16];
extern char green_r  [16];
extern char ayyi_warn[32];
extern char ayyi_err [32];
extern char go_rhs   [32];
extern char ok       [];
extern char fail     [];
#endif

#endif //__ayyi_utils_h__
