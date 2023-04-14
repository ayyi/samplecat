/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif
#include "debug/debug.h"
#include "file_manager/mimetype.h"

#include "samplecat/support.h"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"


void
p_(int level, const char* format, ...)
{
	va_list argp;
	va_start(argp, format);
	if (level <= _debug_) {
		gchar* s = g_strdup_vprintf(format, argp);
		fprintf(stdout, "%s\n", s);
		g_free(s);
	}
	va_end(argp);
}


/* Returns TRUE if the object exists, FALSE if it doesn't.
 * For symlinks, the file pointed to must exist.
 */
gboolean 
file_exists (const char *path)
{
	struct stat info;
	return !stat(path, &info);
}

time_t
file_mtime (const char *path)
{
	struct stat info;
	if (stat(path, &info)) return -1;
	return info.st_mtime;
}

gboolean
is_dir (const char *path)
{
	struct stat info;
	return lstat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

gboolean
dir_is_empty (const char *path)
{

	if (strcmp(path, "/") == 0) return FALSE;

	DIR* dp = opendir (path);

	int n = 0;
	while(readdir(dp) != NULL){
		n++;
		if (n > 2) {
			closedir(dp);
			return FALSE;
		}
	}
	closedir(dp);
	return TRUE;
}

void
file_extension (const char* path, char* extn)
{
	g_return_if_fail(path);
	g_return_if_fail(extn);

	gchar** split = g_strsplit(path, ".", 0);
	if(split[0]){
		int i = 1;
		while(split[i]) i++;
		dbg(2, "extn=%s i=%i", split[i-1], i);
		strcpy(extn, split[i-1]);
		
	}else{
		dbg(0, "failed (%s)", path);
		memset(extn, '\0', 1);
	}

	g_strfreev(split);
}


gboolean
can_use (GList* l, const char* d)
{
	for(;l;l=l->next){
		if(!strcmp(l->data, d)){
			return true;
		}
	}
	return false;
}


gboolean
mimestring_is_unsupported (char* mime_string)
{
	MIME_type* mime_type = mime_type_lookup(mime_string);
	return mimetype_is_unsupported(mime_type, mime_string);
}


gboolean
mimetype_is_unsupported (MIME_type* mime_type, char* mime_string)
{
	g_return_val_if_fail(mime_type, true);

	char* supported[] = {
		"application/ogg",
	};
	int i; for(i=0;i<G_N_ELEMENTS(supported);i++){
		if(!strcmp(mime_string, supported[i])){
			dbg(2, "mimetype ok: %s", mime_string);
			return false;
		}
	}

	if(strcmp(mime_type->media_type, "audio") && strcmp(mime_type->media_type, "video")){
		return true;
	}

	char* unsupported[] = {
		"audio/csound", 
		"audio/midi", 
		"audio/prs.sid",
		"audio/telephone-event",
		"audio/tone",
		"audio/x-speex",
		"audio/x-musepack",
		"audio/x-mpegurl",
	};
	for(i=0;i<G_N_ELEMENTS(unsupported);i++){
		if(!strcmp(mime_string, unsupported[i])){
			return true;
		}
	}
	dbg(2, "mimetype ok: %s", mime_string);
	return false;
}


#include "file_manager/support.h"
#include "file_manager/mimetype.h"
#include "file_manager/pixmaps.h"

GdkPixbuf*
get_iconbuf_from_mimetype (char* mimetype)
{
	GdkPixbuf* iconbuf = NULL;
	MIME_type* mime_type = mime_type_lookup(mimetype);
	if(mime_type){
		type_to_icon(mime_type);
		if (mime_type->image)
			iconbuf = mime_type->image->sm_pixbuf;
		else
			dbg(1, "no icon.");
	}
	return iconbuf;
}


gboolean
ensure_config_dir ()
{
	static char* path = NULL;
	if(!path) path = g_strdup_printf("%s/.config/" PACKAGE, g_get_home_dir()); // is static - don't free.

	return (!g_mkdir_with_parents(path, 488));
}


uint8_t*
pixbuf_to_blob (GdkPixbuf* in, guint *len)
{
	if(!in){ 
		if (len) *len = 0;
		return NULL;
	}

	// Serialise the pixbuf
	GdkPixdata pixdata;
	gdk_pixdata_from_pixbuf(&pixdata, in, 0);
	guint length;
	guint8* ser = gdk_pixdata_serialize(&pixdata, &length);
#ifdef HAVE_ZLIB
	unsigned long dsize = compressBound(length);
	unsigned char* dst = g_malloc(dsize * sizeof(char));
	int rv = compress(dst, &dsize, (const unsigned char*)ser, length);
	if (rv == Z_OK) {
		dbg(1, "compressed pixbuf %d -> %d", length, dsize);
		if (len) *len = dsize;
		g_free(ser);
		return dst;
	} else {
		dbg(2, "compression error");
	}
#endif
	if (len) *len = length;

	return ser;
}


void
samplerate_format (char* str, int samplerate)
{
	// format a samplerate given in Hz to be output in kHz

	if(!samplerate){ str[0] = '\0'; return; }

	snprintf(str, 32, "%f", ((float)samplerate) / 1000.0);
	while(str[strlen(str)-1] == '0'){
		str[strlen(str)-1] = '\0';
	}
	if(str[strlen(str)-1] == '.') str[strlen(str)-1] = '\0';
}


void
bitrate_format (char* str, int bitrate)
{
	if(bitrate<1){ str[0] = '\0'; return; }
	else if (bitrate < 1000) snprintf(str, 32, "%d b/s", bitrate);
	else if (bitrate < 1000000) snprintf(str, 32, "%.1f kb/s", bitrate/1000.0);
	else if (bitrate < 8192000) snprintf(str, 32, "%.1f kB/s", bitrate/8192.0);
	else if (bitrate < 1000000000) snprintf(str, 32, "%.1f Mb/s", bitrate/1000000.0);
	else snprintf(str, 32, "%.1f MB/s", bitrate/8192000.0);
	str[31] = '\0';
}


void
bitdepth_format (char* str, int bitdepth)
{
	if(bitdepth<1){ str[0] = '\0'; return; }
	snprintf(str, 32, "%d b/sample", bitdepth);
	str[31] = '\0';
}


char*
dir_format (char* dir)
{
	if (dir) {
		if (!strcmp(dir, g_get_home_dir()))
			return dir + strlen(g_get_home_dir());
		else if(strstr(dir, g_get_home_dir()) == dir)
			return dir + strlen(g_get_home_dir()) + 1;
	}

	return dir;
}


gchar*
format_channels (int n_ch)
{
	switch(n_ch){
		case 1:
			return g_strdup("mono");
			break;
		case 2:
			return g_strdup("stereo");
			break;
	}
	return g_strdup_printf("%i channels", n_ch);
}


void
format_smpte (char* str, int64_t t /*milliseconds*/)
{
	snprintf(str, 32, "%02d:%02d:%02d.%03d", (int)(t/3600000), (int)(t/60000)%60, (int)(t/1000)%60, (int)(t%1000));
	str[31] = '\0';
}


float
gain2db (float gain)
{
	union {float f; int i;} t;
	t.f = gain;
	int * const    exp_ptr =  &t.i;
	int            x = *exp_ptr;
	const int      log_2 = ((x >> 23) & 255) - 128;
	x &= ~(255 << 23);
	x += 127 << 23;
	*exp_ptr = x;

	gain = ((-1.0f/3) * t.f + 2) * t.f - 2.0f/3;

	return 20.0f * (gain + log_2) * 0.69314718f;
}


char*
gain2dbstring (float gain)
{
	//result must be freed by caller

	float dB = gain2db(gain);

	if(dB < -200)
		return g_strdup("");

	return g_strdup_printf("%.2f dBA", gain2db(gain));
}


gchar*
str_replace (const gchar* string, const gchar* search, const gchar* replacement)
{
	gchar *str, **arr;

	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (search != NULL, NULL);

	if (replacement == NULL) replacement = "";

	arr = g_strsplit (string, search, -1);
	if (arr != NULL && arr[0] != NULL)
		str = g_strjoinv (replacement, arr);
	else
		str = g_strdup (string);

	g_strfreev (arr);

	return str;
}


char*
remove_trailing_slash (char* path)
{
	size_t len = strlen(path);
	if ((len > 0) && (path[len-1] == '/')) path[len-1] = '\0';
	return path;
}
