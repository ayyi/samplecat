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
#ifndef __yaml_utils_h__
#define __yaml_utils_h__

#include <yaml.h>
#include <glib.h>
#include "agl/typedefs.h"

#define end_map \
		if(!yaml_mapping_end_event_initialize(&event)) goto error; \
		if(!yaml_emitter_emit(&emitter, &event)) goto error;

#define map_open(A) \
		if(!yaml_scalar_event_initialize(&event, NULL, str_tag, (guchar*)A, -1, plain_implicit, 0, YAML_PLAIN_SCALAR_STYLE)) goto error; \
		if(!yaml_emitter_emit(&emitter, &event)) goto error; \
		if(!yaml_mapping_start_event_initialize(&event, NULL, map_tag, 1, YAML_BLOCK_MAPPING_STYLE)) goto error; \
		if(!yaml_emitter_emit(&emitter, &event)) goto error; \

#define end_document \
	yaml_document_end_event_initialize(&event, 0); \
	yaml_emitter_emit(&emitter, &event); \
	yaml_stream_end_event_initialize(&event); \
	if(!yaml_emitter_emit(&emitter, &event)) goto error;

gboolean yaml_add_key_value_pair       (const char* key, const char*);
gboolean yaml_add_key_value_pair_int   (const char* key, int);
gboolean yaml_add_key_value_pair_float (const char* key, float);
gboolean yaml_add_key_value_pair_bool  (const char* key, gboolean);
gboolean yaml_add_key_value_pair_array (const char* key, int[], int size);
gboolean yaml_add_key_value_pair_pt    (const char* key, AGliPt*);

typedef void   (*YamlCallback)         (yaml_parser_t*, const char* key, gpointer);
typedef void   (*YamlValueCallback)    (yaml_event_t*, gpointer);

typedef struct _yh
{
	char*        key;
	YamlCallback callback;
	gpointer     data;
} YamlHandler;

typedef struct _yvh
{
	char*             key;
	YamlValueCallback callback;
	gpointer          value;
} YamlValueHandler;

void     yaml_set_string               (yaml_event_t*, gpointer);
void     yaml_set_int                  (yaml_event_t*, gpointer);
void     yaml_set_uint64               (yaml_event_t*, gpointer);

#ifndef __yaml_utils_c__
extern unsigned char* str_tag;
extern unsigned char* map_tag;
extern yaml_emitter_t emitter;
#endif

#endif
