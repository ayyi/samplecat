/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2016-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <GL/gl.h>
#include "agl/ext.h"
#include "agl/utils.h"
#include "agl/event.h"
#include "agl/behaviours/key.h"
#include "agl/behaviours/cache.h"
#include "agl/behaviours/selectable.h"
#include "agl/text/renderer.h"
#include "actors/scrollbar.h"
#include "waveform/actor.h"
#include "waveform/text.h"
#include "debug/debug.h"
#include "icon/utils.h"
#include "file_manager/file_manager.h"
#include "file_manager/pixmaps.h"
#include "application.h"
#include "views/files.impl.h"
#include "views/files.with_wav.h"

#define row_height0 22
#define wav_height 30
#define row_spacing 8
#define ROW_HEIGHT        (row_height0 + wav_height + row_spacing)
#define N_ROWS_VISIBLE(A) ((int)agl_actor__height(((AGlActor*)A)) / ROW_HEIGHT)
#define POSITION(I)       (view->positions.vals[I].val.i)
#define scrollable_height (FILES->view->items->len)
#define max_scroll(VIEW)  (scrollable_height - N_ROWS_VISIBLE((VIEW)->filelist) + 2)
#define KEYS(A)           ((KeyBehaviour*)A->behaviours[0])
#define SELECTABLE(A)     ((SelectBehaviour*)(A)->behaviours[1])

#define FILES ((FilesView*)view)

extern AGl _agl;
extern void files_add_behaviours (FilesView*);

const char* col_heads[] = {"Filename", "Size", "Date", "Owner", "Group"};

static void files_free (AGlActor*);

static int instance_count = 0;
static AGlActorClass actor_class = {0, "Files", (AGlActorNew*)files_with_wav, files_free};
static GHashTable* icon_textures = NULL;

static ActorKeyHandler
	files_nav_up,
	files_nav_down;

static ActorKey keys[] = {
	{XK_Up,        files_nav_up},
	{XK_Down,      files_nav_down},
	{0,}
};

typedef enum {
	F_COL_ICON = 0,
	F_COL_NAME,
	F_COL_SIZE,
	F_COL_DATE,
	F_COL_OWNER,
	F_COL_GROUP,
} SortColum;

static int sort_types[] = {F_COL_ICON, F_COL_NAME, 0, F_COL_DATE, F_COL_SIZE, F_COL_OWNER, F_COL_GROUP, 0};
static int col[] = {0, 24, 260, 360, 490, 530, 575};

static gboolean  files_scan_dir                (AGlActor*);
static void      files_with_wav_on_scroll      (AGlObservable*, AGlVal row, gpointer view);
static bool      not_audio                     (const char* path);
static AGlActor* filelist_view                 (void*);
static void      files_setup_visible_waveforms (FilesWithWav*);


static AGlVal
columns_xy_to_val (DragZone* obj, int zone_index, AGlActor* actor, AGliPt xy, AGliPt offset)
{
	FilesWithWav* view = (FilesWithWav*)actor;

	AGlVal val = (AGlVal){.i = xy.x - (offset.x - 2)};
	int delta = val.i - obj->val->i;

	while (zone_index < view->dzb.n_zones) {
		view->dzb.zones[zone_index].val->i += delta;
		zone_index++;
	}

	agl_actor__invalidate(view->files.filelist);

	return val;
}

static DragZoneClass dragzone_class = {
	.cursor = &_agl.cursors.h_double_arrow,
	.xy_to_val = columns_xy_to_val
};


AGlActorClass*
files_with_wav_get_class ()
{
	if (!icon_textures) {

		icon_textures = g_hash_table_new(NULL, NULL);

		agl_actor_class__add_behaviour(&actor_class, key_get_class());
		agl_actor_class__add_behaviour(&actor_class, selectable_get_class());
		agl_actor_class__add_behaviour(&actor_class, state_get_class());

		dir_init();
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
		FilesWithWav* view = (FilesWithWav*)actor;

		int sort_column = sort_types[FILES->view->sort_type];

		PLAIN_COLOUR2 (_agl.shaders.plain) = 0x222222ff;
		agl_use_program ((AGlShader*)_agl.shaders.plain);
		int left = sort_column > 1 ? POSITION(sort_column - 2) : 0;
		agl_rect_ ((AGlRect){left -2, -2, POSITION(sort_column - 1) - left, row_height0 - 2});

		int x = col[1];
		for (int c=0;c<G_N_ELEMENTS(col_heads);c++) {
			agl_push_clip (0.f, 0.f, MIN(POSITION(c) - 6, agl_actor__width(actor)), actor->region.y2);
			agl_print (x, 0, 0, STYLE.text, col_heads[c]);
			agl_pop_clip ();

			x = POSITION(c);
		}

		return true;
	}

	void files_init (AGlActor* a)
	{
		FilesWithWav* view = (FilesWithWav*)a;

		agl_observable_set_int (view->files.scroll, 0);

		g_idle_add((GSourceFunc)files_scan_dir, a);

		void files_on_row_add (GtkTreeModel* tree_model, GtkTreePath* path, GtkTreeIter* iter, AGlActor* actor)
		{
			static int idle;

			gboolean files_on_row_add_idle (void* _actor)
			{
				AGlActor* actor = _actor;
				FilesWithWav* view = (FilesWithWav*)actor;
				idle = 0;

				files_with_wav_on_scroll (SELECTABLE(actor)->observable, SELECTABLE(actor)->observable->value, actor);

				agl_actor__invalidate(view->files.filelist);

				return G_SOURCE_REMOVE;
			}
			if (idle) idle = g_source_remove(idle);
			idle = g_idle_add(files_on_row_add_idle, actor);
		}
		g_signal_connect(FILES->viewmodel, "row-inserted", (GCallback)files_on_row_add, a);

		void files_on_row_change (GtkTreeModel* tree_model, GtkTreePath* path, GtkTreeIter* iter, AGlActor* actor)
		{
			FilesWithWav* view = (FilesWithWav*)actor;

			// hide all waveforms
			for (GList* l = view->files.filelist->children; l; l = l->next)
				((AGlActor*)l->data)->region = (AGlfRegion){0};

			files_setup_visible_waveforms(view);

			agl_actor__invalidate(view->files.filelist);
		}
		g_signal_connect (FILES->viewmodel, "row-changed", (GCallback)files_on_row_change, a);
	}

	void files_set_size (AGlActor* actor)
	{
		FilesWithWav* view = (FilesWithWav*)actor;
		DirectoryView* dv = FILES->view;
		GPtrArray* items = dv->items;

		if (view->files.scroll->value.i > max_scroll((FilesView*)view)) {
			agl_observable_set_int (view->files.scroll, max_scroll((FilesView*)view));
		}

		int last = MIN(items->len, view->files.scroll->value.i + N_ROWS_VISIBLE(actor));
		for (int r = view->files.scroll->value.i; r < last; r++) {
			int y = row_height0 + r * ROW_HEIGHT;

			WavViewItem* vitem = items->pdata[r];
			if (vitem->wav) {
				wf_actor_set_rect((WaveformActor*)vitem->wav, &(WfRectangle){0, y + row_height0, agl_actor__width(actor) - 20, wav_height});
			}
		}
	}

	bool files_event (AGlActor* actor, AGlEvent* event, AGliPt xy)
	{
		FilesWithWav* view = (FilesWithWav*)actor;

		switch (event->type) {
			case AGL_BUTTON_PRESS:
				switch (event->button.button) {
					case 4:
						dbg(1, "! scroll up");
						agl_observable_set_int (view->files.scroll, MAX(0, view->files.scroll->value.i - 1));
						break;
					case 5:
						dbg(1, "! scroll down");
						if (scrollable_height > N_ROWS_VISIBLE(actor)) {
							if (view->files.scroll->value.i < max_scroll((FilesView*)view))
								agl_observable_set_int (view->files.scroll, view->files.scroll->value.i + 1);
						}
						break;
				}
				return AGL_HANDLED;
			case AGL_BUTTON_RELEASE:
				;int row = files_with_wav_row_at_coord (view, 0, xy.y - actor->region.y1);
				dbg(1, "RELEASE button=%i y=%i row=%i", event->button.button, xy.y, row);
				switch (event->button.button) {
					case 1:
						if (row < 0) {
							int column = -1;
							for (int c=0;c<G_N_ELEMENTS(col);c++) {
								if (xy.x > col[c]) {
									column = c;
								}
							}
							if (column > 0) {
								int order[] = {0, SORT_NAME, SORT_SIZE, SORT_DATE, SORT_OWNER, SORT_GROUP, 0};
								if (order[column]) {
									directory_view_set_sort (FILES->view, order[column], !FILES->view->sort_order);
									files_setup_visible_waveforms (view);
									agl_actor__invalidate (view->files.filelist);
								}
							}
						} else {
							VIEW_IFACE_GET_CLASS((ViewIface*)FILES->view)->set_selected((ViewIface*)FILES->view, &(ViewIter){.i = row}, true);
							agl_actor__invalidate(view->files.filelist);
						}
				}
				return AGL_HANDLED;
			default:
				break;
		}
		return AGL_NOT_HANDLED;
	}

	FilesWithWav* view = agl_actor__new (FilesWithWav,
		.files = {
			.actor = {
				.class = &actor_class,
				.colour = 0x66ff99ff,
				.init = files_init,
				.paint = files_paint,
				.set_size = files_set_size,
				.on_event = files_event,
			},
			.viewmodel = vm_directory_new(),
			.scroll = agl_observable_new(),
			.row_height = ROW_HEIGHT
		},
	);
	AGlActor* actor = (AGlActor*)view;

	files_add_behaviours((FilesView*)view);

	FILES->viewmodel->view = (ViewIface*)(FILES->view = directory_view_new(FILES->viewmodel, (FilesView*)view));

	view->files.filelist = agl_actor__add_child (actor, filelist_view (actor));
	view->files.scrollbar = agl_actor__add_child (actor, scrollbar_view (view->files.filelist, GTK_ORIENTATION_VERTICAL, view->files.scroll, NULL, ROW_HEIGHT));

	agl_observable_subscribe (view->files.scroll, files_with_wav_on_scroll, view);

	KEYS(actor)->keys = &keys;

	agl_actor__add_behaviour(actor, dragzone_behaviour_init (&view->dzb, (DragZoneBehaviourPrototype){
		.model = &view->positions,
		.n_zones = 5,
	}));

	am_object_init(&view->positions, 5);

	for (int i=0;i<view->dzb.n_zones;i++) {
		dragzone_init(&dragzone_class, &view->zones[i], (DragZone){
			.val = &view->positions.vals[i].val,
			.hover.text = col_heads[i],
			.region = (AGliRegion){
				.x1 = col[i + 2] - 2,
				.x2 = col[i + 2] + 2,
				.y1 = 0,
				.y2 = row_height0
			}
		});
		view->positions.vals[i].type = G_TYPE_INT;
		view->positions.vals[i].val.i = col[i + 2];
	}

	void columns_set_position (AGlActor* actor, AMObject* obj, int index, AGlVal val, ErrorHandler handler, gpointer user_data)
	{
		FilesWithWav* view = (FilesWithWav*)actor;

		while (index < view->dzb.n_zones) {
			view->dzb.zones[index].region.x1 = view->dzb.zones[index].val->i - 2;
			view->dzb.zones[index].region.x2 = view->dzb.zones[index].val->i + 2;
			index++;
		}
	}
	view->positions.set = columns_set_position;

	return (AGlActor*)view;
}


static void
files_free (AGlActor* actor)
{
	FilesWithWav* view = (FilesWithWav*)actor;

	g_object_unref(FILES->view);

	am_object_deinit(&view->positions);

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
	agl_actor__invalidate(view->files.filelist);

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
	if (r > items->len) return -1;
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
			if (vitem->wav) {
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

		agl_actor__invalidate (view->files.filelist);
	}
}


#define HIDE_ITEM(N) \
	if (N > 0 && N < items->len) { \
		WavViewItem* vitem = items->pdata[N]; \
		if (vitem->wav) { \
			vitem->wav->region = (AGlfRegion){0}; \
			wf_actor_set_rect((WaveformActor*)vitem->wav, &(WfRectangle){0,}); \
		} \
	}


static void
files_setup_visible_waveforms (FilesWithWav* view)
{
	AGlActor* actor = view->files.filelist;
	DirectoryView* dv = FILES->view;
	GPtrArray* items = dv->items;

	int offset = view->files.scroll->value.i;
	int last = MIN(items->len, offset + N_ROWS_VISIBLE(actor));
	for (int r = offset; r < last; r++) {
		WavViewItem* vitem = items->pdata[r];
		if (vitem->wav) {
			vitem->wav->region = (AGlfRegion){0}; // disable animations
			wf_actor_set_rect((WaveformActor*)vitem->wav, &(WfRectangle){0, row_height0 + r * ROW_HEIGHT + row_height0, agl_actor__width(actor) - 20, wav_height});
		} else {
			DirItem* item = vitem->item;
			char* name = item->leafname;

			char* path = g_strdup_printf("%s/%s", files_view_get_path(FILES), name);
			if (!not_audio(path)) {
				int y = row_height0 + r * ROW_HEIGHT;
				// note that because the list is initially shown unsorted, the waveforms are not created in the order you might expect
				vitem->wav = agl_actor__add_child(actor, ({
					WaveformActor* wa = wf_context_add_new_actor (view->wfc, NULL);

					Waveform* waveform = waveform_new(path);
					wf_actor_set_waveform(wa, waveform, NULL, NULL);
					wf_actor_set_colour(wa, STYLE.fg);
					wf_actor_set_rect(wa, &(WfRectangle){0, y + row_height0, agl_actor__width(actor) - 20, wav_height});

					(AGlActor*)wa;
				}));
			} else {
				g_free(path);
			}
		}
	}
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
	HIDE_ITEM(row.i + N_ROWS_VISIBLE(actor));

	files_setup_visible_waveforms(view);

	agl_actor__invalidate(actor);
}


static bool
files_nav (AGlActor* actor, int offset)
{
	FilesWithWav* view = (FilesWithWav*)actor;
	DirectoryView* dv = FILES->view;
	GPtrArray* items = dv->items;
	AGlObservable* observable = SELECTABLE(actor)->observable;

	if (agl_observable_set_int (observable, CLAMP(observable->value.i + offset, 0, (int)items->len - 1)))
		agl_actor__invalidate(view->files.filelist);

	return AGL_HANDLED;
}


static bool
files_nav_up (AGlActor* actor, AGlModifierType modifier)
{
	return files_nav (actor, -1);
}


static bool
files_nav_down (AGlActor* actor, AGlModifierType modifier)
{
	return files_nav (actor, 1);
}


static bool
not_audio (const char* path)
{
	static char* types[] = {".pdf", ".jpg", ".png", ".txt", ".nfo"};

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
		int i, r = 0; for (i = offset; r = i - offset, i < items->len && (i - offset < n_rows); i++) {
			int y = row_height0 + i * ROW_HEIGHT;
			int y2 = row_height0 + i * ROW_HEIGHT;
			if (r == FILES->view->selection - offset) {
				PLAIN_COLOUR2 (_agl.shaders.plain) = STYLE.selection;
				agl_use_program((AGlShader*)_agl.shaders.plain);
				agl_rect_((AGlRect){0, y - 2, agl_actor__width(actor) - 20, row_height0 + wav_height + 4});
			} else {
				// waveform background
				PLAIN_COLOUR2 (_agl.shaders.plain) = STYLE.bg_alt;
				agl_use_program((AGlShader*)_agl.shaders.plain);
				agl_rect_((AGlRect){0, y + row_height0, agl_actor__width(actor) - 20, wav_height});
			}

			WavViewItem* vitem = items->pdata[i];
			DirItem* item = vitem->item;
			char size[16] = {'\0'}; snprintf(size, 15, "%zu", item->size);
			const char* val[] = {item->leafname, size, pretty_time(&item->mtime), user_name(item->uid), group_name(item->gid)};
			for (int c=0;c<G_N_ELEMENTS(val);c++) {
				agl_push_clip(0, 0, MIN(POSITION(c) - 6, agl_actor__width(actor)), 100000.);
				agl_print(c > 0 ? POSITION(c - 1) : col[1], y2, 0, (STYLE.text & 0xffffff00) + (c ? 0xaa : 0xff), val[c]);
				agl_pop_clip();
			}

			// TODO dont do this in paint
			if (item->mime_type) {
				guint t = get_icon_texture_by_mimetype(item->mime_type);
				if (t) {
					_agl.shaders.texture->uniform.fg_colour = 0xffffffff;
					agl_use_program((AGlShader*)_agl.shaders.texture);
					agl_textured_rect(t, 0, y, 16, 16, NULL);
				} else {
					pwarn("failed to get icon for %s", item->mime_type->subtype);
				}
			}

			g_free((char*)val[2]);
		}
		return true;
	}

	void filelist_init (AGlActor* actor)
	{
	}

	void filelist_set_size (AGlActor* actor)
	{
		actor->region = (AGlfRegion){
			.x2 = agl_actor__width(actor->parent),
			.y1 = header_height,
			.y2 = agl_actor__height (actor->parent)
		};
	}

	bool filelist_event (AGlActor* actor, AGlEvent* event, AGliPt xy)
	{
		return AGL_NOT_HANDLED;
	}

	AGlActor* actor = agl_actor__new (AGlActor,
		.init = filelist_init,
		.name = strdup("Filelist"),
		.set_size = filelist_set_size,
		.paint = filelist_paint,
		.on_event = filelist_event,
	);

	agl_actor__add_behaviour(actor, cache_behaviour());

	return actor;
}
