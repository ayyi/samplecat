/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2014 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif
#include "debug/debug.h"
#include "file_manager/mimetype.h"

#include "samplecat/support.h"


/* Returns TRUE if the object exists, FALSE if it doesn't.
 * For symlinks, the file pointed to must exist.
 */
gboolean 
file_exists(const char *path)
{
	struct stat info;
	return !stat(path, &info);
}

time_t
file_mtime(const char *path)
{
	struct stat info;
	if (stat(path, &info)) return -1;
	return info.st_mtime;
}

gboolean
is_dir(const char *path)
{
	struct stat info;
	return lstat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

gboolean
dir_is_empty(const char *path)
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
file_extension(const char* path, char* extn)
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
mimestring_is_unsupported(char* mime_string)
{
	MIME_type* mime_type = mime_type_lookup(mime_string);
	return mimetype_is_unsupported(mime_type, mime_string);
}


gboolean
mimetype_is_unsupported(MIME_type* mime_type, char* mime_string)
{
	g_return_val_if_fail(mime_type, true);
	int i;

	/* XXX - actually ffmpeg can read audio-tracks in video-files,
	 * application/ogg, application/annodex, application/zip may contain audio
	 * ...
	 */
	char supported[][64] = {
		"application/ogg",
		"video/x-theora+ogg"
	};
	for(i=0;i<G_N_ELEMENTS(supported);i++){
		if(!strcmp(mime_string, supported[i])){
			dbg(2, "mimetype ok: %s", mime_string);
			return false;
		}
	}

	if(strcmp(mime_type->media_type, "audio")){
		return true;
	}

	char unsupported[][64] = {
		"audio/csound", 
		"audio/midi", 
		"audio/prs.sid",
		"audio/telephone-event",
		"audio/tone",
		//"audio/x-tta", 
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


gboolean
ensure_config_dir()
{
	static char* path = NULL;
	if(!path) path = g_strdup_printf("%s/.config/" PACKAGE, g_get_home_dir()); // is static - don't free.

	return (!g_mkdir_with_parents(path, 488));
}


void
colour_get_float(GdkColor* c, float* r, float* g, float* b, const unsigned char alpha)
{
	//convert GdkColor for use with Cairo.

	double _r = c->red;
	double _g = c->green;
	double _b = c->blue;

	*r = _r / 0xffff;
	*g = _g / 0xffff;
	*b = _b / 0xffff;
}


uint8_t*
pixbuf_to_blob(GdkPixbuf* in, guint *len)
{
	if(!in){ 
		if (len) *len = 0;
		return NULL;
	}
	//serialise the pixbuf:
	GdkPixdata pixdata;
	gdk_pixdata_from_pixbuf(&pixdata, in, 0);
	guint length;
	guint8* ser = gdk_pixdata_serialize(&pixdata, &length);
#ifdef HAVE_ZLIB
	unsigned long dsize=compressBound(length);
	unsigned char* dst= malloc(dsize*sizeof(char));
	int rv = compress(dst, &dsize, (const unsigned char *)ser, length);
	if(rv == Z_OK) {
		dbg(1, "compressed pixbuf %d -> %d", length, dsize);
		if (len) *len = dsize;
		free(ser);
		return dst;
	} else {
		dbg(2, "compression error");
	}
#endif
	if (len) *len = length;
	return ser;
}


void
samplerate_format(char* str, int samplerate)
{
	// format a samplerate given in Hz to be output in kHz

	if(!samplerate){ str[0] = '\0'; return; }

	snprintf(str, 32, "%f", ((float)samplerate) / 1000.0);
	while(str[strlen(str)-1]=='0'){
		str[strlen(str)-1] = '\0';
	}
	if(str[strlen(str)-1]=='.') str[strlen(str)-1] = '\0';
}

void
bitrate_format(char* str, int bitrate)
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
bitdepth_format(char* str, int bitdepth)
{
	if(bitdepth<1){ str[0] = '\0'; return; }
	snprintf(str, 32, "%d b/sample", bitdepth);
	str[31] = '\0';
}


char*
dir_format(char* dir)
{
	if(dir && (strstr(dir, g_get_home_dir()) == dir)) return dir + strlen(g_get_home_dir()) + 1;
	else return dir;
}


gchar*
format_channels(int n_ch)
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
format_smpte(char* str, int64_t t /*milliseconds*/)
{
	snprintf(str, 64, "%02d:%02d:%02d.%03d", (int)(t/3600000), (int)(t/60000)%60, (int)(t/1000)%60, (int)(t%1000));
	str[63] = '\0';
}


