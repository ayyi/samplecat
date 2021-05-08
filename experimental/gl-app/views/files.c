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
#define __wf_private__
#include "config.h"
#include "agl/behaviours/key.h"
#include "actors/scrollbar.h"
#include "debug/debug.h"
#include "file_manager/file_manager.h"
#include "file_manager/pixmaps.h"
#include "icon/utils.h"
#include "samplecat.h"
#include "application.h"
#include "views/files.impl.h"
#include "views/files.h"

#define _g_free0(var) (var = (g_free (var), NULL))

#define FONT              "Droid Sans"
#define ROW_HEIGHT        20
#define header_height     row_height
#define RHS_PADDING       20 // dont draw under scrollbar
#define N_ROWS_VISIBLE(A) ((int)(agl_actor__height(((AGlActor*)A))) / ((FilesView*)((AGlActor*)A)->parent)->row_height)
#define scrollable_height (view->view->items->len)
#define max_scroll(VIEW)  ((scrollable_height - N_ROWS_VISIBLE(VIEW->filelist) + 2) * (VIEW)->row_height)
#define SCROLLBAR         ((ScrollbarActor*)((FilesView*)actor)->scrollbar)
#define KEYS(A)           ((KeyBehaviour*)A->behaviours[0])
#define SELECTABLE(A)     ((SelectBehaviour*)(A)->behaviours[1])
#define PATH              (FILES_STATE((AGlActor*)view)->params->params[0].val.c)

static void files_free (AGlActor*);

static AGl* agl = NULL;
static int instance_count = 0;
static int col[] = {0, 24, 260, 360, 400, 440};
static AGlActorClass actor_class = {0, "Files", (AGlActorNew*)files_view, files_free};

static bool files_scan_dir  (AGlActor*);
static void files_on_scroll (AGlObservable*, AGlVal row, gpointer view);
static void files_on_select (AGlObservable*, AGlVal row, gpointer);

static AGlActor* filelist_view (void*);

static ActorKeyHandler
	files_nav_up,
	files_nav_down,
	files_page_up,
	files_page_down,
	files_nav_home,
	files_nav_end;

static ActorKey keys[] = {
	{XK_Up,        files_nav_up},
	{XK_Down,      files_nav_down},
	{XK_Page_Up,   files_page_up},
	{XK_Page_Down, files_page_down},
	{XK_Home,      files_nav_home},
	{XK_End,       files_nav_end},
	{0,}
};


AGlActorClass*
files_view_get_class ()
{
	if (!agl) {
		agl = agl_get_instance();

		agl_actor_class__add_behaviour(&actor_class, key_get_class());

		dir_init();
	}
	return &actor_class;
}


void
files_add_behaviours (FilesView* view)
{
	AGlActor* actor = (AGlActor*)view;

	KEYS(actor)->keys = &keys;

	actor->behaviours[1] = selectable();
	SELECTABLE(actor)->on_select = files_on_select;
	agl_behaviour_init((AGlBehaviour*)SELECTABLE(actor), (AGlActor*)view);

	void set_path (AGlActor* actor, const char* path)
	{
		g_idle_add((GSourceFunc)files_scan_dir, (gpointer)actor);
	}

	actor->behaviours[2] = state();
	StateBehaviour* state = FILES_STATE(actor);
	#define N_PARAMS 1
	state->params = g_malloc(sizeof(ParamArray) + N_PARAMS * sizeof(ConfigParam));
	state->params->size = N_PARAMS;
	state->params->params[0] = (ConfigParam){
		.name = "path",
		.utype = G_TYPE_STRING,
		.set.c = set_path
	};
	agl_behaviour_init(actor->behaviours[2], (AGlActor*)view);
}


AGlActor*
files_view (gpointer _)
{
	instance_count++;

	files_view_get_class();

	bool files_paint (AGlActor* actor)
	{
		char* col_heads[] = {"Filename", "Size", "Owner", "Group"};

		for (int c=0;c<G_N_ELEMENTS(col_heads);c++) {
			agl_push_clip(0.f, 0.f, MIN(col[c + 2] - 6, agl_actor__width(actor) - RHS_PADDING), 1000000.);
			agl_print(col[c + 1], 0, 0, STYLE.text, col_heads[c]);
			agl_pop_clip();
		}


		return true;
	}

	void files_init (AGlActor* a)
	{
		FilesView* view = (FilesView*)a;

		g_idle_add((GSourceFunc)files_scan_dir, a);

		void files_on_row_add (GtkTreeModel* tree_model, GtkTreePath* path, GtkTreeIter* iter, AGlActor* actor)
		{
			files_on_scroll(SELECTABLE(actor)->observable, SELECTABLE(actor)->observable->value, actor);
			agl_actor__invalidate (actor);
		}
		g_signal_connect(view->viewmodel, "row-inserted", (GCallback)files_on_row_add, a);
	}

	void files_layout (AGlActor* actor)
	{
		FilesView* view = (FilesView*)actor;

		if(view->scroll->value.i > max_scroll(view)){
			agl_observable_set_int (view->scroll, max_scroll(view));
		}
	}

	FilesView* view = agl_actor__new (FilesView,
		.actor = {
			.class = &actor_class,
			.name = "Files",
			.colour = 0x66ff99ff,
			.init = files_init,
			.paint = files_paint,
			.set_size = files_layout,
		},
		.viewmodel = vm_directory_new(),
		.scroll = agl_observable_new(),
		.row_height = ROW_HEIGHT
	);
	AGlActor* actor = (AGlActor*)view;

	files_add_behaviours(view);

	view->viewmodel->view = (ViewIface*)(view->view = directory_view_new(view->viewmodel, view));

	agl_actor__add_child (actor, view->filelist = filelist_view (actor));
	agl_actor__add_child (actor, view->scrollbar = scrollbar_view (view->filelist, GTK_ORIENTATION_VERTICAL, view->scroll, NULL, ROW_HEIGHT));

	agl_observable_subscribe (view->scroll, files_on_scroll, view);

	return (AGlActor*)view;
}


static void
files_free (AGlActor* actor)
{
	FilesView* view = (FilesView*)actor;

	g_object_unref(view->view);

	if(!--instance_count){
	}
}


const char*
files_view_get_path (FilesView* view)
{
	return PATH;
}


void
files_view_set_path (FilesView* view, const char* path)
{
	char** val = &FILES_STATE((AGlActor*)view)->params->params[0].val.c;
	if(*val) g_free(*val);
	*val = g_strdup(path);

	g_idle_add((GSourceFunc)files_scan_dir, (gpointer)view);
}


static bool
files_scan_dir (AGlActor* a)
{
	FilesView* view = (FilesView*)a;

	vm_directory_set_path(view->viewmodel, PATH && strlen(PATH) ? PATH : g_get_home_dir());
	agl_actor__invalidate (a);

	return G_SOURCE_REMOVE;
}


static void
files_on_scroll (AGlObservable* observable, AGlVal val, gpointer _view)
{
	FilesView* view = (FilesView*)_view;
	AGlActor* actor = view->filelist;
	DirectoryView* dv = view->view;
	GPtrArray* items = dv->items;

	#define EMPTY_ROWS 1 // Allow some empty space at the bottom
	g_return_if_fail(val.i <= items->len - ((int)N_ROWS_VISIBLE(view->filelist)) + EMPTY_ROWS);

	actor->scrollable.y1 = - val.i * view->row_height;
	actor->scrollable.y2 = actor->scrollable.y1 + (items->len + 1) * view->row_height;

	agl_actor__invalidate (actor);
}


static void
files_on_select (AGlObservable* o, AGlVal row, gpointer _actor)
{
	AGlActor* actor = _actor;
	FilesView* files = (FilesView*)_actor;
	DirectoryView* dv = files->view;
	GPtrArray* items = dv->items;
	AGlObservable* scroll = files->scroll;

	if (row.i > -1 && row.i < items->len && row.i != dv->selection) {
		dv->selection = row.i;

		if (row.i > scroll->value.i + N_ROWS_VISIBLE(files->filelist) - 2) {
			agl_observable_set_int (scroll, row.i - (N_ROWS_VISIBLE(files->filelist) - 2));
		}
		else if (row.i < scroll->value.i + 1){
			agl_observable_set_int (scroll, row.i - 1);
		}

		VIEW_IFACE_GET_CLASS((ViewIface*)files->view)->set_selected((ViewIface*)files->view, &(ViewIter){.i = row.i}, true);

		agl_actor__invalidate (actor);
	}
}


static bool
files_nav (AGlActor* actor, int offset)
{
	FilesView* files = (FilesView*)actor;
	DirectoryView* dv = files->view;
	GPtrArray* items = dv->items;
	AGlObservable* observable = SELECTABLE(actor)->observable;

	agl_observable_set_int (observable, CLAMP(observable->value.i + offset, 0, (int)items->len - 1));

	return AGL_HANDLED;
}


static bool
files_nav_up (AGlActor* actor, GdkModifierType modifier)
{
	return files_nav (actor, -1);
}


static bool
files_nav_down (AGlActor* actor, GdkModifierType modifier)
{
	return files_nav (actor, 1);
}


static bool
files_page_up (AGlActor* actor, GdkModifierType modifier)
{
	return files_nav (actor, -N_ROWS_VISIBLE(((FilesView*)actor)->filelist));
}


static bool
files_page_down (AGlActor* actor, GdkModifierType modifier)
{
	return files_nav (actor, N_ROWS_VISIBLE(((FilesView*)actor)->filelist));
}


static bool
files_nav_home (AGlActor* actor, GdkModifierType modifier)
{
	return files_nav (actor, -10000);
}


static bool
files_nav_end (AGlActor* actor, GdkModifierType modifier)
{
	return files_nav (actor, 10000);
}


int
files_view_row_at_coord (FilesView* view, int x, int y)
{
	y += view->scroll->value.i;
	if(y < 0) return -1;

	int r = y / view->row_height;
	GPtrArray* items = view->view->items;

	if(r > items->len) return -1;

	return r;
}


static AGlActor*
filelist_view (void* _)
{
	bool filelist_paint (AGlActor* actor)
	{
		FilesView* view = (FilesView*)actor->parent;
		DirectoryView* dv = view->view;
		GPtrArray* items = dv->items;

		int y0 = -actor->scrollable.y1;

		if (!items->len)
			return agl_print(col[1], y0, 0, STYLE.text, "No files"), true;

		int n_rows = N_ROWS_VISIBLE(actor);

		int scroll_offset = view->scroll->value.i;
		int i, r; for (i = scroll_offset; r = i - scroll_offset, i < items->len && (i - scroll_offset < n_rows); i++) {
			if (r == view->view->selection - scroll_offset) {
				PLAIN_COLOUR2 (agl->shaders.plain) = STYLE.selection;
				agl_use_program (agl->shaders.plain);
				agl_rect_ ((AGlRect){0, y0 + r * ROW_HEIGHT - 2, agl_actor__width(actor), ROW_HEIGHT});
			}

			ViewItem* vitem = items->pdata[i];
			DirItem* item = vitem->item;
			char size[16] = {'\0'}; snprintf(size, 15, "%zu", item->size);
			const char* val[] = {item->leafname, size, user_name(item->uid), group_name(item->gid)};
			for(int c=0;c<G_N_ELEMENTS(val);c++){
				agl_push_clip (0, y0, MIN(col[c + 2] - 6, actor->region.x2), actor->region.y2);
				agl_print (col[c + 1], y0 + r * ROW_HEIGHT, 0, STYLE.text, val[c]);
				agl_pop_clip ();
			}

			if (item->mime_type) {
				guint t = get_icon_texture_by_mimetype (item->mime_type);

				agl_use_program ((AGlShader*)agl->shaders.texture);
				agl_textured_rect (t, 0, y0 + r * ROW_HEIGHT, 16, 16, NULL);
			}
		}
		return true;
	}

	void filelist_init (AGlActor* actor)
	{
#ifdef AGL_ACTOR_RENDER_CACHE
		actor->fbo = agl_fbo_new (agl_actor__width(actor), agl_actor__height(actor) + 40, 0, AGL_FBO_HAS_STENCIL);
		actor->cache.enabled = true;
		actor->cache.size_request = (AGliPt){agl_actor__width(actor), agl_actor__height(actor) + 40};
#endif
	}

	void filelist_set_size (AGlActor* actor)
	{
		actor->region = (AGlfRegion){
			.x2 = agl_actor__width(actor->parent),
			.y1 = ROW_HEIGHT,
			.y2 = agl_actor__height (actor->parent)
		};
	}

	bool filelist_event (AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		FilesView* view = (FilesView*)actor->parent;

		switch (event->type) {
			case GDK_BUTTON_PRESS:
				switch (event->button.button) {
					case 4:
						dbg(1, "! scroll up");
						agl_observable_set_int (view->scroll, view->scroll->value.i - 1);
						break;
					case 5:
						dbg(1, "! scroll down");
						if (scrollable_height > N_ROWS_VISIBLE(actor)) {
							if (view->scroll->value.i < max_scroll(view))
								agl_observable_set_int (view->scroll, view->scroll->value.i + 1);
						}
						break;
				}
				return AGL_HANDLED;

			case GDK_BUTTON_RELEASE:
				;int row = files_view_row_at_coord (view, 0, xy.y);
				dbg(1, "RELEASE button=%i y=%i row=%i", event->button.button, xy.y - actor->region.y1, row);
				switch (event->button.button) {
					case 1:
						agl_observable_set_int (SELECTABLE((AGlActor*)view)->observable, row);
				}
				return AGL_HANDLED;
			default:
				break;
		}
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
