/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2004-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#define __yaml_save_c__

#include "config.h"
#include <stdint.h>
#include <yaml.h>
#include "debug/debug.h"
#include "yaml/save.h"

unsigned char* str_tag = (unsigned char*)"tag:yaml.org,2002:str";
unsigned char* map_tag = (unsigned char*)"tag:yaml.org,2002:map";
unsigned char* seq_tag = (unsigned char*)"tag:yaml.org,2002:seq";
yaml_emitter_t emitter;


bool
yaml_add_key_value_pair (const char* key, const char* value)
{
	yaml_event_t event;

	if (!value) value = "";

	EMIT(yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)key, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE));
	EMIT(yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)value, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE));

	return true;
}


bool
yaml_add_key_value_pair_int (const char* key, int ival)
{
	yaml_event_t event;
	char value[256];
	snprintf(value, 255, "%i", ival);

	EMIT(yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)key, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE));
	EMIT(yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)value, -1, 1, 0, YAML_ANY_SCALAR_STYLE));

	return true;
}


bool
yaml_add_key_value_pair_float (const char* key, float fval)
{
	yaml_event_t event;
	char value[256];
	snprintf(value, 255, "%.2f", fval);

	if (!yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)key, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE)) return false;
	if (!yaml_emitter_emit(&emitter, &event)) return FALSE;
	if (!yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)value, -1, 0, 1, YAML_PLAIN_SCALAR_STYLE)) return false;
	if (!yaml_emitter_emit(&emitter, &event)) return FALSE;

	return true;
}


bool
yaml_add_key_value_pair_bool (const char* key, bool val)
{
	yaml_event_t event;
	char value[256];
	snprintf(value, 255, "%s", val ? "true" : "false");

	if(!yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)key, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE)) return FALSE;
	if(!yaml_emitter_emit(&emitter, &event)) return false;
	if(!yaml_scalar_event_initialize(&event, NULL, (guchar*)YAML_BOOL_TAG, (guchar*)value, -1, 1, 0, YAML_ANY_SCALAR_STYLE)) return FALSE;
	if(!yaml_emitter_emit(&emitter, &event)) return false;

	return true;
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

	return true;
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

	for (int i=0;i<2;i++) {
		snprintf(value, 15, "%i", i ? pt->y : pt->x);
		EMIT(yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)value, -1, 1, 0, YAML_ANY_SCALAR_STYLE));
	}

	EMIT(SEQUENCE_END_EVENT_INIT(event, mark, mark));

	yaml_event_delete(&event);

	return true;
}

