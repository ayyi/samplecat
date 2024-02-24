#ifndef __ayyi_debug_h__
#define __ayyi_debug_h__

#include "glib.h"

#ifdef DEBUG
// note that the level check is now outside the print fn in order to
// prevent functions that return print arguments from being run
#  define dbg(A, B, ...) do {if(A <= _debug_) debug_printf(__func__, A, B, ##__VA_ARGS__);} while(FALSE)
#  define PF0 {dbg(0, "...");}
#  define PF {if (_debug_) PF0}
#  define PF2 {if (_debug_ > 1) PF0;}
#else
#  define dbg(A, B, ...)
#  define PF0
#  define PF
#  define PF2
#endif
#define gerr(A, ...) g_critical("%s(): "A, __func__, ##__VA_ARGS__)
#define perr(A, ...) errprintf2(__func__, A"\n", ##__VA_ARGS__)
#define pwarn(A, ...) g_warning("%s(): "A, __func__, ##__VA_ARGS__)
#define PF_DONE printf("%s(): done.\n", __func__);
#define P_GERR if (error){ gerr("%s\n", error->message); g_error_free(error); error = NULL; }
#define GERR_INFO if (error){ printf("%s\n", error->message); g_error_free(error); error = NULL; }
#define GERR_WARN if (error){ pwarn("%s", error->message); g_error_free(error); error = NULL; }

void        debug_printf             (const char* func, int level, const char* format, ...) __attribute__ ((format (printf, 3, 4)));
void        warnprintf               (const char* format, ...);
void        warnprintf2              (const char* func, char* format, ...);
void        errprintf                (char* fmt, ...);
void        errprintf2               (const char* func, char* format, ...);

void        underline                ();

void        log_handler              (const gchar* log_domain, GLogLevelFlags, const gchar* message, gpointer);

extern int _debug_;                  // debug level. 0=off.

extern char ayyi_warn[32];
extern char ayyi_err [32];

extern char ayyi_white [12];
extern char ayyi_red   [10];
extern char ayyi_blue  [10];
extern char ayyi_green [10];
extern char ayyi_grey  [12];
extern char ayyi_bold  [12];
extern char ayyi_warn  [32];
extern char ayyi_err   [32];

#endif
