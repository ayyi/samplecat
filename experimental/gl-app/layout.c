/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <glib/gstdio.h>
#include <debug/debug.h>
#include "agl/actor.h"
#include "samplecat.h"
#include "utils/ayyi_utils.h"
#include "waveform/actor.h"
#include "yaml_utils.h"
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
					return true;
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
	// At this point in the parsing, we have just found a "ROOT" map. First event should be a YAML_SCALAR_EVENT...

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
						stack[sp]->region.x2 = stack[sp]->region.x1 + p.x;
						stack[sp]->region.y2 = stack[sp]->region.y1 + p.y;
						key[0] = '\0';
					} else if(!strcmp(key, "position")){
						AGliPt p = config_load_point(parser, event);
						stack[sp]->region.x1 = p.x;
						stack[sp]->region.y1 = p.y;
						stack[sp]->region.x2 += p.x;
						stack[sp]->region.y2 += p.y;
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
									stack[sp]->name = g_strdup(name);
								}
							}else{
								gwarn("             type not found: %s", event->data.scalar.value);
							}
						}
					}else{
						if(!state_set_named_parameter(stack[sp], key, (char*)event->data.scalar.value)){
							dbg(0, "  ignoring: %s=%s", key, event->data.scalar.value);
						}
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
						{
							if(actor->class == panel_view_get_class()){
								g_return_val_if_fail(g_list_length(actor->children) == 1, false);
								AGlActor* child = (AGlActor*)actor->children->data;
								char* name = child->name;
								if(strcmp(name, "Search") && strcmp(name, "Spectrogram") && strcmp(name, "Filters") && strcmp(name, "Tabs") && strcmp(name, "Waveform")){
									if(!strcmp(name, "Scrollable")){
										name = ((AGlActor*)child->children->data)->name;
									}
									((PanelView*)actor)->title = name;
								}
							}
						}
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


static gboolean
layout_set_size (gpointer data)
{
	agl_actor__set_size((AGlActor*)app->scene);
	return G_SOURCE_REMOVE;
}


static void
config_load_windows_yaml (yaml_parser_t* parser, yaml_event_t* event, gpointer user_data)
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

	if(!g_list_length(((AGlActor*)app->scene)->children)) return gwarn("layout did not load - pls check config file");

	g_idle_add(layout_set_size, NULL);
}


static FILE*
open_settings_file ()
{
	char* paths[] = {
		g_strdup_printf("%s/.config/" PACKAGE, g_get_home_dir()),
		g_build_filename(PACKAGE_DATA_DIR "/" PACKAGE, NULL),
		g_build_filename(g_get_current_dir(), NULL)
	};

	FILE* fp;
	int i; for(i=0;i<G_N_ELEMENTS(paths);i++){
		char* filename = g_strdup_printf("%s/"PACKAGE".yaml", paths[i]);
		fp = fopen(filename, "rb");
		g_free(filename);
		if(fp) break;
	}
	if(!fp){
		fprintf(stderr, "unable to load config file %s/"PACKAGE".yaml\n", paths[0]);
	}
	for(i=0;i<G_N_ELEMENTS(paths);i++){
		g_free(paths[i]);
	}
	return fp;
}


static bool
find_event (yaml_parser_t* parser, yaml_event_t* event, const char* name)
{
	bool found = false;
	while(!found && yaml_parser_parse(parser, event)){
		switch (event->type) {
			case YAML_SCALAR_EVENT:
				if(!strcmp((char*)event->data.scalar.value, "size")){
					found = true;
				}
				break;
			default:
				break;
		}
	}
	return found;
}


AGliPt
get_window_size_from_settings ()
{
	AGliPt size = {640, 360};

	yaml_parser_t parser; yaml_parser_initialize(&parser);

	FILE* fp = open_settings_file();
	if(fp){
		yaml_parser_set_input_file(&parser, fp);

		yaml_event_t event;
		if(find_event(&parser, &event, "size")){
			size = config_load_point(&parser, &event);
		}
		yaml_event_delete(&event);

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
		agl_actor_register_class("Scrollable", scrollable_view_get_class());
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

	yaml_parser_t parser; yaml_parser_initialize(&parser);

	FILE* fp = open_settings_file();
	if(!fp) return false;

	return yaml_load(fp, (YamlHandler[]){
		{"windows", config_load_windows_yaml},
		{NULL}
	});
}


bool
save_settings ()
{
	PF;

	if(!yaml_emitter_initialize(&emitter)){ gerr("failed to initialise yaml writer."); return false; }

	char* tmp = g_strdup_printf("%s/samplecat.yaml", g_get_tmp_dir());
	FILE* fp = fopen(tmp, "wb");
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

	char value[256];
	snprintf(value, 255, "0x%08x", 0xff00ff00);
	if(!yaml_add_key_value_pair("test_colour", value)) goto error;

	bool add_child (yaml_event_t* event, AGlActor* actor)
	{
		AGlActorClass* c = actor->class;
		g_return_val_if_fail(c, false);
		if(c != scrollbar_view_get_class() && c != button_get_class()){
			g_return_val_if_fail(actor->name, false);

			map_open_(event, actor->name);

			if(!yaml_add_key_value_pair("type", c->name)) goto error;

			bool is_panel_child = actor->parent && actor->parent->class == panel_view_get_class();

			if(!is_panel_child){
				int vals1[2] = {actor->region.x1, actor->region.y1};
				if(vals1[0] || vals1[1]){
					yaml_add_key_value_pair_array("position", vals1, 2);
				}
				int vals2[2] = {agl_actor__width(actor), agl_actor__height(actor)};
				yaml_add_key_value_pair_array("size", vals2, 2);
			}

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

			StateBehaviour* b = (StateBehaviour*)agl_actor__get_behaviour(actor, state_get_class());
			if(b){
				ParamArray* params = b->params;
				if(params){
					for(int i = 0; i < params->size; i++){
						ConfigParam* param = &params->params[i];
						switch(param->utype){
							case G_TYPE_STRING:
								if(!yaml_add_key_value_pair(param->name, param->val.c)) goto error;
								break;
							default:
								break;
						}
					}
				}
			}

			if(!b || b->is_container){
				for(GList* l = actor->children;l;l=l->next){
					if(!add_child(event, l->data)) goto error;
				}
			}

			end_map_(event);
		}
		return true;
	  error:
		return false;
	}

	map_open_(&event, "windows");
	map_open_(&event, "window");
	if(!((AGlActor*)app->scene)->children || !add_child(&event, (AGlActor*)app->scene)) goto close;
	end_map_(&event);
	end_map_(&event);

	EMIT(yaml_mapping_end_event_initialize(&event));

	end_document;
	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
	fclose(fp);
	dbg(1, "yaml write finished ok.");

	char* filename = g_strdup_printf("%s.yaml", app->config_ctx.filename);
	if(g_rename (tmp, filename)){
		gwarn("failed to save config");
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


