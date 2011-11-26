#define __ayyi_utils_c__
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <glib.h>

#include "ayyi_utils.h"

extern unsigned debug;


void
debug_printf(const char* func, int level, const char* format, ...)
{
    va_list args;

    va_start(args, format);
    if (level <= debug) {
        fprintf(stderr, "%s(): ", func);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
    }
    va_end(args);
}


void
errprintf(char *format, ...)
{
  //fn prints an error string, then passes arguments on to vprintf.

  printf("%s ", ayyi_err);

  va_list argp;           //points to each unnamed arg in turn
  va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
  vprintf(format, argp);
  va_end(argp);           //clean up
}


void
errprintf2(const char* func, char *format, ...)
{
  //fn prints an error string, then passes arguments on to vprintf.

  printf("%s %s(): ", ayyi_err, func);

  va_list argp;           //points to each unnamed arg in turn
  va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
  vprintf(format, argp);
  va_end(argp);           //clean up
}


void
errprintf3(const char* func, char *format, ...)
{
  //fn prints an error string, then passes arguments on to vprintf.

  printf("%s %s(): ", ayyi_err, func); //move to log_handler

  char str[256];

  va_list argp;           //points to each unnamed arg in turn
  va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
  vsprintf(str, format, argp);
  va_end(argp);           //clean up

  g_critical("%s", str);
}


void
warnprintf(char *format, ...)
{
	//fn prints a warning string, then passes arguments on to vprintf.

	printf("%s ", warn);

	va_list argp;           //points to each unnamed arg in turn
	va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
	vprintf(format, argp);
	va_end(argp);           //clean up
}


void 
warnprintf2(const char* func, char *format, ...)
{
  //fn prints a warning string, then passes arguments on to vprintf.

  printf("%s %s(): ", ayyi_warn, func);

  va_list argp;
  va_start(argp, format);
  vprintf(format, argp);
  va_end(argp);
}


void
warn_gerror(const char* msg, GError** error)
{
	//print and free the GEerror
	if(*error){
		printf("%s %s: %s\n", ayyi_warn, msg, (*error)->message);
		g_error_free(*error);
		*error = NULL;
	}
}


void
info_gerror(const char* msg, GError** error)
{
	//print and free the GEerror
	if(*error){
		printf("%s: %s\n", msg, (*error)->message);
		g_error_free(*error);
		*error = NULL;
	}
}


gchar*
path_from_utf8(const gchar* utf8)
{
	gchar *path;
	GError *error = NULL;

	if (!utf8) return NULL;

	path = g_locale_from_utf8(utf8, -1, NULL, NULL, &error);
	if (error)
		{
		printf("Unable to convert to locale from UTF-8:\n%s\n%s\n", utf8, error->message);
		g_error_free(error);
		}
	if (!path)
		{
		/* if invalid UTF-8, text probaby still in original form, so just copy it */
		path = g_strdup(utf8);
		}

	return path;
}


GList*
get_dirlist(const char* path)
{
	/*
	scan a directory and return a list of any subdirectoies. Not recursive.
	-the list, and each entry in it,  must be freed.
	*/

	GList* dir_list = NULL;
	char filepath[256];
	G_CONST_RETURN gchar *file;
	GError *error = NULL;
	GDir *dir;
	if((dir = g_dir_open(path, 0, &error))){
		while((file = g_dir_read_name(dir))){
			if(file[0]=='.') continue;
			snprintf(filepath, 128, "%s/%s", path, file);

			if(g_file_test(filepath, G_FILE_TEST_IS_DIR)){
				dbg (2, "found dir: %s", filepath);
				dir_list = g_list_append(dir_list, g_strdup(filepath));
			}
		}
		g_dir_close(dir);
	}else{
		if(debug > 1) gwarn ("cannot open directory. %s", error->message);
		g_error_free(error);
		error = NULL;
	}
	return dir_list;
}


void
string_increment_suffix(char* new, const char* orig, int new_max)
{
  //change, eg, "name-2" to "name-3"
  //FIXME length of "new" musn't exceed new_max.
  //this increases the length of a string of unknown length so is slightly dangerous.

  char delimiter[] = "-";

  //split the string by delimiter:
  char* suffix = g_strrstr(orig, delimiter);
  if(!suffix){ dbg (2, "delimiter not found"); sprintf(new, "%s%s1", orig, delimiter); return; } //delimiter not found.
  if(suffix == orig + strlen(orig)){ dbg (0, "delimiter at end: %s", orig); sprintf(new, "%s1", orig); return; } //delimiter is at the end.
  suffix++; 
  unsigned pos = suffix - orig;
  dbg (2, "pos=%i suffix=%s", pos, suffix);
 
  int number = atoi(suffix);

  char stem[256];
  strncpy(stem, orig, pos-1);
  stem[pos-1] = 0; //strncpy doesnt terminate the string
  dbg (2, "stem=%s num=%i->%i", stem, number, number+1);
  sprintf(new, "%s%s%i", stem, delimiter, number+1);
}


int
get_terminal_width()
{
	struct winsize ws;

	if (ioctl(1, TIOCGWINSZ, (void *)&ws) != 0) printf("ioctl failed\n");

	//printf("terminal width = %d\n", ws.ws_col);

	return ws.ws_col;
}


gchar*
audio_path_get_base(const char* path)
{
	//strips off directory information, file extension, L/R flags, trailing underscores.

	gchar* basename = g_path_get_basename(path);

	char* pos;
	if((pos = g_strrstr(basename, "."))){
		*pos = '\0';
	}

	if((pos = g_strrstr(basename, "-L")) == basename + strlen(basename) - 2){
		*pos = '\0';
	}
	if((pos = g_strrstr(basename, "-R")) == basename + strlen(basename) - 2){
		*pos = '\0';
	}

	void remove_trailing(char* s)
	{
		while(s[strlen(s) -1] == '_') s[strlen(s) -1] = '\0';
	}

	remove_trailing(basename);

	return basename;
}


gboolean
audio_path_get_wav_leaf(char* leaf, const char* path, int len)
{
	//gives the leafname contained in the given path but substitutes a '.wav' extension.
	//-if the path is a directory, leaf should be empty, but without checking we have no way of knowing...

	g_return_val_if_fail(strlen(path), FALSE);

	//look for slashes and chop off anything before:
	char* pos;
	if((pos = g_strrstr(path, "/"))){
		pos += 1; //move to the rhs of the slash.
	}else pos = (char*)path;

	//make leaf contain the last segment:
	g_strlcpy(leaf, pos, len);

	dbg (2, "path=%s --> %s", path, leaf);

	if((pos = g_strrstr(leaf, "."))){
		strcpy(pos + 1, "wav");
	}

	return TRUE;
}


char*
audio_path_truncate(char* path, char chr)
{
	//remove any instances of @chr from the end of the path string.

	while(path[strlen(path) - 1] == chr){
		path[strlen(path) - 1] = '\0';
	}
	return path;
}


