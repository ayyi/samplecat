/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2017 Tim Orford <tim@orford.org>                  |
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
#include "waveform/shader.h"
#include "waveform/actors/text.h"
#include "file_manager/file_manager.h"
#include "file_manager/pixmaps.h"
#include "samplecat.h"
#include "views/scrollbar.h"
#include "views/files.impl.h"
#include "views/files.h"

#define _g_free0(var) (var = (g_free (var), NULL))

#define FONT "Droid Sans"
#define row_height 20
#define N_ROWS_VISIBLE(A) (agl_actor__height(((AGlActor*)A)) / row_height) - 1
#define scrollable_height (view->view->items->len)
#define max_scroll_offset (scrollable_height - N_ROWS_VISIBLE(actor) + 2)

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Files", (AGlActorNew*)files_view};
static GHashTable* icon_textures = NULL;

static gboolean files_scan_dir (AGlActor*);
static guint    create_icon    (const char*, GdkPixbuf*);
static guint    get_icon       (const char*, GdkPixbuf*);


AGlActorClass*
files_view_get_class ()
{
	return &actor_class;
}


static void
_init()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();
		agl_set_font_string("Roboto 10"); // initialise the pango context

		icon_textures = g_hash_table_new(NULL, NULL);

		dir_init();

		init_done = true;
	}
}


AGlActor*
files_view(WaveformActor* _)
{
	instance_count++;

	_init();

	bool files_paint(AGlActor* actor)
	{
		FilesView* view = (FilesView*)actor;
		DirectoryView* dv = view->view;
		GPtrArray* items = dv->items;

		int n_rows = N_ROWS_VISIBLE(actor);

		int col[] = {0, 24, 260, 360, 400, 440};
		char* col_heads[] = {"Filename", "Size", "Owner", "Group"};
		int y0 = -actor->scrollable.y1;
		int c; for(c=0;c<G_N_ELEMENTS(col_heads);c++){
			agl_enable_stencil(0, y0, col[c + 2] - 6, actor->region.y2);
			agl_print(col[c + 1], y0, 0, 0xffffffff, col_heads[c]);
		}

		if(!items->len)
			return agl_print(0, 0, 0, 0xffffffff, "No files"), true;

		int y = row_height - actor->scrollable.y1;
		int i, r; for(i = view->scroll_offset; r = i - view->scroll_offset, i < items->len && (i - view->scroll_offset < n_rows); i++){
			if(r == view->view->selection - view->scroll_offset){
				agl->shaders.plain->uniform.colour = 0x6677ff77;
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_rect_((AGlRect){0, y + r * row_height - 2, agl_actor__width(actor), row_height});
			}

			ViewItem* vitem = items->pdata[i];
			DirItem* item = vitem->item;
			//dbg(0, "  %i: %zu %s", i, item->size, item->leafname);
			char size[16] = {'\0'}; snprintf(size, 15, "%zu", item->size);
			const char* val[] = {item->leafname, size, user_name(item->uid), group_name(item->gid)};
			int c; for(c=0;c<G_N_ELEMENTS(val);c++){
				agl_enable_stencil(0, y0, col[c + 2] - 6, actor->region.y2);
				agl_print(col[c + 1], y + r * row_height, 0, 0xffffffff, val[c]);
			}

			if(item->mime_type){
				GdkPixbuf* pixbuf = mime_type_get_pixbuf(item->mime_type);
				guint t = get_icon(item->mime_type->subtype, pixbuf);
				agl_use_program((AGlShader*)agl->shaders.texture);
				agl_textured_rect(t, 0, y + r * row_height, 16, 16, NULL);
			}
		}

		agl_disable_stencil();

		return true;
	}

	void files_init(AGlActor* a)
	{
		FilesView* view = (FilesView*)a;

#ifdef XXAGL_ACTOR_RENDER_CACHE
		a->fbo = agl_fbo_new(agl_actor__width(a), agl_actor__height(a), 0, AGL_FBO_HAS_STENCIL);
		a->cache.enabled = true;
#endif
		g_idle_add((GSourceFunc)files_scan_dir, a);

		void files_on_row_add (GtkTreeModel* tree_model, GtkTreePath* path, GtkTreeIter* iter, AGlActor* actor)
		{
			agl_actor__invalidate(actor);
		}
		g_signal_connect(view->viewmodel, "row-inserted", (GCallback)files_on_row_add, a);
	}

	void files_set_scroll_position(AGlActor* actor, int scroll_offset)
	{
		FilesView* view = (FilesView*)actor;
		DirectoryView* dv = view->view;
		GPtrArray* items = dv->items;

		scroll_offset = MAX(0, scroll_offset); // TODO why doesnt CLAMP do this?

		view->scroll_offset = CLAMP(scroll_offset, 0, max_scroll_offset);
		actor->scrollable.y1 = - view->scroll_offset * row_height;
		actor->scrollable.y2 = actor->scrollable.y1 + (items->len + 1) * row_height;
	}

	void files_set_size(AGlActor* actor)
	{
		FilesView* view = (FilesView*)actor;
		view->scroll_offset = MIN(view->scroll_offset, max_scroll_offset);
		files_set_scroll_position(actor, view->scroll_offset);
	}

	bool files_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		FilesView* view = (FilesView*)actor;

		switch(event->type){
			case GDK_BUTTON_PRESS:
				switch(event->button.button){
					case 4:
						dbg(0, "! scroll up");
						files_set_scroll_position(actor, view->scroll_offset - 1);
						agl_actor__invalidate(actor);
						break;
					case 5:
						dbg(0, "! scroll down");
						if(scrollable_height > N_ROWS_VISIBLE(actor)){
							files_set_scroll_position(actor, view->scroll_offset + 1);
							agl_actor__invalidate(actor);
						}
						break;
				}
				break;
			case GDK_BUTTON_RELEASE:
				;int row = files_view_row_at_coord (view, 0, xy.y - actor->region.y1);
				dbg(1, "RELEASE button=%i y=%i row=%i", event->button.button, xy.y - actor->region.y1, row);
				switch(event->button.button){
					case 1:
						VIEW_IFACE_GET_CLASS((ViewIface*)view->view)->set_selected((ViewIface*)view->view, &(ViewIter){.i = row}, true);
				}
				break;
			default:
				break;
		}
		return AGL_HANDLED;
	}

	void files_free(AGlActor* actor)
	{
		FilesView* view = (FilesView*)actor;

		g_object_unref(view->view);

		if(!--instance_count){
		}
	}

	FilesView* view = WF_NEW(FilesView,
		.actor = {
			.class = &actor_class,
			.name = "Files",
			.init = files_init,
			.free = files_free,
			.paint = files_paint,
			.set_size = files_set_size,
			.on_event = files_event,
		},
		.viewmodel = vm_directory_new()
	);

	view->viewmodel->view = (ViewIface*)(view->view = directory_view_new(view->viewmodel, view));

	agl_actor__add_child((AGlActor*)view, scrollbar_view(NULL, GTK_ORIENTATION_VERTICAL));

	return (AGlActor*)view;
}


static gboolean
files_scan_dir(AGlActor* a)
{
	//dir_rescan(((FilesView*)a)->model);
	vm_directory_set_path(((FilesView*)a)->viewmodel, g_get_home_dir());
	agl_actor__invalidate(a);
	return G_SOURCE_REMOVE;
}


int
files_view_row_at_coord (FilesView* view, int x, int y)
{
	#define header_height 20
	y += view->scroll_offset * row_height - header_height;
	if(y < 0) return -1;
	int r = y / row_height;
	GPtrArray* items = view->view->items;
	if(r > items->len) return -1;
	return r;
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

