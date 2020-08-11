/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://www.ayyi.org           |
* | copyright (C) 2007-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __yaml_load_h__
#define __yaml_load_h__

#include <stdbool.h>
#include <yaml.h>

typedef bool (*YamlCallback)        (yaml_parser_t*, yaml_event_t*, gpointer);
typedef bool (*YamlMappingCallback) (yaml_parser_t*, yaml_event_t*, char*, gpointer);

typedef struct
{
	char*        key;
	YamlCallback callback;
	gpointer     data;
} YamlHandler;

typedef struct
{
	char*               key;
	YamlMappingCallback callback;
	gpointer            data;
} YamlMappingHandler;

bool yaml_load           (FILE*, YamlHandler[]);
bool load_mapping        (yaml_parser_t*, yaml_event_t*, YamlHandler[], YamlMappingHandler[], gpointer);
bool handle_scalar_event (yaml_parser_t*, yaml_event_t*, YamlHandler[]);
bool find_event          (yaml_parser_t*, yaml_event_t*, const char*);

void yaml_set_string     (yaml_event_t*, gpointer);
void yaml_set_int        (yaml_event_t*, gpointer);
void yaml_set_uint64     (yaml_event_t*, gpointer);

#define get_expected_event(parser, event, EVENT_TYPE) \
	if(!yaml_parser_parse(parser, event)) return false; \
	if((event)->type != EVENT_TYPE) return false;

#endif
