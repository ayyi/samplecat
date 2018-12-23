/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2018 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#define __wf_private__
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include "agl/ext.h"
#include "agl/utils.h"
#include "agl/actor.h"
#include "waveform/waveform.h"
#include "waveform/actors/text.h"
#include "file_manager/file_manager.h"
#include "file_manager/pixmaps.h"
#include "samplecat.h"
#include "application.h"
#include "views/scrollbar.h"
#include "views/files.impl.h"
#include "views/files.with_wav.h"

#define _g_free0(var) (var = (g_free (var), NULL))

#define row_height0 20
#define wav_height 30
#define row_spacing 8
#define row_height (row_height0 + wav_height + row_spacing)
#define N_ROWS_VISIBLE(A) (agl_actor__height(((AGlActor*)A)) / row_height)
#define scrollable_height (FILES->view->items->len)
#define max_scroll_offset (scrollable_height - N_ROWS_VISIBLE(actor) + 2)

#define FILES ((FilesView*)view)
#define SCROLLBAR ((ScrollbarActor*)((FilesView*)actor)->scrollbar)

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Files", (AGlActorNew*)files_with_wav};
static GHashTable* icon_textures = NULL;

static gboolean files_scan_dir           (AGlActor*);
static void     files_with_wav_on_scroll (Observable*, int row, gpointer view);
static guint    create_icon              (const char*, GdkPixbuf*);
static guint    get_icon                 (const char*, GdkPixbuf*);


AGlActorClass*
files_with_wav_get_class ()
{
	return &actor_class;
}


static void
_init()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();

		icon_textures = g_hash_table_new(NULL, NULL);
		agl_set_font_string("Roboto 10"); // initialise the pango context

		dir_init();

		init_done = true;
	}
}

// TODO this is a copy of private AGlActor fn.
//      In this particular case we can just check the state of the Scrollbar child
static bool
agl_actor__is_animating(AGlActor* a)
{
	if(a->transitions) return true;

	GList* l = a->children;
	for(;l;l=l->next){
		if(agl_actor__is_animating((AGlActor*)l->data)) return true;
	}
	return false;
}


AGlActor*
files_with_wav(gpointer _)
{
	instance_count++;

	_init();

	bool files_paint(AGlActor* actor)
	{
		FilesWithWav* view = (FilesWithWav*)actor;
		DirectoryView* dv = FILES->view;
		GPtrArray* items = dv->items;

		int n_rows = N_ROWS_VISIBLE(actor);

		int col[] = {0, 24, 260, 360, 400, 440, 480};
		char* col_heads[] = {"Filename", "Size", "Date", "Owner", "Group"};
		int sort_column = 1;

#ifdef AGL_ACTOR_RENDER_CACHE
		int y0 = 0;
		bool is_animating = agl_actor__is_animating(actor);
		if(is_animating) y0 = -actor->scrollable.y1;
#else
		int y0 = -actor->scrollable.y1;
#endif

		agl->shaders.plain->uniform.colour = 0x222222ff;
		agl_use_program((AGlShader*)agl->shaders.plain);
		agl_rect_((AGlRect){col[sort_column], y0 - 2, col[sort_column + 1] - col[sort_column], row_height0});

		int c; for(c=0;c<G_N_ELEMENTS(col_heads);c++){
			agl_enable_stencil(0, y0, col[c + 2] - 6, actor->region.y2);
			agl_print(col[c + 1], y0, 0, app->style.text, col_heads[c]);
		}

		if(!items->len)
			return agl_print(0, 0, 0, app->style.text, "No files"), true;

		y0 += row_height0;
		int offset = SCROLLBAR->scroll->value;
		int i, r; for(i = offset; r = i - offset, i < items->len && (i - offset < n_rows); i++){
			int y = y0 + r * row_height;
			if(r == FILES->view->selection - offset){
				agl->shaders.plain->uniform.colour = app->style.selection;
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_disable_stencil();
				agl_rect_((AGlRect){0, y - 2, agl_actor__width(actor) - 20, row_height0 + wav_height + 4});
			}else{
				// waveform background
				agl_disable_stencil();
				agl->shaders.plain->uniform.colour = app->style.bg_alt;
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_rect_((AGlRect){0, y + row_height0, agl_actor__width(actor) - 20, wav_height});
			}

			WavViewItem* vitem = items->pdata[i];
			DirItem* item = vitem->item;
			char size[16] = {'\0'}; snprintf(size, 15, "%zu", item->size);
			const char* val[] = {item->leafname, size, user_name(item->uid), group_name(item->gid)};
			int c; for(c=0;c<G_N_ELEMENTS(val);c++){
				agl_enable_stencil(0, y0, col[c + 2] - 6, actor->region.y2);
				agl_print(col[c + 1], y, 0, app->style.text, val[c]);
			}

			if(!vitem->wav){
				DirItem* item = vitem->item;
				char* name = item->leafname;
				WaveformActor* wa = wf_canvas_add_new_actor(view->wfc, NULL);
				agl_actor__add_child(actor, (AGlActor*)wa);
				Waveform* waveform = waveform_new(g_strdup_printf("%s/%s", FILES->path, name));
				wf_actor_set_waveform(wa, waveform, NULL, NULL);
				wf_actor_set_colour(wa, app->style.fg);
				wf_actor_set_rect(wa, &(WfRectangle){0, y + row_height0, agl_actor__width(actor) - 20, wav_height});
				vitem->wav = (AGlActor*)wa;
			}

			// TODO dont do this in paint
			if(item->mime_type){
				GdkPixbuf* pixbuf = mime_type_get_pixbuf(item->mime_type);
				if(GDK_IS_PIXBUF(pixbuf)){
					guint t = get_icon(item->mime_type->subtype, pixbuf);
					agl->shaders.texture->uniform.fg_colour = 0xffffffff;
					agl_use_program((AGlShader*)agl->shaders.texture);
					agl_textured_rect(t, 0, y, 16, 16, NULL);
				}else{
					gwarn("failed to get icon for %s", item->mime_type->subtype);
				}
			}
		}

		agl_disable_stencil();

		return true;
	}

	void files_init(AGlActor* a)
	{
		FilesWithWav* view = (FilesWithWav*)a;
		AGlActor* actor = a;

#ifdef AGL_ACTOR_RENDER_CACHE
		a->fbo = agl_fbo_new(agl_actor__width(a), agl_actor__height(a), 0, AGL_FBO_HAS_STENCIL);
		a->cache.enabled = true;
#endif

		observable_set(SCROLLBAR->scroll, 0);

		g_idle_add((GSourceFunc)files_scan_dir, a);

		void files_on_row_add (GtkTreeModel* tree_model, GtkTreePath* path, GtkTreeIter* iter, AGlActor* actor)
		{
			DirectoryView* dv = ((FilesView*)actor)->view;
			GPtrArray* items = dv->items;

			actor->scrollable.y2 = actor->scrollable.y1 + (items->len + 1) * row_height;

			agl_actor__invalidate(actor);
		}
		g_signal_connect(FILES->viewmodel, "row-inserted", (GCallback)files_on_row_add, a);

		void files_on_row_change (GtkTreeModel* tree_model, GtkTreePath* path, GtkTreeIter* iter, AGlActor* actor)
		{
			FilesWithWav* view = (FilesWithWav*)actor;
			DirectoryView* dv = FILES->view;
			GPtrArray* items = dv->items;

			int r = GPOINTER_TO_INT(iter->user_data);
			if(r < 5){
				WavViewItem* vitem = items->pdata[r];
				DirItem* item = vitem->item;
				char* name = item->leafname;
			}
		}
		g_signal_connect(FILES->viewmodel, "row-changed", (GCallback)files_on_row_change, a);
	}

	void files_set_size(AGlActor* actor)
	{
		FilesWithWav* view = (FilesWithWav*)actor;
		DirectoryView* dv = FILES->view;

		if(SCROLLBAR->scroll->value > max_scroll_offset){
			observable_set(SCROLLBAR->scroll, max_scroll_offset);
		}
	}

	bool files_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		FilesWithWav* view = (FilesWithWav*)actor;

		switch(event->type){
			case GDK_SCROLL:
				// This event comes from scrollbar view after dragging the scrollbar handle
				;GdkEventMotion* motion = (GdkEventMotion*)event;
				dbg(1, "SCROLL %ipx/%i %i/%i", (int)motion->y, scrollable_height * row_height, MAX(((int)motion->y) / row_height, 0), scrollable_height);
				observable_set(SCROLLBAR->scroll, MAX(((int)motion->y) / row_height, 0));
				break;
			case GDK_BUTTON_PRESS:
				switch(event->button.button){
					case 4:
						dbg(1, "! scroll up");
						observable_set(SCROLLBAR->scroll, MAX(0, SCROLLBAR->scroll->value - 1));
						break;
					case 5:
						dbg(1, "! scroll down");
						if(scrollable_height > N_ROWS_VISIBLE(actor)){
							if(SCROLLBAR->scroll->value < max_scroll_offset)
								observable_set(SCROLLBAR->scroll, SCROLLBAR->scroll->value + 1);
						}
						break;
				}
				break;
			case GDK_BUTTON_RELEASE:
				;int row = files_with_wav_row_at_coord (view, 0, xy.y - actor->region.y1);
				dbg(1, "RELEASE button=%i y=%i row=%i", event->button.button, xy.y - actor->region.y1, row);
				switch(event->button.button){
					case 1:
						VIEW_IFACE_GET_CLASS((ViewIface*)FILES->view)->set_selected((ViewIface*)FILES->view, &(ViewIter){.i = row}, true);
				}
				break;
			default:
				break;
		}
		return AGL_HANDLED;
	}

	void files_free(AGlActor* actor)
	{
		FilesWithWav* view = (FilesWithWav*)actor;

		g_object_unref(FILES->view);

		if(!--instance_count){
		}
	}

	FilesWithWav* view = WF_NEW(FilesWithWav,
		.files = {
			.actor = {
				.class = &actor_class,
				.name = "Files",
				.colour = 0x66ff99ff,
				.init = files_init,
				.free = files_free,
				.paint = files_paint,
				.set_size = files_set_size,
				.on_event = files_event,
			},
			.viewmodel = vm_directory_new()
		}
	);
	AGlActor* actor = (AGlActor*)view;

	FILES->viewmodel->view = (ViewIface*)(FILES->view = directory_view_new(FILES->viewmodel, (FilesView*)view));

	agl_actor__add_child((AGlActor*)view, FILES->scrollbar = scrollbar_view(NULL, GTK_ORIENTATION_VERTICAL));

	observable_subscribe(SCROLLBAR->scroll, files_with_wav_on_scroll, view);

	return (AGlActor*)view;
}


void
files_with_wav_set_path (FilesWithWav* view, const char* path)
{
	FILES->path = g_strdup(path);
	g_idle_add((GSourceFunc)files_scan_dir, (gpointer)view);
}


static gboolean
files_scan_dir(AGlActor* a)
{
	FilesWithWav* view = (FilesWithWav*)a;

	vm_directory_set_path(FILES->viewmodel, FILES->path ? FILES->path : g_get_home_dir());
	agl_actor__invalidate(a);

	return G_SOURCE_REMOVE;
}


int
files_with_wav_row_at_coord (FilesWithWav* view, int x, int y)
{
	AGlActor* actor = (AGlActor*)view;

	#define header_height 20

	y += SCROLLBAR->scroll->value * row_height - header_height;
	if(y < 0) return -1;
	int r = y / row_height;
	GPtrArray* items = FILES->view->items;
	if(r > items->len) return -1;
	return r;
}


void
files_with_wav_select (FilesWithWav* view, int row)
{
	AGlActor* actor = (AGlActor*)view;
	DirectoryView* dv = FILES->view;

	if(row > -1 && row < FILES->view->items->len && row != view->files.view->selection){
		GPtrArray* items = dv->items;
		{
			WavViewItem* vitem = items->pdata[view->files.view->selection];
			if(vitem->wav){
				wf_actor_set_colour((WaveformActor*)vitem->wav, 0x66ff66ff);
			}
		}

		view->files.view->selection = row;

		WavViewItem* vitem = items->pdata[row];
		if(vitem->wav){
			wf_actor_set_colour((WaveformActor*)vitem->wav, 0xff6666ff);
		}

		iRange range = {SCROLLBAR->scroll->value, SCROLLBAR->scroll->value + N_ROWS_VISIBLE(view) - 1};
		if(row > range.end){
			observable_set(SCROLLBAR->scroll, row - N_ROWS_VISIBLE(view) + 1);
		}
		if(row < range.start){
			observable_set(SCROLLBAR->scroll, row);
		}

		agl_actor__invalidate((AGlActor*)view);
	}
}


#define HIDE_ITEM(N) \
	if(N > 0 && N < items->len){ \
		WavViewItem* vitem = items->pdata[N]; \
		if(vitem->wav){ \
			wf_actor_set_rect((WaveformActor*)vitem->wav, &(WfRectangle){0,}); \
		} \
	}


static void
files_with_wav_on_scroll (Observable* observable, int row, gpointer _view)
{
	AGlActor* actor = (AGlActor*)_view;
	FilesWithWav* view = (FilesWithWav*)_view;
	DirectoryView* dv = FILES->view;
	GPtrArray* items = dv->items;

	actor->scrollable.y1 = - SCROLLBAR->scroll->value * row_height;
	actor->scrollable.y2 = actor->scrollable.y1 + (items->len + 1) * row_height;

	HIDE_ITEM(row - 1);
	HIDE_ITEM(SCROLLBAR->scroll->value + N_ROWS_VISIBLE(actor));

	int last = MIN(items->len, SCROLLBAR->scroll->value + N_ROWS_VISIBLE(actor));
	for(int r = SCROLLBAR->scroll->value; r < last ; r++){
		WavViewItem* vitem = items->pdata[r];
		if(vitem->wav){
			wf_actor_set_rect((WaveformActor*)vitem->wav, &(WfRectangle){0, row_height0 + r * row_height + row_height0, agl_actor__width(actor) - 20, wav_height});
		}
	}

	agl_actor__invalidate(actor);
}


static guint
create_icon(const char* name, GdkPixbuf* pixbuf)
{
	g_return_val_if_fail(pixbuf, 0);

	guint textures[1];
	glGenTextures(1, textures);

	dbg(2, "icon: pixbuf=%ix%i %ibytes/px", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf), gdk_pixbuf_get_n_channels(pixbuf));
	glBindTexture   (GL_TEXTURE_2D, textures[0]);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D    (GL_TEXTURE_2D, 0, GL_RGBA, gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf), 0, GL_RGBA, GL_UNSIGNED_BYTE, gdk_pixbuf_get_pixels(pixbuf));
	gl_warn("texture bind");
	g_object_unref(pixbuf);

	return textures[0];
}


static guint
get_icon(const char* name, GdkPixbuf* pixbuf)
{
	guint t = GPOINTER_TO_INT(g_hash_table_lookup(icon_textures, name));
	if(!t){
		t = create_icon(name, pixbuf);
		g_hash_table_insert(icon_textures, (gpointer)name, GINT_TO_POINTER(t));

	}
	return t;
}

