#define __debug_c__
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <glib.h>
#include "debug.h"


int _debug_ = 0;


void
debug_printf(const char* func, int level, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	if (level <= _debug_) {
		fprintf(stderr, "%s(): ", func);
		vfprintf(stderr, format, args);
		fprintf(stderr, "\n");
	}
	va_end(args);
}


void 
warnprintf(char* format, ...)
{
  // print a warning string, then pass arguments on to vprintf.

  printf("%s ", ayyi_warn);

  va_list argp;           //points to each unnamed arg in turn
  va_start(argp, format); //make arg pointer point to 1st unnamed arg
  vprintf(format, argp);
  va_end(argp); 		  //clean up
}


void 
warnprintf2(const char* func, char* format, ...)
{
	// print a warning string, then pass arguments on to vprintf.

	printf("%s %s(): ", ayyi_warn, func);

	va_list argp;
	va_start(argp, format);
	vprintf(format, argp);
	va_end(argp);
}


void
errprintf(char* format, ...)
{
	// print an error string, then pass arguments on to vprintf.

	printf("%s ", ayyi_err);

	va_list argp;           //points to each unnamed arg in turn
	va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
	vprintf(format, argp);
	va_end(argp);           //clean up
}


void
errprintf2(const char* func, char* format, ...)
{
	// print an error string, then pass arguments on to vprintf.

	printf("%s %s(): ", ayyi_err, func);

	va_list argp;           //points to each unnamed arg in turn
	va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
	vprintf(format, argp);
	va_end(argp);           //clean up
}


void
log_handler(const gchar* log_domain, GLogLevelFlags log_level, const gchar* message, gpointer user_data)
{
	switch(log_level){
		case G_LOG_LEVEL_CRITICAL:
			printf("%s %s\n", ayyi_err, message);
			break;
		case G_LOG_LEVEL_WARNING:
			printf("%s %s\n", ayyi_warn, message);
			break;
		default:
			if(_debug_) printf("log_handler(): level=%i %s\n", log_level, message);
			break;
	}
}

