/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2013-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#define __debug_c__
#undef TIMING

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <glib.h>
#include "debug.h"

int _debug_ = 0;

char ayyi_bold  [12] = "\x1b[1;39m";
char ayyi_white [12] = "\x1b[0;39m";
char ayyi_red   [10] = "\x1b[1;31m";
char ayyi_blue  [10] = "\x1b[1;34m";
char ayyi_green [10] = "\x1b[1;32m";
char ayyi_grey  [12] = "\x1b[0;90m";
char ayyi_warn  [32] = "\x1b[1;33mwarning:\x1b[0;39m";
char ayyi_err   [32] = "\x1b[1;31merror\x1b[0;39m";

#ifdef TIMING
double ref_time;
#endif

void debug_printf (const char* func, int level, const char* format, ...) __attribute__ ((format (printf, 3, 4)));


void
debug_printf (const char* func, int level, const char* format, ...)
{
#ifdef TIMING
	struct timeval now;
	gettimeofday(&now, 0);

	double t = (now.tv_sec + now.tv_usec * 0.000001) * 1000;
	if (!ref_time) ref_time = t;
#endif

	va_list args;
	va_start(args, format);
	if (level <= _debug_) {
#ifdef TIMING
		fprintf(stderr, "%6.1f %s(): ", t - ref_time, func);
#else
		fprintf(stderr, "%s(): ", func);
#endif
		vfprintf(stderr, format, args);
		fprintf(stderr, "\n");
	}
	va_end(args);
}


/*
 *  Print a warning string, then pass arguments on to vprintf.
 */
void 
warnprintf (const char* format, ...)
{
  // print a warning string, then pass arguments on to vprintf.

  printf("%s ", ayyi_warn);

  va_list argp;           //points to each unnamed arg in turn
  va_start(argp, format); //make arg pointer point to 1st unnamed arg
  vprintf(format, argp);
  va_end(argp); 		  //clean up
}


void 
warnprintf2 (const char* func, char* format, ...)
{
	// print a warning string, then pass arguments on to vprintf.

	printf("%s %s(): ", ayyi_warn, func);

	va_list argp;
	va_start(argp, format);
	vprintf(format, argp);
	va_end(argp);
}


void
errprintf (char* format, ...)
{
	// print an error string, then pass arguments on to vprintf.

	printf("%s ", ayyi_err);

	va_list argp;           //points to each unnamed arg in turn
	va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
	vprintf(format, argp);
	va_end(argp);           //clean up
}


void
errprintf2 (const char* func, char* format, ...)
{
	// print an error string, then pass arguments on to vprintf.

	printf("%s %s(): ", ayyi_err, func);

	va_list argp;           //points to each unnamed arg in turn
	va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
	vprintf(format, argp);
	va_end(argp);           //clean up
}


void
log_handler (const gchar* log_domain, GLogLevelFlags log_level, const gchar* message, gpointer user_data)
{
	switch (log_level) {
		case G_LOG_LEVEL_CRITICAL:
			printf("%s %s\n", ayyi_err, message);
			break;
		case G_LOG_LEVEL_WARNING:
			printf("%s %s\n", ayyi_warn, message);
			break;
		case G_LOG_LEVEL_DEBUG:
			break;
		default:
			if (_debug_) printf("log_handler(): level=%i %s\n", log_level, message);
			break;
	}
}


void
underline ()
{
	char f[128] = {0,};
	char block1[] = { 0xe2, 0x94, 0x80, '\0' };
	for (int i=0;i<125;i+=3) memcpy(&f[i], block1, 3);
	puts(f);
}
