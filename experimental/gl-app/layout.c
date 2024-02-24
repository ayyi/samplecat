/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <glib/gstdio.h>
#include <debug/debug.h>
#include "agl/actor.h"
#include "waveform/actor.h"
#include "yaml/load.h"
#include "yaml/save.h"
#include "application.h"
#include "behaviours/state.h"
#include "behaviours/panel.h"
#include "layout.h"

typedef AGlActorClass* (get_class)();

get_class dock_v_get_class, dock_h_get_class, scrollable_view_get_class, list_view_get_class, files_view_get_class, directories_view_get_class, inspector_view_get_class, search_view_get_class, filters_view_get_class, player_view_get_class, scrollbar_view_get_class, button_get_class
#ifdef HAVE_FFTW3
	, spectrogram_view_get_class
#endif
	;
#include "views/dock_v.h"
#include "views/tabs.h"

GHashTable* agl_actor_registry; // maps className to AGlActorClass


static void
agl_actor_register_class (const char* name, AGlActorClass* class)
{
	static int i = 0;
	class->type = ++i;
	g_hash_table_insert(agl_actor_registry, (char*)name, class);
}


static bool
config_load_point (yaml_parser_t* parser, const char*, gpointer _pt)
{
	AGliPt* pt = _pt;
	g_auto(yaml_event_t) event;

	for (int i=0;i<2;i++) {
		if (yaml_parser_parse(parser, &event)) {
			switch (event.type) {
				case YAML_SCALAR_EVENT:
					;int* a = i ? &pt->y : &pt->x;
					*a = atoi((char*)event.data.scalar.value);
					break;
				default:
					break;
			}
		}
		yaml_event_delete(&event);
	}

	get_expected_event(parser, &event, YAML_SEQUENCE_END_EVENT);

	return true;
}


static bool
config_load_size (yaml_parser_t* parser, const char*, gpointer _actor)
{
	AGlActor* actor = _actor;

	AGliPt pt = {0};
	config_load_point(parser, NULL, &pt);

	actor->region.x2 = actor->region.x1 + pt.x;
	actor->region.y2 = actor->region.y1 + pt.y;

	return true;
}


static bool
config_load_position (yaml_parser_t* parser, const char*, gpointer _actor)
{
	AGlActor* actor = _actor;

	AGliPt pt = {0};
	config_load_point(parser, NULL, &pt);

	actor->region.x1 = pt.x;
	actor->region.y1 = pt.y;
	actor->region.x2 += pt.x;
	actor->region.y2 += pt.y;

	return true;
}


static bool
load_size_req (yaml_parser_t* parser, const yaml_event_t* event, const char* key, gpointer _actor)
{
	g_return_val_if_fail(event->type == YAML_MAPPING_START_EVENT, false);

	AGlActor* actor = _actor;
	if (actor->class == panel_view_get_class() || actor->class == dock_v_get_class() || actor->class == dock_h_get_class()) {
		return yaml_load_section(parser,
			NULL,
			NULL,
			(YamlSequenceHandler[]){
				{"min", config_load_point, &((PanelView*)actor)->size_req.min},
				{"preferred", config_load_point, &((PanelView*)actor)->size_req.preferred},
				{"max", config_load_point, &((PanelView*)actor)->size_req.max}
			},
			actor
		);
	}
	return false;
}


#define STACK_SIZE 32

typedef struct {
	AGlActor* items[STACK_SIZE];
	int       sp;
	char      name[64];
} Stack;

#define STACK_PUSH(ACTOR) ({if(stack->sp >= STACK_SIZE) return FALSE; stack->items[++stack->sp] = ACTOR;})
#define STACK_POP() ({AGlActor* a = stack->items[stack->sp]; stack->items[stack->sp--] = NULL; a;})


static bool
type_handler (const yaml_event_t* event, const char*, gpointer _stack)
{
	g_return_val_if_fail(event->type == YAML_SCALAR_EVENT, false);

	Stack* stack = _stack;

	if (!strcmp((char*)event->data.scalar.value, "ROOT")) {
		STACK_PUSH((AGlActor*)app->scene);
	} else if (!strcmp((char*)event->data.scalar.value, "Waveform")) {
		STACK_PUSH ((AGlActor*)wf_context_add_new_actor (app->wfc, NULL));
	} else {
		AGlActorClass* c = g_hash_table_lookup(agl_actor_registry, event->data.scalar.value);
		if (c) {
			if (c->new) {
				STACK_PUSH(c->new(NULL));
				stack->items[stack->sp]->name = g_strdup(stack->name);
			}
		} else {
			pwarn("type not found: %s", event->data.scalar.value);
		}
	}

	return true;
}


static bool
generic_handler (const yaml_event_t* event, const char* key, gpointer _stack)
{
	g_return_val_if_fail(event->type == YAML_SCALAR_EVENT, false);

	Stack* stack = _stack;
	if (stack->sp < 0) return false;
	AGlActor* actor = stack->items[stack->sp];
	g_return_val_if_fail(actor, false);

	if (state_has_parameter(actor, key)) {
		state_set_named_parameter(actor, key, (char*)event->data.scalar.value);
		return true;
	}

	return false;
}


/*
 *  The previous event should be YAML_MAPPING_START_EVENT
 *
 *  Parsing is done manually so as to handle the MAPPING_END
 */
static bool
add_node (yaml_parser_t* parser, const yaml_event_t*, const char* node_name, gpointer _stack)
{
	Stack* stack = _stack;
	g_strlcpy(stack->name, node_name, 64);
	char key[64] = {0,};

	yaml_event_t event;
	while (yaml_parser_parse(parser, &event)) {
		switch (event.type) {
			case YAML_SCALAR_EVENT:
				dbg(2, "YAML_SCALAR_EVENT: value='%s' %li plain=%i style=%i", event.data.scalar.value, event.data.scalar.length, event.data.scalar.plain_implicit, event.data.scalar.style);

				g_strlcpy(key, (char*)event.data.scalar.value, 64);

				bool handled = handle_scalar_event(parser, &event, (YamlHandler[]){
					{"type", type_handler, stack},
					{NULL}
				}) ||
				handle_mapping_event(parser, &event, (YamlMappingHandler[]){
					{"size-req", load_size_req, stack->items[stack->sp]},
					{NULL}
				}) ||
				handle_sequence_event(parser, &event, (YamlSequenceHandler[]){
					{"position", config_load_position, stack->items[stack->sp]},
					{"size", config_load_size, stack->items[stack->sp]},
					{NULL}
				});

				if (handled) {
					key[0] = '\0';
				} else {
					AGlActor* actor = stack->items[stack->sp];
					g_return_val_if_fail(actor, false);
					if (state_has_parameter(actor, key)) {
						g_auto(yaml_event_t) event2;
						get_expected_event(parser, &event2, YAML_SCALAR_EVENT);
						return generic_handler(&event2, key, _stack);
					}
				}
				break;
			case YAML_MAPPING_START_EVENT:
				dbg(2, "YAML_MAPPING_EVENT: value='%s'", key);
				if (key[0]) {
					add_node(parser, NULL, key, stack);
					key[0] = '\0';
				}
				break;
			case YAML_MAPPING_END_EVENT:
				if (stack->sp > 0) {
					dbg(2, "<- sp=%i: adding '%s' to '%s'", stack->sp, stack->items[stack->sp] ? stack->items[stack->sp]->name : NULL, stack->items[stack->sp - 1] ? stack->items[stack->sp - 1]->name : NULL);
					AGlActor* actor = stack->items[stack->sp];
					AGlActor* parent = stack->items[stack->sp - 1];
					AGlActorClass* parent_class = parent->class;
					if (parent_class == dock_v_get_class() || parent_class == dock_h_get_class()) {
						dock_v_add_panel((DockVView*)stack->items[stack->sp], STACK_POP());
						{
							if (actor->class == panel_view_get_class()) {
								g_return_val_if_fail(g_list_length(actor->children) == 1, false);
								AGlActor* child = (AGlActor*)actor->children->data;
								char* name = child->name;
								if (strcmp(name, "Search") && strcmp(name, "Spectrogram") && strcmp(name, "Filters") && strcmp(name, "Tabs") && strcmp(name, "Waveform")) {
									if (!strcmp(name, "Scrollable")) {
										name = ((AGlActor*)child->children->data)->name;
									}
									((PanelView*)actor)->title = name;
								}
							}
						}
					} else if (parent_class == tabs_view_get_class()) {
						tabs_view__add_tab((TabsView*)parent, actor->name, STACK_POP());
					} else {
						agl_actor__add_child(stack->items[stack->sp], STACK_POP());
					}
					g_signal_emit_by_name(app, "actor-added", actor);
				}

				yaml_event_delete(&event);
				return true;
				break;
			case YAML_NO_EVENT:
			case YAML_SEQUENCE_START_EVENT:
				pwarn("unexpected parser event type: YAML_SEQUENCE_START_EVENT");
			case YAML_SEQUENCE_END_EVENT:
			default:
				pwarn("unexpected parser event type: %i", event.type);
				goto fail;
		}

		yaml_event_delete(&event);
	}

	return false;

  fail:
	yaml_event_delete(&event);
	return false;
}


static bool
window_handler (yaml_parser_t* parser, const yaml_event_t* _event, const char* _key, gpointer _)
{
	// At this point in the parsing, we have just found a "ROOT" map. First event should be a YAML_SCALAR_EVENT...

	Stack stack = {.sp = -1};

	return yaml_load_section (parser,
		(YamlHandler[]){
			{"type", type_handler, &stack},
			{NULL, generic_handler, &stack},
			{NULL,}
		},
		(YamlMappingHandler[]){
			{NULL, add_node, &stack},
			{NULL,}
		},
		(YamlSequenceHandler[]){
			{"size", config_load_size, app->scene},
			{"position", config_load_position, app->scene},
			{NULL,}
		},
		&stack
	);

	return false;
}


static gboolean
layout_set_size (gpointer data)
{
	agl_actor__set_size((AGlActor*)app->scene);
	return G_SOURCE_REMOVE;
}


static bool
config_load_windows (yaml_parser_t* parser, const yaml_event_t* _, const char*, gpointer user_data)
{
	yaml_load_section(parser,
		NULL,
		(YamlMappingHandler[]){
			{"window", window_handler}
		},
		NULL,
		user_data
	);

#if 0
#ifdef DEBUG
	bool check_dock_layout(AGlActor* actor)
	{
		int height = agl_actor__height(actor);

		int yl = 0;
		if(!strcmp(actor->name, "Dock H")){
			GList* l = actor->children;
			for(;l;l=l->next){
				AGlActor* child = l->data;
				yl = child->region.y2;
			}
		}
		dbg(0, "-> %i %i parent=%i", height, yl, actor->parent->region.y2);
		return true;
	}

	bool check_layout(AGlActor* actor)
	{
		GList* l = actor->children;
		for(;l;l=l->next){
			AGlActor* child = l->data;
			dbg(0, "  %s", child->name);
			if(!strcmp(child->name, "Dock H") || !strcmp(child->name, "Dock V")){
				check_dock_layout(child);
			}else{
				check_layout(child);
			}
		}
		return true;
	}
	check_layout((AGlActor*)app->scene);
#endif
#endif

	if(!g_list_length(((AGlActor*)app->scene)->children)){
		pwarn("layout did not load - pls check config file");
		return false;
	}

	g_idle_add(layout_set_size, NULL);

	return true;
}


static FILE*
open_settings_file ()
{
	g_autofree char* cwd = g_get_current_dir();
	char* paths[] = {
		g_strdup_printf("%s/.config/" PACKAGE, g_get_home_dir()),
		g_build_filename(PACKAGE_DATA_DIR "/" PACKAGE, NULL),
		g_build_filename(cwd, NULL)
	};

	FILE* fp;
	for (int i=0; i<G_N_ELEMENTS(paths); i++) {
		char* filename = g_strdup_printf("%s/"PACKAGE".yaml", paths[i]);
		fp = fopen(filename, "rb");
		g_free(filename);
		if (fp) break;
	}
	if (!fp) {
		fprintf(stderr, "unable to load config file %s/"PACKAGE".yaml\n", paths[0]);
	}
	for (int i=0;i<G_N_ELEMENTS(paths);i++) {
		g_free(paths[i]);
	}
	return fp;
}


AGliPt
get_window_size_from_settings ()
{
	AGliPt size = {640, 360};

	yaml_parser_t parser; yaml_parser_initialize(&parser);

	FILE* fp = open_settings_file();
	if (fp) {
		yaml_parser_set_input_file(&parser, fp);

		yaml_event_t e;
		yaml_event_t* event = &e;
		if (find_event(&parser, &event, "size")) {
			config_load_point(&parser, NULL, &size);
			yaml_event_delete(event);
		}

		yaml_parser_delete(&parser);
		fclose(fp);
	}

	return size;
}


/*
 *  On first run, settings are loaded from /usr/share/samplecat or cwd
 */
bool
load_settings ()
{
	PF;

	if(!agl_actor_registry){
		agl_actor_registry = g_hash_table_new(g_str_hash, g_str_equal);
		agl_actor_register_class("Dock H", dock_h_get_class());
		agl_actor_register_class("Dock V", dock_v_get_class());
		agl_actor_register_class("Panel", panel_view_get_class());
		agl_actor_register_class("Dirs", directories_view_get_class());
		agl_actor_register_class("Inspector", inspector_view_get_class());
		agl_actor_register_class("Search", search_view_get_class());
		agl_actor_register_class("Filters", filters_view_get_class());
		agl_actor_register_class("Player", player_view_get_class());
		agl_actor_register_class("Tabs", tabs_view_get_class());
		agl_actor_register_class("List", list_view_get_class());
		agl_actor_register_class("Scrollbar", scrollbar_view_get_class());
		agl_actor_register_class("Files", files_view_get_class());
		agl_actor_register_class("Waveform", wf_actor_get_class());
#ifdef HAVE_FFTW3
		agl_actor_register_class("Spectrogram", spectrogram_view_get_class());
#endif

		agl_actor_class__add_behaviour(files_view_get_class(), panel_get_class());
		agl_actor_class__add_behaviour(search_view_get_class(), panel_get_class());
		agl_actor_class__add_behaviour(inspector_view_get_class(), panel_get_class());
#ifdef HAVE_FFTW3
		agl_actor_class__add_behaviour(spectrogram_view_get_class(), panel_get_class());
#endif
		agl_actor_class__add_behaviour(wf_actor_get_class(), panel_get_class());
	}

	FILE* fp = open_settings_file();

	bool ret = fp && yaml_load(fp, (YamlMappingHandler[]){
		{"windows", config_load_windows},
		{NULL}
	});

	if(fp) fclose(fp);

	return ret;
}


bool
save_settings ()
{
	PF;

	if (!yaml_emitter_initialize(&emitter)) { gerr("failed to initialise yaml writer."); return false; }

	char* tmp = g_strdup_printf("%s/samplecat.yaml", g_get_tmp_dir());
	FILE* fp = fopen(tmp, "wb");
	if (!fp) {
		printf("cannot open config file for writing (%s)\n", app->config_ctx.filename);
		return FALSE;
	}

	yaml_emitter_set_output_file(&emitter, fp);
	yaml_emitter_set_canonical(&emitter, false);

	yaml_event_t event;
	EMIT_(yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING));
	EMIT_(yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 0));
	EMIT_(yaml_mapping_start_event_initialize(&event, NULL, (guchar*)"tag:yaml.org,2002:map", 1, YAML_BLOCK_MAPPING_STYLE));

	char value[256];
	snprintf(value, 255, "0x%08x", 0xff00ff00);
	if (!yaml_add_key_value_pair("test_colour", value)) goto error;

	bool add_child (yaml_event_t* event, AGlActor* actor)
	{
		AGlActorClass* c = actor->class;
		if (!c) return true; // It is not an error for an actor to not have a class. It will not be saved.

		if (c != scrollbar_view_get_class() && c != button_get_class() && c != &default_actor_class) {
			g_return_val_if_fail(actor->name, false);

			map_open_(event, actor->name);

			if (!yaml_add_key_value_pair("type", c->name)) goto error;

			bool is_panel_child = actor->parent && actor->parent->class == panel_view_get_class();

			if (!is_panel_child) {
				int vals1[2] = {actor->region.x1, actor->region.y1};
				if (vals1[0] || vals1[1]) {
					yaml_add_key_value_pair_array("position", vals1, 2);
				}
				int vals2[2] = {agl_actor__width(actor), agl_actor__height(actor)};
				yaml_add_key_value_pair_array("size", vals2, 2);
			}

			if (actor->class == panel_view_get_class() || actor->class == dock_v_get_class() || actor->class == dock_h_get_class()) {
				PanelView* panel = (PanelView*)actor;
				bool b[3] = {
					panel->size_req.min.x > -1 || panel->size_req.min.y > -1,
					panel->size_req.preferred.x > -1 || panel->size_req.preferred.y > -1,
					panel->size_req.max.x > -1 || panel->size_req.max.y > -1
				};
				if (b[0] || b[1] || b[2]) {
					map_open_(event, "size-req");
					if (b[0])
						if(!yaml_add_key_value_pair_pt("min", &panel->size_req.min)) goto error;
					if (b[1])
						if(!yaml_add_key_value_pair_pt("preferred", &panel->size_req.preferred)) goto error;
					if (b[2])
						if(!yaml_add_key_value_pair_pt("max", &panel->size_req.max)) goto error;
					end_map(event);
				}
			}

			StateBehaviour* b = (StateBehaviour*)agl_actor__find_behaviour (actor, state_get_class());
			if (b) {
				ParamArray* params = b->params;
				if (params) {
					for (int i = 0; i < params->size; i++) {
						ConfigParam* param = &params->params[i];
						switch (param->utype) {
							case G_TYPE_STRING:
								if(!yaml_add_key_value_pair(param->name, param->val.c)) goto error;
								break;
							default:
								break;
						}
					}
				}
			}

			if (!b || b->is_container) {
				for (GList* l = actor->children;l;l=l->next) {
					if (!add_child(event, l->data)) goto error;
				}
			}

			end_map(event);
		}
		return true;
	  error:
		return false;
	}

	map_open_(&event, "windows");
	map_open_(&event, "window");
	if (!((AGlActor*)app->scene)->children || !add_child(&event, (AGlActor*)app->scene)) goto close;
	end_map(&event);
	end_map(&event);

	EMIT_(yaml_mapping_end_event_initialize(&event));

	end_document;
	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
	fclose(fp);
	dbg(1, "yaml write finished ok.");

	char* filename = g_strdup_printf("%s.yaml", app->config_ctx.filename);
	if(g_rename (tmp, filename)){
		pwarn("failed to save config");
	}
	g_free(filename);
	g_free(tmp);

	return true;

  error:
	switch (emitter.error){
		case YAML_MEMORY_ERROR:
			fprintf(stderr, "Memory error: Not enough memory for emitting\n");
			break;
		case YAML_WRITER_ERROR:
			fprintf(stderr, "Writer error: %s\n", emitter.problem);
			break;
		case YAML_EMITTER_ERROR:
			fprintf(stderr, "yaml emitter error: %s\n", emitter.problem);
			break;
		default:
			fprintf(stderr, "Internal error\n");
			break;
	}
	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
  close:
	fclose(fp);
	g_free(tmp);

	return false;
}

