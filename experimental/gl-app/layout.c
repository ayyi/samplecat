/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <debug/debug.h>
#include "agl/actor.h"
#include "src/typedefs.h"
#include "samplecat.h"
#include "utils/ayyi_utils.h"
#include "waveform/waveform.h"
#include "yaml_utils.h"
#include "application.h"
#include "layout.h"

typedef AGlActorClass* (get_class)();
get_class dock_v_get_class, dock_h_get_class, scrollable_view_get_class, list_view_get_class, files_view_get_class, directories_view_get_class, inspector_view_get_class, search_view_get_class, scrollbar_view_get_class;
#include "views/dock_v.h"
#include "views/tabs.h"

GHashTable* agl_actor_registry; // maps className to AGlActorClass

#define PLAIN_IMPLICIT true

#define end_map_(E) \
		if(!yaml_mapping_end_event_initialize(E)) goto error; \
		if(!yaml_emitter_emit(&emitter, E)) goto error;

#define map_open_(E, A) \
		if(!yaml_scalar_event_initialize(E, NULL, str_tag, (guchar*)A, -1, PLAIN_IMPLICIT, 0, YAML_PLAIN_SCALAR_STYLE)) goto error; \
		if(!yaml_emitter_emit(&emitter, E)) goto error; \
		if(!yaml_mapping_start_event_initialize(E, NULL, map_tag, 1, YAML_BLOCK_MAPPING_STYLE)) goto error; \
		if(!yaml_emitter_emit(&emitter, E)) goto error; \

#define EMIT(A) \
	if(!A) goto error; \
	if(!yaml_emitter_emit(&emitter, &event)) goto error;


static void
agl_actor_register_class (const char* name, AGlActorClass* class)
{
	static int i = 0;
	class->type = ++i;
	g_hash_table_insert(agl_actor_registry, (char*)name, class);
}


static AGliPt
config_load_point (yaml_parser_t* parser, yaml_event_t* event)
{
	AGliPt p = {0,};

	void load_integers(yaml_parser_t* parser, AGliPt* p)
	{
		yaml_event_t event;
		int i; for(i=0;i<2;i++){
			if(yaml_parser_parse(parser, &event)){
				switch (event.type) {
					case YAML_SCALAR_EVENT:
						;int* a = i ? &p->y : &p->x;
						*a = atoi((char*)event.data.scalar.value);
						break;
					default:
						break;
				}
			}
		}
		yaml_event_delete(&event);
	}

	while(yaml_parser_parse(parser, event)){
		switch (event->type) {
			case YAML_SEQUENCE_START_EVENT:
				load_integers(parser, &p);
				break;
			case YAML_SEQUENCE_END_EVENT:
				return p;
			case YAML_SCALAR_EVENT:
				gwarn("expected array start but got scalar");
				break;
			default:
				gwarn("expected array start %i", event->type);
		}
	}
	gwarn("?");
	return p;
}


#if 0
/*
 *  Parse mandatory wrapper
 */
static bool
start_wrapper (yaml_parser_t* parser, const char* key)
{
	yaml_event_t event;
	if(yaml_parser_parse(parser, &event)){
		if(event.type == YAML_MAPPING_START_EVENT){
			return TRUE;
		}
	}
	return FALSE;
}


static bool
end_wrapper (yaml_parser_t* parser)
{
	yaml_event_t event;
	if(yaml_parser_parse(parser, &event)){
		if(event.type == YAML_MAPPING_END_EVENT){
			return TRUE;
		}
	}
	return FALSE;
}
#endif

#define get_expected_event(parser, event, EVENT_TYPE) \
	if(!yaml_parser_parse(parser, event)) return false; \
	if((event)->type != EVENT_TYPE) return false;

static bool
load_size_req (yaml_parser_t* parser, yaml_event_t* event, AGlActor* actor)
{
	if(actor->class == panel_view_get_class() || actor->class == dock_v_get_class() || actor->class == dock_h_get_class()){
		get_expected_event(parser, event, YAML_MAPPING_START_EVENT);
		int i = 0; for(;yaml_parser_parse(parser, event) && i < 20; i++){
			switch(event->type){
				case YAML_SCALAR_EVENT:
					{
						char* key = (char*)event->data.scalar.value;
						if(!strcmp(key, "min")){
							((PanelView*)actor)->size_req.min = config_load_point(parser, event);
						}else if(!strcmp(key, "preferred")){
							((PanelView*)actor)->size_req.preferred = config_load_point(parser, event);
						}else if(!strcmp(key, "max")){
							((PanelView*)actor)->size_req.max = config_load_point(parser, event);
						}
					}
					break;
				case YAML_MAPPING_END_EVENT:
					return TRUE;
				default:
					return false;
			}
		}
	}
	return false;
}


static bool
config_load_window_yaml (yaml_parser_t* parser, yaml_event_t* event)
{
	//at this point in the parsing, we have just found a "ROOT" map. First event should be a YAML_SCALAR_EVENT...

	#define STACK_SIZE 32
	AGlActor* stack[STACK_SIZE] = {0,};
	int sp = -1;
	#define STACK_PUSH(ACTOR) ({if(sp >= STACK_SIZE) return FALSE; stack[++sp] = ACTOR;})
	#define STACK_POP() ({AGlActor* a = stack[sp]; stack[sp--] = NULL; a;})

	//if(!start_wrapper(parser, "ROOT")) return FALSE;

	char key[64] = {0,};
	char name[64] = {0,};
	int depth = 0;
	while(yaml_parser_parse(parser, event)){
		switch (event->type) {
			case YAML_SCALAR_EVENT:
				dbg(2, "YAML_SCALAR_EVENT: value='%s' %i plain=%i style=%i", event->data.scalar.value, event->data.scalar.length, event->data.scalar.plain_implicit, event->data.scalar.style);

				if(!key[0]){
					// 1st half of a pair
					g_strlcpy(key, (char*)event->data.scalar.value, 64);

					if(!strcmp(key, "size")){
						AGliPt p = config_load_point(parser, event);
						stack[sp]->region.x2 = p.x;
						stack[sp]->region.y2 = p.y;
						key[0] = '\0';
					} else if(!strcmp(key, "position")){
						AGliPt p = config_load_point(parser, event);
						stack[sp]->region.x1 = p.x;
						stack[sp]->region.y1 = p.y;
						key[0] = '\0';
					} else if(!strcmp(key, "size-req")){
						load_size_req(parser, event, stack[sp]);
						key[0] = '\0';
					}
				}else{
					// 2nd half of a pair
					if(!strcmp(key, "type")){
						dbg(2, "found actor: %s=%s", key, event->data.scalar.value);
						if(!strcmp((char*)event->data.scalar.value, "ROOT")){
							STACK_PUSH((AGlActor*)app->scene);
						}else if(!strcmp((char*)event->data.scalar.value, "Waveform")){
							STACK_PUSH((AGlActor*)wf_canvas_add_new_actor(app->wfc, NULL));
						}else{
							AGlActorClass* c = g_hash_table_lookup(agl_actor_registry, event->data.scalar.value);
							if(c){
								if(c->new){
									STACK_PUSH(c->new(NULL));
									stack[sp]->name = g_strdup_printf(name);
								}
							}else{
								gwarn("             type not found: %s", event->data.scalar.value);
							}
						}
					}else{
						dbg(0, "  ignoring: %s=%s", key, event->data.scalar.value);
					}
					key[0] = '\0';
				}
				break;
			case YAML_MAPPING_START_EVENT:
				depth++;
				if(key[0]){
					g_strlcpy(name, key, 64);
					key[0] = '\0';
				}
				else gwarn("mapping event has no name. depth=%i", depth);
				break;
			case YAML_MAPPING_END_EVENT:
				dbg(2, "<-");
				if(sp > 0){
					dbg(2, "<- sp=%i: adding '%s' to '%s'", sp, stack[sp] ? stack[sp]->name : NULL, stack[sp - 1] ? stack[sp - 1]->name : NULL);
					AGlActor* actor = stack[sp];
					AGlActor* parent = stack[sp - 1];
					AGlActorClass* parent_class = parent->class;
					if(parent_class == dock_v_get_class() || parent_class == dock_h_get_class()){
						dock_v_add_panel((DockVView*)stack[sp], STACK_POP());
					}else if(parent_class == tabs_view_get_class()){
						tabs_view__add_tab((TabsView*)parent, actor->name, STACK_POP());
					}else{
						agl_actor__add_child(stack[sp], STACK_POP());
					}
					g_signal_emit_by_name(app, "actor-added", actor);
				}

				depth--;
				if(depth < 0){ dbg(2, "done"); return true; }
				break;
			case YAML_SEQUENCE_START_EVENT:
			case YAML_SEQUENCE_END_EVENT:
			default:
				gwarn("unexpected parser event type: %i", event->type);
				break;
		}
	}

	//if(!end_wrapper(parser)) return FALSE;

	gwarn("unexpected end");
	return false;
}


static void
config_load_windows_yaml (yaml_parser_t* parser, yaml_event_t* event)
{
	// TODO use start_wrapper ?

	char key[64] = {0,};
	int depth = 0;
	int i = 0;
	while(yaml_parser_parse(parser, event) && i++ < 2){
		switch (event->type) {
			case YAML_SCALAR_EVENT:
				dbg(2, "YAML_SCALAR_EVENT: value='%s' %i plain=%i style=%i", event->data.scalar.value, event->data.scalar.length, event->data.scalar.plain_implicit, event->data.scalar.style);

				if(!key[0]){
					// first half of a pair
					g_strlcpy(key, (char*)event->data.scalar.value, 64);
				}else{
					gwarn("unexpected");
				}
				break;
			case YAML_MAPPING_START_EVENT:
				depth++;
				if(key[0]){
					if(strcmp(key, "window")) gwarn("expected 'window'. no other valid sections.");

					config_load_window_yaml(parser, event);
				}
				break;
			default:
				break;
		}
	}

	bool layout_set_size(gpointer data)
	{
		agl_actor__set_size((AGlActor*)app->scene);
		return G_SOURCE_REMOVE;
	}
	g_idle_add(layout_set_size, NULL);
}


/*
 *  On first run, settings are loaded from /usr/share/samplecat or cwd
 */
gboolean
load_settings ()
{
	PF;

	if(!agl_actor_registry){
		agl_actor_registry = g_hash_table_new(g_str_hash, g_str_equal);
		agl_actor_register_class("Dock H", dock_h_get_class());
		agl_actor_register_class("Dock V", dock_v_get_class());
		agl_actor_register_class("Panel", panel_view_get_class());
		agl_actor_register_class("Scrollable", scrollable_view_get_class());
		agl_actor_register_class("Dirs", directories_view_get_class());
		agl_actor_register_class("Inspector", inspector_view_get_class());
		agl_actor_register_class("Search", search_view_get_class());
		agl_actor_register_class("Tabs", tabs_view_get_class());
		agl_actor_register_class("List", list_view_get_class());
		agl_actor_register_class("Scrollbar", scrollbar_view_get_class());
		agl_actor_register_class("Files", files_view_get_class());
		agl_actor_register_class("Waveform", wf_actor_get_class());
	}

	yaml_parser_t parser; yaml_parser_initialize(&parser);

	char* paths[] = {
		g_strdup_printf("%s/.config/" PACKAGE, g_get_home_dir()),
		g_build_filename(PACKAGE_DATA_DIR "/" PACKAGE, NULL),
		g_build_filename(g_get_current_dir(), NULL)
	};

	FILE* file_input;
	int i; for(i=0;i<G_N_ELEMENTS(paths);i++){
		char* filename = g_strdup_printf("%s/"PACKAGE".yaml", paths[i]);
		file_input = fopen(filename, "rb");
		g_free(filename);
		if(file_input) break;
	}
	for(i=0;i<G_N_ELEMENTS(paths);i++){
		g_free(paths[i]);
	}
	if(!file_input){
		fprintf(stderr, "unable to load config file");
		return false;
	}

	yaml_parser_set_input_file(&parser, file_input);

	int section = 0;
	int safety = 0;
	int depth = 0;
	char key[64] = {0,};
	gboolean end = FALSE;
	yaml_event_t event;

	get_expected_event(&parser, &event, YAML_STREAM_START_EVENT);
	get_expected_event(&parser, &event, YAML_DOCUMENT_START_EVENT);

	do {
		if (!yaml_parser_parse(&parser, &event)) goto error; // Get the next event.

		switch (event.type) {
			case YAML_DOCUMENT_END_EVENT:
				end = TRUE;
				dbg(2, "YAML_DOCUMENT_END_EVENT");
				break;
			case YAML_SCALAR_EVENT:
				dbg(2, "YAML_SCALAR_EVENT: value=%s %i plain=%i style=%i", event.data.scalar.value, event.data.scalar.length, event.data.scalar.plain_implicit, event.data.scalar.style);

				if(!key[0]){ // first half of a pair
					g_strlcpy(key, (char*)event.data.scalar.value, 64);
				}else{
					// second half of a pair
					dbg(2, "      %s=%s", key, event.data.scalar.value);
					if(!strcmp(key, "example-1")){
						long long colour = strtoll((char*)event.data.scalar.value, NULL, 16);
						dbg(0, "%Li", colour);
					}
					else if(!strcmp(key, "example-2")){
						long long colour = strtoll((char*)event.data.scalar.value, NULL, 16);
						dbg(0, "%Li", colour);
					}

					key[0] = '\0';
				}
				break;
			case YAML_MAPPING_START_EVENT:
				depth++;
				if(key[0]){
					if(!section){
						if(!strcmp(key, "windows")){
							dbg(2, "found Windows section");
							config_load_windows_yaml(&parser, &event);

						}
						else gwarn("unexpected section: %s", key);
					}
					else dbg(2, "new section: %s", key);
					key[0] = '\0';
				}
				else dbg(2, "YAML_MAPPING_START_EVENT");
				break;
			case YAML_MAPPING_END_EVENT:
				dbg(2, "YAML_MAPPING_END_EVENT");
				if(--depth < 0) gwarn("too many YAML_MAPPING_END_EVENT's.");
				break;
			case YAML_STREAM_END_EVENT:
				end = TRUE;
				dbg(2, "YAML_STREAM_END_EVENT");
				break;
			case YAML_SEQUENCE_START_EVENT:
			case YAML_SEQUENCE_END_EVENT:
			case YAML_ALIAS_EVENT:
			case YAML_NO_EVENT:
			default:
				gwarn("unexpected event: %i", event.type);
				break;
		}

		//the application is responsible for destroying the event object.
		yaml_event_delete(&event);

	} while(!end && safety++ < 50);

	yaml_parser_delete(&parser);

	return TRUE;

  error:
	yaml_parser_delete(&parser);

	return FALSE;
}


gboolean
save_settings ()
{
	PF;
	char value[256];

	if(!yaml_emitter_initialize(&emitter)){ gerr("failed to initialise yaml writer."); return FALSE; }

	char* filename = g_strdup_printf("%s.yaml", app->config_ctx.filename);
	FILE* fp = fopen(filename, "wb");
	g_free(filename);
	if(!fp){
		printf("cannot open config file for writing (%s)\n", app->config_ctx.filename);
		return FALSE;
	}

	yaml_emitter_set_output_file(&emitter, fp);
	yaml_emitter_set_canonical(&emitter, false);

	yaml_event_t event;
	EMIT(yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING));
	EMIT(yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 0));
	EMIT(yaml_mapping_start_event_initialize(&event, NULL, (guchar*)"tag:yaml.org,2002:map", 1, YAML_BLOCK_MAPPING_STYLE));

	snprintf(value, 255, "0x%08x", 0xff00ff00);
	if(!yaml_add_key_value_pair("test_colour", value)) goto error;

	void add_child(yaml_event_t* event, AGlActor* actor)
	{
		AGlActorClass* c = actor->class;
		g_return_if_fail(c);
		if(c != scrollbar_view_get_class()){
			g_return_if_fail(actor->name);

			map_open_(event, actor->name);

			if(!yaml_add_key_value_pair("type", c->name)) goto error;

			int vals1[2] = {actor->region.x1, actor->region.y1};
			yaml_add_key_value_pair_array("position", vals1, 2);
			int vals2[2] = {actor->region.x2, actor->region.y2};
			yaml_add_key_value_pair_array("size", vals2, 2);

			if(actor->class == panel_view_get_class() || actor->class == dock_v_get_class() || actor->class == dock_h_get_class()){
				PanelView* panel = (PanelView*)actor;
				bool b[3] = {
					panel->size_req.min.x > -1 || panel->size_req.min.y > -1,
					panel->size_req.preferred.x > -1 || panel->size_req.preferred.y > -1,
					panel->size_req.max.x > -1 || panel->size_req.max.y > -1
				};
				if(b[0] || b[1] || b[2]){
					map_open_(event, "size-req");
					if(b[0])
						if(!yaml_add_key_value_pair_pt("min", &panel->size_req.min)) goto error;
					if(b[1])
						if(!yaml_add_key_value_pair_pt("preferred", &panel->size_req.preferred)) goto error;
					if(b[2])
						if(!yaml_add_key_value_pair_pt("max", &panel->size_req.max)) goto error;
					end_map_(event);
				}
			}

			GList* l = actor->children;
			for(;l;l=l->next){
				add_child(event, l->data);
			}

			end_map_(event);
		}
		return;
	  error:
		return;
	}

	map_open_(&event, "windows");
	map_open_(&event, "window");
	add_child(&event, (AGlActor*)app->scene);
	end_map_(&event);
	end_map_(&event);

	EMIT(yaml_mapping_end_event_initialize(&event));

	end_document;
	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
	fclose(fp);
	dbg(1, "yaml write finished ok.");

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
	fclose(fp);

	return false;
}


