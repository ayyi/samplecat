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
#ifndef __yaml_utils_h__
#define __yaml_utils_h__

#include <yaml.h>
#include <glib.h>
#include "agl/typedefs.h"

#define PLAIN_IMPLICIT true

#define map_open_(E, A) \
		if(!yaml_scalar_event_initialize(E, NULL, str_tag, (guchar*)A, -1, PLAIN_IMPLICIT, 0, YAML_PLAIN_SCALAR_STYLE)) goto error; \
		if(!yaml_emitter_emit(&emitter, E)) goto error; \
		if(!yaml_mapping_start_event_initialize(E, NULL, map_tag, 1, YAML_BLOCK_MAPPING_STYLE)) goto error; \
		if(!yaml_emitter_emit(&emitter, E)) goto error; \

#define end_map(E) \
		if(!yaml_mapping_end_event_initialize(E)) goto error; \
		if(!yaml_emitter_emit(&emitter, E)) goto error;

#define end_document \
	yaml_document_end_event_initialize(&event, 0); \
	yaml_emitter_emit(&emitter, &event); \
	yaml_stream_end_event_initialize(&event); \
	if(!yaml_emitter_emit(&emitter, &event)) goto error;

#define EMIT(A) \
	if(!A) return FALSE; \
	if(!yaml_emitter_emit(&emitter, &event)) return FALSE;

#define EMIT_(A) \
	if(!A) goto error; \
	if(!yaml_emitter_emit(&emitter, &event)) goto error;


bool yaml_add_key_value_pair       (const char* key, const char*);
bool yaml_add_key_value_pair_int   (const char* key, int);
bool yaml_add_key_value_pair_float (const char* key, float);
bool yaml_add_key_value_pair_bool  (const char* key, bool);
bool yaml_add_key_value_pair_array (const char* key, int[], int size);
bool yaml_add_key_value_pair_pt    (const char* key, AGliPt*);

typedef void (*YamlValueCallback) (yaml_event_t*, gpointer);

typedef struct
{
	char*             key;
	YamlValueCallback callback;
	gpointer          value;
} YamlValueHandler;

#ifndef __yaml_utils_c__
extern unsigned char* str_tag;
extern unsigned char* map_tag;
extern yaml_emitter_t emitter;
#endif

#endif
