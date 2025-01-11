/*
 +----------------------------------------------------------------------+
 | This file is part of libwaveform https://github.com/ayyi/libwaveform |
 | copyright (C) 2013-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#define __common_c__

#include "config.h"
#undef USE_GTK
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#define XLIB_ILLEGAL_ACCESS // needed to access Display internals
#include <X11/Xlib.h>
#include <glib-object.h>
#include "agl/actor.h"
#ifdef USE_EPOXY
# define __glx_test__
#endif
#include "keys.h"
#include "test/common2.h"


char*
find_wav (const char* wav)
{
	if(wav[0] == '/'){
		return g_strdup(wav);
	}
	char* filename = g_build_filename(g_get_current_dir(), wav, NULL);
	if(g_file_test(filename, G_FILE_TEST_EXISTS)){
		return filename;
	}
	g_free(filename);

	filename = g_build_filename(g_get_current_dir(), "test", wav, NULL);
	if(g_file_test(filename, G_FILE_TEST_EXISTS)){
		return filename;
	}
	g_free(filename);

	filename = g_build_filename(g_get_current_dir(), "test/data", wav, NULL);
	if(g_file_test(filename, G_FILE_TEST_EXISTS)){
		return filename;
	}
	g_free(filename);

	filename = g_build_filename(g_get_current_dir(), "data", wav, NULL);
	if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
		return filename;
	}
	g_free(filename);

	return NULL;
}


const char*
find_data_dir ()
{
	static char* dirs[] = {"test/data", "data"};

	for (int i=0;i<G_N_ELEMENTS(dirs);i++) {
		if (g_file_test(dirs[i], G_FILE_TEST_EXISTS)) {
			return dirs[i];
		}
	}

	return NULL;
}


static GHashTable* key_handlers = NULL;

KeyHandler*
key_lookup (int keycode)
{
	return key_handlers ? g_hash_table_lookup(key_handlers, &keycode) : NULL;
}


/**
 * Display the refresh rate of the display using the GLX_OML_sync_control
 * extension.
 */
void
show_refresh_rate (Display* dpy)
{
#if defined(GLX_OML_sync_control) && defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
	int32_t  n;
	int32_t  d;

	PFNGLXGETMSCRATEOMLPROC get_msc_rate = (PFNGLXGETMSCRATEOMLPROC)glXGetProcAddressARB((const GLubyte*) "glXGetMscRateOML");
	if (get_msc_rate != NULL) {
		(*get_msc_rate)(dpy, glXGetCurrentDrawable(), &n, &d);
		printf( "refresh rate: %.1fHz\n", (float) n / d);
		return;
	}
#endif
	printf("glXGetMscRateOML not supported.\n");
}


void
add_key_handlers (AGlKey keys[])
{
	if (!key_handlers) {
		key_handlers = g_hash_table_new(g_int_hash, g_int_equal);

		int i = 0; while(true) {
			AGlKey* key = &keys[i];
			if (i > 100 || !key->key) break;
			g_hash_table_insert(key_handlers, &key->key, key->handler);
			i++;
		}
	}
}
