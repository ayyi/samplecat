/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2016-2021 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
*/

#include "config.h"
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include "agl/ext.h"
#include "agl/utils.h"
#include "agl/behaviours/key.h"
#include "agl/text/renderer.h"
#include "actors/scrollbar.h"
#include "waveform/actor.h"
#include "waveform/text.h"
#include "debug/debug.h"
#include "icon/utils.h"
#include "file_manager/file_manager.h"
#include "file_manager/pixmaps.h"
#include "samplecat.h"
#include "application.h"
#include "views/files.impl.h"
#include "views/files.with_wav.h"

#define _g_free0(var) (var = (g_free (var), NULL))

#define row_height0 22
#define wav_height 30
#define row_spacing 8
#define ROW_HEIGHT        (row_height0 + wav_height + row_spacing)
#define N_ROWS_VISIBLE(A) ((int)agl_actor__height(((AGlActor*)A)) / ROW_HEIGHT)
#define scrollable_height (FILES->view->items->len)
#define max_scroll(VIEW)  (scrollable_height - N_ROWS_VISIBLE((VIEW)->filelist) + 2)
#define SELECTABLE(A)     ((SelectBehaviour*)(A)->behaviours[1])

#define FILES ((FilesView*)view)

extern void files_add_behaviours (FilesView*);

static void files_free (AGlActor*);

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Files", (AGlActorNew*)files_with_wav, files_free};
static GHashTable* icon_textures = NULL;
static int col[] = {0, 24, 260, 360, 490, 530, 575};

static gboolean files_scan_dir           (AGlActor*);
static void     files_with_wav_on_scroll (AGlObservable*, AGlVal row, gpointer view);
static bool     not_audio                (const char* path);
static AGlActor*filelist_view            (void*);


AGlActorClass*
files_with_wav_get_class ()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();

		icon_textures = g_hash_table_new(NULL, NULL);
		agl_set_font_string("Roboto 10"); // initialise the pango context

		agl_actor_class__add_behaviour(&actor_class, key_get_class());

		dir_init();

		init_done = true;
	}

	return &actor_class;
}


AGlActor*
files_with_wav (gpointer _)
{
	instance_count++;

	files_with_wav_get_class();

	bool files_paint (AGlActor* actor)
	{
		char* col_heads[] = {"Filename", "Size", "Date", "Owner", "Group"};
		int sort_column = 1;

		PLAIN_COLOUR2 (agl->shaders.plain) = 0x222222ff;
		agl_use_program ((AGlShader*)agl->shaders.plain);
		agl_rect_ ((AGlRect){col[sort_column] -2, -2, col[sort_column + 1] - col[sort_column], row_height0 - 2});

		for (int c=0;c<G_N_ELEMENTS(col_heads);c++) {
			agl_push_clip (0.f, 0.f, MIN(col[c + 2] - 6, agl_actor__width(actor)), actor->region.y2);
			agl_print (col[c + 1], 0, 0, STYLE.text, col_heads[c]);
			agl_pop_clip ();
		}


		return true;
	}

	void files_init (AGlActor* a)
	{
		FilesWithWav* view = (FilesWithWav*)a;

#ifdef AGL_ACTOR_RENDER_CACHE
		a->fbo = agl_fbo_new(agl_actor__width(a), agl_actor__height(a), 0, AGL_FBO_HAS_STENCIL);
		a->cache.enabled = true;
#endif

		agl_observable_set_int (view->files.scroll, 0);

		g_idle_add((GSourceFunc)files_scan_dir, a);

		void files_on_row_add (GtkTreeModel* tree_model, GtkTreePath* path, GtkTreeIter* iter, AGlActor* actor)
		{
			files_with_wav_on_scroll (SELECTABLE(actor)->observable, SELECTABLE(actor)->observable->value, actor);

			agl_actor__invalidate(actor);
		}
		g_signal_connect(FILES->viewmodel, "row-inserted", (GCallback)files_on_row_add, a);

		void files_on_row_change (GtkTreeModel* tree_model, GtkTreePath* path, GtkTreeIter* iter, AGlActor* actor)
		{
#if 0
			FilesWithWav* view = (FilesWithWav*)actor;
			DirectoryView* dv = FILES->view;
			GPtrArray* items = dv->items;

			int r = GPOINTER_TO_INT(iter->user_data);
			if(r < 5){
				WavViewItem* vitem = items->pdata[r];
				DirItem* item = vitem->item;
				char* name = item->leafname;
			}
#endif
		}
		g_signal_connect (FILES->viewmodel, "row-changed", (GCallback)files_on_row_change, a);
	}

	void files_set_size (AGlActor* actor)
	{
		FilesWithWav* view = (FilesWithWav*)actor;

		if (view->files.scroll->value.i > max_scroll((FilesView*)view)) {
			agl_observable_set_int (view->files.scroll, max_scroll((FilesView*)view));
		}
	}

	bool files_event (AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		FilesWithWav* view = (FilesWithWav*)actor;

		switch (event->type) {
			case GDK_BUTTON_PRESS:
				switch (event->button.button) {
					case 4:
						dbg(1, "! scroll up");
						agl_observable_set_int (view->files.scroll, MAX(0, view->files.scroll->value.i - 1));
						break;
					case 5:
						dbg(1, "! scroll down");
						if(scrollable_height > N_ROWS_VISIBLE(actor)){
							if(view->files.scroll->value.i < max_scroll((FilesView*)view))
								agl_observable_set_int (view->files.scroll, view->files.scroll->value.i + 1);
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

	FilesWithWav* view = agl_actor__new (FilesWithWav,
		.files = {
			.actor = {
				.class = &actor_class,
				.name = "Files",
				.colour = 0x66ff99ff,
				.init = files_init,
				.paint = files_paint,
				.set_size = files_set_size,
				.on_event = files_event,
			},
			.viewmodel = vm_directory_new(),
			.scroll = agl_observable_new(),
			.row_height = ROW_HEIGHT
		}
	);
	AGlActor* actor = (AGlActor*)view;

	files_add_behaviours((FilesView*)view);

	FILES->viewmodel->view = (ViewIface*)(FILES->view = directory_view_new(FILES->viewmodel, (FilesView*)view));

	agl_actor__add_child (actor, view->files.filelist = filelist_view (actor));
	agl_actor__add_child (actor, view->files.scrollbar = scrollbar_view (view->files.filelist, GTK_ORIENTATION_VERTICAL, view->files.scroll, NULL, ROW_HEIGHT));

	agl_observable_subscribe (view->files.scroll, files_with_wav_on_scroll, view);

	return (AGlActor*)view;
}


static void
files_free (AGlActor* actor)
{
	FilesWithWav* view = (FilesWithWav*)actor;

	g_object_unref(FILES->view);

	if (!--instance_count) {
	}
}


void
files_with_wav_set_path (FilesWithWav* view, const char* path)
{
	char** val = &FILES_STATE((AGlActor*)view)->params->params[0].val.c;
	if (*val) g_free(*val);
	*val = g_strdup(path);

	g_idle_add((GSourceFunc)files_scan_dir, (gpointer)view);
}


static gboolean
files_scan_dir (AGlActor* a)
{
	FilesWithWav* view = (FilesWithWav*)a;

	char* path = FILES_STATE((AGlActor*)view)->params->params[0].val.c;
	vm_directory_set_path(FILES->viewmodel, path ? path : g_get_home_dir());
	agl_actor__invalidate(a);

	return G_SOURCE_REMOVE;
}


int
files_with_wav_row_at_coord (FilesWithWav* view, int x, int y)
{
	#define header_height 20

	y += view->files.scroll->value.i * ROW_HEIGHT - header_height;
	if (y < 0) return -1;
	int r = y / ROW_HEIGHT;
	GPtrArray* items = FILES->view->items;
	if(r > items->len) return -1;
	return r;
}


void
files_with_wav_select (FilesWithWav* view, int row)
{
	DirectoryView* dv = FILES->view;

	if (row > -1 && row < FILES->view->items->len && row != view->files.view->selection) {
		GPtrArray* items = dv->items;
		{
			WavViewItem* vitem = items->pdata[view->files.view->selection];
			if(vitem->wav){
				wf_actor_set_colour((WaveformActor*)vitem->wav, 0x66ff66ff);
			}
		}

		view->files.view->selection = row;

		WavViewItem* vitem = items->pdata[row];
		if (vitem->wav) {
			wf_actor_set_colour((WaveformActor*)vitem->wav, 0xff6666ff);
		}

		iRange range = {view->files.scroll->value.i, view->files.scroll->value.i + N_ROWS_VISIBLE(view) - 1};
		if (row > range.end) {
			agl_observable_set_int (view->files.scroll, row - N_ROWS_VISIBLE(view) + 1);
		}
		if (row < range.start) {
			agl_observable_set_int (view->files.scroll, row);
		}

		agl_actor__invalidate ((AGlActor*)view);
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
files_with_wav_on_scroll (AGlObservable* observable, AGlVal row, gpointer _view)
{
	FilesWithWav* view = (FilesWithWav*)_view;
	AGlActor* actor = view->files.filelist;
	DirectoryView* dv = FILES->view;
	GPtrArray* items = dv->items;

	actor->scrollable.y1 = - row.i * ROW_HEIGHT;
	actor->scrollable.y2 = actor->scrollable.y1 + (items->len + 1) * ROW_HEIGHT;

	HIDE_ITEM(row.i - 1);
	HIDE_ITEM(view->files.scroll->value.i + N_ROWS_VISIBLE(actor));

	int last = MIN(items->len, view->files.scroll->value.i + N_ROWS_VISIBLE(actor));
	for (int r = view->files.scroll->value.i; r < last ; r++) {
		WavViewItem* vitem = items->pdata[r];
		if (vitem->wav) {
			wf_actor_set_rect((WaveformActor*)vitem->wav, &(WfRectangle){0, row_height0 + r * ROW_HEIGHT + row_height0, agl_actor__width(actor) - 20, wav_height});
		}
	}

	agl_actor__invalidate(actor);
}


static bool
not_audio (const char* path)
{
	static char* types[] = {".pdf", ".jpg", ".png", ".txt"};

	for (int i=0;i<G_N_ELEMENTS(types);i++) {
		if (g_str_has_suffix(path, types[i]))
			return true;
	}

	return false;
}


static AGlActor*
filelist_view (void* _)
{
	bool filelist_paint (AGlActor* actor)
	{
		FilesWithWav* view = (FilesWithWav*)actor->parent;
		DirectoryView* dv = FILES->view;
		GPtrArray* items = dv->items;

		int n_rows = N_ROWS_VISIBLE(actor);

		if (!items->len)
			return agl_print(0, 0, 0, STYLE.text, "No files"), true;

		int offset = view->files.scroll->value.i;
		int i, r; for (i = offset; r = i - offset, i < items->len && (i - offset < n_rows); i++) {
			int y = row_height0 + i * ROW_HEIGHT;
			int y2 = row_height0 + i * ROW_HEIGHT;
			if (r == FILES->view->selection - offset) {
				PLAIN_COLOUR2 (agl->shaders.plain) = STYLE.selection;
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_rect_((AGlRect){0, y - 2, agl_actor__width(actor) - 20, row_height0 + wav_height + 4});
			} else {
				// waveform background
				PLAIN_COLOUR2 (agl->shaders.plain) = STYLE.bg_alt;
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_rect_((AGlRect){0, y + row_height0, agl_actor__width(actor) - 20, wav_height});
			}

			WavViewItem* vitem = items->pdata[i];
			DirItem* item = vitem->item;
			char size[16] = {'\0'}; snprintf(size, 15, "%zu", item->size);
			const char* val[] = {item->leafname, size, pretty_time(&item->mtime), user_name(item->uid), group_name(item->gid)};
			for (int c=0;c<G_N_ELEMENTS(val);c++) {
				agl_push_clip(0, 0, MIN(col[c + 2] - 6, agl_actor__width(actor)), 100000.);
				agl_print(col[c + 1], y2, 0, (STYLE.text & 0xffffff00) + (c ? 0xaa : 0xff), val[c]);
				agl_pop_clip();
			}

			if (!vitem->wav) {
				DirItem* item = vitem->item;
				char* name = item->leafname;

				char* path = g_strdup_printf("%s/%s", files_view_get_path(FILES), name);
				if (!not_audio(path)) {
					WaveformActor* wa = wf_context_add_new_actor (view->wfc, NULL);
					agl_actor__add_child(actor, (AGlActor*)wa);

					Waveform* waveform = waveform_new(path);
					wf_actor_set_waveform(wa, waveform, NULL, NULL);
					wf_actor_set_colour(wa, STYLE.fg);
					wf_actor_set_rect(wa, &(WfRectangle){0, y + row_height0, agl_actor__width(actor) - 20, wav_height});
					vitem->wav = (AGlActor*)wa;
				} else {
					g_free(path);
				}
			}

			// TODO dont do this in paint
			if(item->mime_type){
				guint t = get_icon_texture_by_mimetype(item->mime_type);
				if(t){
					agl->shaders.texture->uniform.fg_colour = 0xffffffff;
					agl_use_program((AGlShader*)agl->shaders.texture);
					agl_textured_rect(t, 0, y, 16, 16, NULL);
				}else{
					pwarn("failed to get icon for %s", item->mime_type->subtype);
				}
			}

			g_free((char*)val[2]);
		}
		return true;
	}

	void filelist_init (AGlActor* actor)
	{
		// currently, only the outer actor is cached. not optimised.
#if 0
#ifdef AGL_ACTOR_RENDER_CACHE
		actor->fbo = agl_fbo_new (agl_actor__width(actor), agl_actor__height(actor) + 40, 0, AGL_FBO_HAS_STENCIL);
		actor->cache.enabled = true;
		actor->cache.size_request = (AGliPt){agl_actor__width(actor), agl_actor__height(actor) + 40};
#endif
#endif
	}

	void filelist_set_size (AGlActor* actor)
	{
		actor->region = (AGlfRegion){
			.x2 = agl_actor__width(actor->parent),
			.y1 = header_height,
			.y2 = agl_actor__height (actor->parent)
		};
	}

	bool filelist_event (AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		return AGL_NOT_HANDLED;
	}

	return agl_actor__new (AGlActor,
		.name = "Filelist",
		.init = filelist_init,
		.set_size = filelist_set_size,
		.paint = filelist_paint,
		.on_event = filelist_event,
	);
}
