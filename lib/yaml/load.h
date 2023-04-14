/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
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

typedef struct
{
	char*        key;
	bool         (*callback) (const yaml_event_t*, const char* key, gpointer);
	gpointer     data;
} YamlHandler;

typedef struct
{
	char*               key;
	bool                (*callback) (yaml_parser_t*, const yaml_event_t*, const char*, gpointer);
	gpointer            data;
} YamlMappingHandler;

typedef struct
{
	char*                key;
	bool                 (*callback) (yaml_parser_t*, const char*, gpointer);
	gpointer             data;
} YamlSequenceHandler;

bool yaml_load           (FILE*, YamlMappingHandler[]);
bool yaml_load_string    (const char*, YamlMappingHandler[]);
bool yaml_load_section   (yaml_parser_t*, YamlHandler[], YamlMappingHandler[], YamlSequenceHandler[], gpointer);

bool handle_scalar_event (yaml_parser_t*, const yaml_event_t*, YamlHandler[]);
bool handle_mapping_event (yaml_parser_t*, const yaml_event_t*, YamlMappingHandler[]);
bool handle_sequence_event (yaml_parser_t*, const yaml_event_t*, YamlSequenceHandler[]);

bool find_event          (yaml_parser_t*, yaml_event_t**, const char*);

bool yaml_set_string     (const yaml_event_t*, const char*, gpointer);
void yaml_set_int        (const yaml_event_t*, gpointer);
void yaml_set_uint64     (const yaml_event_t*, gpointer);
bool yaml_set_bool       (const yaml_event_t*, const char*, gpointer);

#define get_expected_event(parser, event, EVENT_TYPE) \
	if (!yaml_parser_parse(parser, event)) { \
		yaml_event_delete(event); \
		return false; \
	} \
	if ((event)->type != EVENT_TYPE) { \
		yaml_event_delete(event); \
		return false; \
	}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(yaml_event_t, yaml_event_delete)
G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(yaml_parser_t, yaml_parser_delete)
