/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2020 Tim Orford <tim@orford.org> and others       |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#define _GNU_SOURCE
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include "debug/debug.h"
#include "samplecat/support.h"
#include "samplecat/model.h"
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

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#define backend samplecat.model->backend

static SamplecatDBConfig* mysql_config;


void
db_init(SamplecatDBConfig* _mysql)
{
	mysql_config = _mysql;
}


gboolean
db_connect()
{
	gboolean db_connected = false;
#ifdef USE_MYSQL
	if(can_use(samplecat.model->backends, "mysql")){
		db_connected = samplecat_set_backend(BACKEND_MYSQL);
	}
#endif
#ifdef USE_SQLITE
	if(!db_connected && can_use(samplecat.model->backends, "sqlite")){
		if(sqlite__connect()){
			db_connected = samplecat_set_backend(BACKEND_SQLITE);
		}
	}
#endif

	return db_connected;
}


gboolean
samplecat_set_backend(BackendType type)
{
	backend.pending = false;

	switch(type){
		case BACKEND_MYSQL:
			#ifdef USE_MYSQL
			mysql__set_as_backend(&backend, mysql_config);
			if(mysql__connect()){
				printf("database is mysql.\n");
				return true;
			}
			return false;
			#endif
			break;
		case BACKEND_SQLITE:
			#ifdef USE_SQLITE
			sqlite__set_as_backend(&backend);

			if(_debug_) printf("database is sqlite.\n");
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
			printf("database is tracker.\n");
			#endif
			break;
		default:
			break;
	}
	backend.init(
#ifdef USE_MYSQL
		mysql_config
#else
		NULL
#endif
	);
	return true;
}


/*
 *  Copy of deprecated gdk_pixbuf_from_pixdata
 *
 *  @copy_pixels: whether to copy raw pixel data; run-length encoded
 */
static GdkPixbuf*
pixbuf_from_pixdata (const GdkPixdata* pixdata, gboolean copy_pixels, GError** error)
{
	/* From glib's gmem.c */
	#define SIZE_OVERFLOWS(a,b) (G_UNLIKELY ((b) > 0 && (a) > G_MAXSIZE / (b)))

	#define RLE_OVERRUN(offset) (rle_buffer_limit == NULL ? FALSE : rle_buffer + (offset) > rle_buffer_limit)

	g_return_val_if_fail (pixdata != NULL, NULL);
	g_return_val_if_fail (pixdata->width > 0, NULL);
	g_return_val_if_fail (pixdata->height > 0, NULL);
	g_return_val_if_fail (pixdata->rowstride >= pixdata->width, NULL);
	g_return_val_if_fail ((pixdata->pixdata_type & GDK_PIXDATA_COLOR_TYPE_MASK) == GDK_PIXDATA_COLOR_TYPE_RGB || (pixdata->pixdata_type & GDK_PIXDATA_COLOR_TYPE_MASK) == GDK_PIXDATA_COLOR_TYPE_RGBA, NULL);
	g_return_val_if_fail ((pixdata->pixdata_type & GDK_PIXDATA_SAMPLE_WIDTH_MASK) == GDK_PIXDATA_SAMPLE_WIDTH_8, NULL);
	g_return_val_if_fail ((pixdata->pixdata_type & GDK_PIXDATA_ENCODING_MASK) == GDK_PIXDATA_ENCODING_RAW || (pixdata->pixdata_type & GDK_PIXDATA_ENCODING_MASK) == GDK_PIXDATA_ENCODING_RLE, NULL);
	g_return_val_if_fail (pixdata->pixel_data != NULL, NULL);

	guint bpp = (pixdata->pixdata_type & GDK_PIXDATA_COLOR_TYPE_MASK) == GDK_PIXDATA_COLOR_TYPE_RGB ? 3 : 4;
	guint encoding = pixdata->pixdata_type & GDK_PIXDATA_ENCODING_MASK;

#if 0
	g_debug ("gdk_pixbuf_from_pixdata() called on:");
	g_debug ("\tEncoding %s", encoding == GDK_PIXDATA_ENCODING_RAW ? "raw" : "rle");
	g_debug ("\tDimensions: %d x %d", pixdata->width, pixdata->height);
	g_debug ("\tRowstride: %d, Length: %d", pixdata->rowstride, pixdata->length);
	g_debug ("\tCopy pixels == %s", copy_pixels ? "true" : "false");
#endif

	if (encoding == GDK_PIXDATA_ENCODING_RLE){
		copy_pixels = TRUE;
	}

	/* Sanity check the length and dimensions */
	if (SIZE_OVERFLOWS (pixdata->height, pixdata->rowstride)) {
		g_set_error_literal (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE, "Image pixel data corrupt");
		return NULL;
	}

	if (encoding == GDK_PIXDATA_ENCODING_RAW && pixdata->length >= 1 && pixdata->length < pixdata->height * pixdata->rowstride - GDK_PIXDATA_HEADER_LENGTH) {
		g_set_error_literal (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE, "Image pixel data corrupt");
		return NULL;
	}

	guint8* data = NULL;
	if (copy_pixels) {
		data = g_try_malloc_n (pixdata->height, pixdata->rowstride);
		if (!data) {
			g_error ("failed to allocate image buffer of %u byte", pixdata->rowstride * pixdata->height);
			return NULL;
		}
	}
	if (encoding == GDK_PIXDATA_ENCODING_RLE) {
		const guint8 *rle_buffer = pixdata->pixel_data;
		guint8 *rle_buffer_limit = NULL;
		guint8 *image_buffer = data;
		guint8 *image_limit = data + pixdata->rowstride * pixdata->height;
		gboolean check_overrun = FALSE;

		if (pixdata->length >= 1)
			rle_buffer_limit = pixdata->pixel_data + pixdata->length - GDK_PIXDATA_HEADER_LENGTH;

		while (image_buffer < image_limit && (rle_buffer_limit != NULL || rle_buffer > rle_buffer_limit)) {
			if (RLE_OVERRUN(1)) {
				check_overrun = TRUE;
				break;
			}

			guint length = *(rle_buffer++);

			if (length & 128) {
				length = length - 128;
				check_overrun = image_buffer + length * bpp > image_limit;
				if (check_overrun)
					length = (image_limit - image_buffer) / bpp;
				if (RLE_OVERRUN(bpp < 4 ? 3 : 4)) {
					check_overrun = TRUE;
					break;
				}
				if (bpp < 4)	/* RGB */
					do {
						memcpy (image_buffer, rle_buffer, 3);
						image_buffer += 3;
					}
					while (--length);
				else		/* RGBA */
					do {
						memcpy (image_buffer, rle_buffer, 4);
						image_buffer += 4;
					}
					while (--length);
				if (RLE_OVERRUN(bpp)) {
					check_overrun = TRUE;
					break;
				}
				rle_buffer += bpp;
			}
			else {
				length *= bpp;
				check_overrun = image_buffer + length > image_limit;
				if (check_overrun)
					length = image_limit - image_buffer;
				if (RLE_OVERRUN(length)) {
					check_overrun = TRUE;
					break;
				}
				memcpy (image_buffer, rle_buffer, length);
				image_buffer += length;
				rle_buffer += length;
			}
		}
		if (check_overrun) {
			g_free (data);
			g_set_error_literal (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE, "Image pixel data corrupt");
			return NULL;
		}
	}
	else if (copy_pixels)
		memcpy (data, pixdata->pixel_data, pixdata->rowstride * pixdata->height);
	else
		data = pixdata->pixel_data;

	return gdk_pixbuf_new_from_data (data,
		GDK_COLORSPACE_RGB,
		(pixdata->pixdata_type & GDK_PIXDATA_COLOR_TYPE_MASK) == GDK_PIXDATA_COLOR_TYPE_RGBA,
		8,
		pixdata->width, pixdata->height, pixdata->rowstride,
		copy_pixels ? (GdkPixbufDestroyNotify) g_free : NULL,
		data);
}


GdkPixbuf*
blob_to_pixbuf(const unsigned char* blob, const guint len)
{
	GdkPixdata pixdata;
	GdkPixbuf* pixbuf = NULL;
#ifdef HAVE_ZLIB
	unsigned long dsize = 1024 * 32; // TODO - save orig-length along w/ blob
	//here: ~ 16k = 400*20*4bpp = OVERVIEW_HEIGHT * OVERVIEW_WIDTH * 4bpp + GDK-OVERHEAD
	unsigned char* dst = malloc(dsize*sizeof(char));
	int rv = uncompress(dst, &dsize, blob, len);
	if(rv == Z_OK) {
		if(gdk_pixdata_deserialize(&pixdata, dsize, dst, NULL)){
			pixbuf = pixbuf_from_pixdata(&pixdata, TRUE, NULL);
		}
		dbg(2, "decompressed pixbuf %d -> %d", len, dsize);
	} else {
		dbg(2, "decompression failed");
		if(gdk_pixdata_deserialize(&pixdata, len, (guint8*)blob, NULL)){
			pixbuf = pixbuf_from_pixdata(&pixdata, TRUE, NULL);
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


