/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://www.ayyi.org           |
* | copyright (C) 2004-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#define __yaml_utils_c__
#include <stdint.h>
#include <yaml.h>
#include "debug/debug.h"
#include "yaml_utils.h"

unsigned char* str_tag = (unsigned char*)"tag:yaml.org,2002:str";
unsigned char* map_tag = (unsigned char*)"tag:yaml.org,2002:map";
yaml_emitter_t emitter;

#define EMIT(A) \
	if(!A) return FALSE; \
	if(!yaml_emitter_emit(&emitter, &event)) return FALSE;

#define get_expected_event(parser, event, EVENT_TYPE) \
	if(!yaml_parser_parse(parser, event)) return false; \
	if((event)->type != EVENT_TYPE) return false;

bool
yaml_add_key_value_pair (const char* key, const char* value)
{
	yaml_event_t event;

	if(!value) value = "";

	EMIT(yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)key, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE));
	EMIT(yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)value, -1, 0, 1, YAML_PLAIN_SCALAR_STYLE));

	return TRUE;
}


bool
yaml_add_key_value_pair_int (const char* key, int ival)
{
	yaml_event_t event;
	char value[256];
	snprintf(value, 255, "%i", ival);

	EMIT(yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)key, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE));
	EMIT(yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)value, -1, 1, 0, YAML_ANY_SCALAR_STYLE));

	return TRUE;
}


bool
yaml_add_key_value_pair_float (const char* key, float fval)
{
	yaml_event_t event;
	char value[256];
	snprintf(value, 255, "%.2f", fval);

	if(!yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)key, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE)) return FALSE;
	if(!yaml_emitter_emit(&emitter, &event)) return FALSE;
	if(!yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)value, -1, 0, 1, YAML_PLAIN_SCALAR_STYLE)) return FALSE;
	if(!yaml_emitter_emit(&emitter, &event)) return FALSE;
	return TRUE;
}


bool
yaml_add_key_value_pair_bool (const char* key, bool val)
{
	yaml_event_t event;
	char value[256];
	snprintf(value, 255, "%s", val ? "true" : "false");

	if(!yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)key, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE)) return FALSE;
	if(!yaml_emitter_emit(&emitter, &event)) return FALSE;
	if(!yaml_scalar_event_initialize(&event, NULL, (guchar*)YAML_BOOL_TAG, (guchar*)value, -1, 1, 0, YAML_ANY_SCALAR_STYLE)) return FALSE;
	if(!yaml_emitter_emit(&emitter, &event)) return FALSE;
	return TRUE;
}


#define EVENT_INIT(event, event_type, event_start_mark, event_end_mark)        \
	(memset(&(event), 0, sizeof(yaml_event_t)),                                \
	(event).type = (event_type),                                               \
	(event).start_mark = (event_start_mark),                                   \
	(event).end_mark = (event_end_mark))

#define SEQUENCE_START_EVENT_INIT(event, event_anchor, event_tag,              \
		event_implicit,event_style,start_mark,end_mark)                        \
	(EVENT_INIT((event), YAML_SEQUENCE_START_EVENT, (start_mark), (end_mark)), \
		(event).data.sequence_start.anchor = (event_anchor),                   \
		(event).data.sequence_start.tag = (event_tag),                         \
		(event).data.sequence_start.implicit = (event_implicit),               \
		(event).data.sequence_start.style = (event_style)                      \
	)

#define SEQUENCE_END_EVENT_INIT(event, start_mark, end_mark)                   \
	(EVENT_INIT((event), YAML_SEQUENCE_END_EVENT, (start_mark), (end_mark)),   \
	TRUE)

bool
yaml_add_key_value_pair_array (const char* key, int val[], int size)
{
	yaml_event_t event;
	yaml_mark_t mark = {0, 0, 0};
	yaml_char_t* anchor = NULL;
	char value[256];

	EMIT(yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)key, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE));

	gpointer tag = NULL;
	EMIT(SEQUENCE_START_EVENT_INIT(event, anchor, tag, /*PLAIN_IMPLICIT*/TRUE, /*node->data.sequence.style*/YAML_FLOW_SEQUENCE_STYLE, mark, mark));

	int i; for(i=0;i<size;i++){
		snprintf(value, 255, "%i", val[i]);
		EMIT(yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)value, -1, 1, 0, YAML_ANY_SCALAR_STYLE));
	}

    EMIT(SEQUENCE_END_EVENT_INIT(event, mark, mark));

	yaml_event_delete(&event);

	return TRUE;
}


bool
yaml_add_key_value_pair_pt (const char* key, AGliPt* pt)
{
	yaml_event_t event;
	yaml_mark_t mark = {0, 0, 0};
	yaml_char_t* anchor = NULL;
	char value[16] = {0,};

	EMIT(yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)key, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE));

	gpointer tag = NULL;
	EMIT(SEQUENCE_START_EVENT_INIT(event, anchor, tag, /*PLAIN_IMPLICIT*/TRUE, /*node->data.sequence.style*/YAML_FLOW_SEQUENCE_STYLE, mark, mark));

	int i; for(i=0;i<2;i++){
		snprintf(value, 15, "%i", i ? pt->y : pt->x);
		EMIT(yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)value, -1, 1, 0, YAML_ANY_SCALAR_STYLE));
	}

    EMIT(SEQUENCE_END_EVENT_INIT(event, mark, mark));

	yaml_event_delete(&event);

	return TRUE;
}


/*
 *  Each top-level section in the yaml file is passed to its matching handler
 */
bool
yaml_load (FILE* fp, YamlHandler handlers[])
{
	yaml_parser_t parser; yaml_parser_initialize(&parser);

	yaml_parser_set_input_file(&parser, fp);

	int section = 0;

	// read the event sequence.
	int safety = 0;
	int depth = 0;
	char key[64] = "";
	bool end = FALSE;
	yaml_event_t event;

	get_expected_event(&parser, &event, YAML_STREAM_START_EVENT);
	get_expected_event(&parser, &event, YAML_DOCUMENT_START_EVENT);

	do {
		if (!yaml_parser_parse(&parser, &event)) goto error; // Get the next event.

		switch (event.type) {
			case YAML_STREAM_START_EVENT:
				dbg(2, "YAML_STREAM_START_EVENT");
				break;
			case YAML_STREAM_END_EVENT:
				end = TRUE;
				dbg(2, "YAML_STREAM_END_EVENT");
				break;
			case YAML_DOCUMENT_START_EVENT:
				dbg(2, "YAML_DOCUMENT_START_EVENT");
				break;
			case YAML_DOCUMENT_END_EVENT:
				end = TRUE;
				dbg(2, "YAML_DOCUMENT_END_EVENT");
				break;
			case YAML_ALIAS_EVENT:
				dbg(0, "YAML_ALIAS_EVENT");
				break;
			case YAML_SCALAR_EVENT:
				//dbg(0, "YAML_SCALAR_EVENT: value=%s %i plain=%i style=%i", event.data.scalar.value, event.data.scalar.length, event.data.scalar.plain_implicit, event.data.scalar.style);

				if(!key[0]){ // 1st half of a pair
					strncpy(key, (char*)event.data.scalar.value, 63);
				}else{
					// 2nd half of a pair
					dbg(2, "      %s=%s", key, event.data.scalar.value);
					key[0] = '\0';
				}
				break;
			case YAML_SEQUENCE_START_EVENT:
				dbg(2, "YAML_SEQUENCE_START_EVENT");
				break;
			case YAML_SEQUENCE_END_EVENT:
				dbg(2, "YAML_SEQUENCE_END_EVENT");
				break;
			case YAML_MAPPING_START_EVENT:
				depth++;
				if(key[0]){
					if(!section){
						dbg(2, "found section! %s", key);
						int i = 0;
						YamlHandler* h;
						while((h = &handlers[i]) && h->key ){
							if(!strcmp(h->key, key)){
								h->callback(&parser, &event, h->data);
								break;
							}
							i++;
						}
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
			case YAML_NO_EVENT:
				dbg(0, "YAML_NO_EVENT");
				break;
		}

		//the application is responsible for destroying the event object.
		yaml_event_delete(&event);

	} while(!end && safety++ < 1024);

	yaml_parser_delete(&parser);
	fclose(fp);

	return TRUE;

  error:
	yaml_parser_delete(&parser);
	fclose(fp);

	return FALSE;
}


void yaml_set_string (yaml_event_t* event, gpointer data)
{
	g_return_if_fail(event->type == YAML_SCALAR_EVENT);
	*((char**)data) = g_strdup((char*)event->data.scalar.value);
}

void
yaml_set_int (yaml_event_t* event, gpointer data)
{
	g_return_if_fail(event->type == YAML_SCALAR_EVENT);
	*((int*)data) = atoi((char*)event->data.scalar.value);
}

void
yaml_set_uint64 (yaml_event_t* event, gpointer data)
{
	g_return_if_fail(event->type == YAML_SCALAR_EVENT);
	*((uint64_t*)data) = strtoull((char*)(event->data.scalar.value), NULL, 10);
}
