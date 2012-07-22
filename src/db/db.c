/*
  This file is part of Samplecat. http://samplecat.orford.org
  copyright (C) 2007-2012 Tim Orford and others.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 3
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#define _GNU_SOURCE
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include "typedefs.h"
#include "utils/ayyi_utils.h"
#include "sample.h"
#ifdef USE_MYSQL
#include "db/mysql.h"
#endif
#ifdef USE_SQLITE
#include "db/sqlite.h"
#endif
#ifdef USE_TRACKER
#include "tracker.h"
#endif
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif
#include "db/db.h"

extern SamplecatBackend backend;


gboolean
samplecat_set_backend(BackendType type)
{
	backend.pending = false;

	switch(type){
		case BACKEND_MYSQL:
			#ifdef USE_MYSQL
			if(mysql__connect()){
				mysql__set_as_backend(&backend);
				printf("backend is mysql.\n");
				return true;
			}
			return false;
			#endif
			break;
		case BACKEND_SQLITE:
			#ifdef USE_SQLITE
			backend.search_iter_new  = sqlite__search_iter_new;
			backend.search_iter_next = sqlite__search_iter_next;
			backend.search_iter_free = sqlite__search_iter_free;

			backend.dir_iter_new     = sqlite__dir_iter_new;
			backend.dir_iter_next    = sqlite__dir_iter_next;
			backend.dir_iter_free    = sqlite__dir_iter_free;

			backend.insert           = sqlite__insert;
			backend.remove           = sqlite__delete_row;
			backend.file_exists      = sqlite__file_exists;
			backend.filter_by_audio  = sqlite__filter_by_audio;

			backend.update_string    = sqlite__update_string;
			backend.update_float     = sqlite__update_float;
			backend.update_int       = sqlite__update_int;
			backend.update_blob      = sqlite__update_blob;

			backend.disconnect       = sqlite__disconnect;

			printf("backend is sqlite.\n");
			#endif
			break;
		case BACKEND_TRACKER:
			#ifdef USE_TRACKER
			backend.search_iter_new  = tracker__search_iter_new;
			backend.search_iter_next = tracker__search_iter_next;
			backend.search_iter_free = tracker__search_iter_free;

			backend.dir_iter_new     = tracker__dir_iter_new;
			backend.dir_iter_next    = tracker__dir_iter_next;
			backend.dir_iter_free    = tracker__dir_iter_free;

			backend.insert           = tracker__insert;
			backend.remove           = tracker__delete_row;
			backend.file_exists      = tracker__file_exists;
			backend.filter_by_audio  = tracker__filter_by_audio;

			backend.update_string    = tracker__update_string;
			backend.update_float     = tracker__update_float;
			backend.update_int       = tracker__update_int;
			backend.update_blob      = tracker__update_blob;

			backend.disconnect       = tracker__disconnect;
			printf("backend is tracker.\n");
			#endif
			break;
		default:
			break;
	}
	return true;
}


GdkPixbuf*
blob_to_pixbuf(const unsigned char* blob, const guint len)
{
	GdkPixdata pixdata;
	GdkPixbuf* pixbuf = NULL;
#ifdef HAVE_ZLIB
	unsigned long dsize=1024*32; // TODO - save orig-length along w/ blob 
	//here: ~ 16k = 400*20*4bpp = OVERVIEW_HEIGHT * OVERVIEW_WIDTH * 4bpp + GDK-OVERHEAD
	unsigned char* dst = malloc(dsize*sizeof(char));
	int rv = uncompress(dst, &dsize, blob, len);
	if(rv == Z_OK) {
		if(gdk_pixdata_deserialize(&pixdata, dsize, dst, NULL)){
				pixbuf = gdk_pixbuf_from_pixdata(&pixdata, TRUE, NULL);
		}
		dbg(2, "decompressed pixbuf %d -> %d", len, dsize);
	} else {
		dbg(2, "decompression failed");
		if(gdk_pixdata_deserialize(&pixdata, len, (guint8*)blob, NULL)){
				pixbuf = gdk_pixbuf_from_pixdata(&pixdata, TRUE, NULL);
		}
	}
	free(dst);
#else
	if(gdk_pixdata_deserialize(&pixdata, len, (guint8*)blob, NULL)){
			pixbuf = gdk_pixbuf_from_pixdata(&pixdata, TRUE, NULL);
	}
#endif
	return pixbuf;
}


