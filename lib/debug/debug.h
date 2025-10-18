/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2013-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#ifndef __ayyi_debug_h__
#define __ayyi_debug_h__

#include "glib.h"

#ifdef DEBUG
// note that the level check is now outside the print fn in order to
// prevent functions that return print arguments from being run
#  define dbg(A, B, ...) do {if(A <= _debug_) debug_printf(__func__, A, B, ##__VA_ARGS__);} while(FALSE)
#else
#  define dbg(A, B, ...)
#endif
#define gerr(A, ...) g_critical("%s(): "A, __func__, ##__VA_ARGS__)
#define perr(A, ...) errprintf2(__func__, A"\n", ##__VA_ARGS__)
#define gwarn(A, ...) g_warning("%s(): "A, __func__, ##__VA_ARGS__);
#define pwarn(A, ...) g_warning("%s(): "A, __func__, ##__VA_ARGS__)
#define PF0 {printf("%s()...\n", __func__);}
#define PF {if (_debug_) printf("%s()...\n", __func__);}
#define PF2 {if (_debug_ > 1) printf("%s()...\n", __func__);}
#define PF_DONE printf("%s(): done.\n", __func__);
#define P_GERR if (error){ gerr("%s\n", error->message); g_error_free(error); error = NULL; }
#define GERR_INFO if (error){ printf("%s\n", error->message); g_error_free(error); error = NULL; }
#define GERR_WARN if (error){ gwarn("%s", error->message); g_error_free(error); error = NULL; }

void debug_printf (const char* func, int level, const char* format, ...) __attribute__ ((format (printf, 3, 4)));
void warnprintf   (const char* format, ...) __attribute__ ((format (printf, 1, 2)));
void warnprintf2  (const char* func, char* format, ...);
void errprintf    (const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
void errprintf2   (const char* func, char* format, ...);

void log_handler  (const gchar* log_domain, GLogLevelFlags, const gchar* message, gpointer);

extern int _debug_;                  // debug level. 0=off.

extern char ayyi_warn[32];
extern char ayyi_err [32];

#endif
