/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <stdbool.h>
#include <yaml.h>
#include <glib.h>
#if __has_include ("agl/typedefs.h")
#include "agl/typedefs.h"
#endif

#define PLAIN_IMPLICIT true

#define yaml_start(fp, event) \
	if(!yaml_emitter_initialize(&emitter)){ perr("failed to initialise yaml writer."); goto out; } \
	yaml_emitter_set_output_file(&emitter, fp); \
	yaml_emitter_set_canonical(&emitter, false); \
	EMIT__(yaml_stream_start_event_initialize(event, YAML_UTF8_ENCODING), event); \
	EMIT__(yaml_document_start_event_initialize(event, NULL, NULL, NULL, 0), event); \
	EMIT__(yaml_mapping_start_event_initialize(event, NULL, (guchar*)"tag:yaml.org,2002:map", 1, YAML_BLOCK_MAPPING_STYLE), event);

#define map_open(E, A) \
	if(!yaml_scalar_event_initialize(E, NULL, str_tag, (guchar*)A, -1, PLAIN_IMPLICIT, 0, YAML_PLAIN_SCALAR_STYLE)) goto error; \
	if(!yaml_emitter_emit(&emitter, E)) goto error; \
	if(!yaml_mapping_start_event_initialize(E, NULL, map_tag, 1, YAML_BLOCK_MAPPING_STYLE)) goto error; \
	if(!yaml_emitter_emit(&emitter, E)) goto error; \

#define end_map(E) \
	if(!yaml_mapping_end_event_initialize(E)) goto error; \
	if(!yaml_emitter_emit(&emitter, E)) goto error;

#define sequence_open(E, A) \
	if(!yaml_scalar_event_initialize(E, NULL, str_tag, (guchar*)A, -1, PLAIN_IMPLICIT, 0, YAML_PLAIN_SCALAR_STYLE)) goto error; \
	if(!yaml_emitter_emit(&emitter, E)) goto error; \
	if(!yaml_mapping_start_event_initialize(E, NULL, seq_tag, 1, YAML_BLOCK_SEQUENCE_STYLE)) goto error; \
	if(!yaml_emitter_emit(&emitter, E)) goto error; \

#define end_sequence(E) \
	if(!yaml_mapping_end_event_initialize(E)) goto error; \
	if(!yaml_emitter_emit(&emitter, E)) goto error;

#define end_document(EM) \
	yaml_document_end_event_initialize(&event, 0); \
	yaml_emitter_emit(EM, &event); \
	yaml_stream_end_event_initialize(&event); \
	if(!yaml_emitter_emit(EM, &event)) goto error;

#define end_document_(EM, event) \
	yaml_document_end_event_initialize(event, 0); \
	yaml_emitter_emit(EM, event); \
	yaml_stream_end_event_initialize(event); \
	if(!yaml_emitter_emit(EM, event)) goto error;

#define EMIT(A) \
	if(!A) return FALSE; \
	if(!yaml_emitter_emit(&emitter, &event)) return FALSE;

#define EMIT_(A) \
	if(!A) goto error; \
	if(!yaml_emitter_emit(&emitter, &event)) goto error;

#define EMIT__(A, event) \
	if(!A) goto error; \
	if(!yaml_emitter_emit(&emitter, event)) goto error;

bool yaml_add_key_value_pair       (const char* key, const char*);
bool yaml_add_key_value_pair_int   (const char* key, int);
bool yaml_add_key_value_pair_float (const char* key, float);
bool yaml_add_key_value_pair_bool  (const char* key, bool);
bool yaml_add_key_value_pair_array (const char* key, int[], int size);
#ifdef __agl_typedefs_h__
bool yaml_add_key_value_pair_pt    (const char* key, AGliPt*);
#endif

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
extern unsigned char* seq_tag;
extern yaml_emitter_t emitter;
#endif
