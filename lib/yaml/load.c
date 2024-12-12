/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2007-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <stdint.h>
#include <glib.h>
#include "debug/debug.h"
#include "yaml/load.h"


static bool
_yaml_load (yaml_parser_t* parser, YamlHandler scalars[], YamlMappingHandler handlers[])
{
	yaml_event_t event;

	get_expected_event(parser, &event, YAML_STREAM_START_EVENT);
	yaml_event_delete(&event);
	get_expected_event(parser, &event, YAML_DOCUMENT_START_EVENT);
	yaml_event_delete(&event);
	get_expected_event(parser, &event, YAML_MAPPING_START_EVENT);
	yaml_event_delete(&event);

	return yaml_load_section(parser, scalars, handlers, NULL, NULL);
}


/*
 *  Each top-level section in the yaml file is passed to its matching handler
 */
bool
yaml_load (FILE* fp, YamlMappingHandler handlers[])
{
	g_auto(yaml_parser_t) parser;
	yaml_parser_initialize(&parser);

	yaml_parser_set_input_file(&parser, fp);

	return _yaml_load(&parser, NULL, handlers);
}


bool
yaml_load_string (const char* str, YamlMappingHandler handlers[])
{
	g_auto(yaml_parser_t) parser;
	yaml_parser_initialize(&parser);

	yaml_parser_set_input_string(&parser, (guchar*)str, strlen(str));

	return _yaml_load(&parser, NULL, handlers);
}


/*
 *  After having entered a new mapping, handle expected scalar and mapping events
 */
bool
yaml_load_section (yaml_parser_t* parser, YamlHandler scalars[], YamlMappingHandler mappings[], YamlSequenceHandler sequences[], gpointer)
{
	char key[64] = {0,};
	g_auto(yaml_event_t) event;

	while (yaml_parser_parse(parser, &event)) {
		switch (event.type) {
			case YAML_SCALAR_EVENT:
				dbg(2, "YAML_SCALAR_EVENT: value='%s' %li plain=%i style=%i", event.data.scalar.value, event.data.scalar.length, event.data.scalar.plain_implicit, event.data.scalar.style);

				if (!key[0]) {
					g_strlcpy(key, (char*)event.data.scalar.value, 64);
				} else {
					// 2nd half of a pair
					int i = 0;
					YamlHandler* h;
					while ((h = &scalars[i]) && h->callback) {
						if (!h->key || !strcmp(h->key, key)) {
							h->callback(&event, key, h->data);
							break;
						}
						i++;
					}
					key[0] = '\0';
				}
				break;
			case YAML_MAPPING_START_EVENT:
				dbg(2, "YAML_MAPPING_START_EVENT '%s'", key);
				g_return_val_if_fail(key[0], false);
				{
					int i = 0;
					YamlMappingHandler* h;
					while ((h = &mappings[i]) && h->callback) {
						if (!h->key || !strcmp(h->key, key)) {
							h->callback(parser, &event, key, h->data);
							break;
						}
						i++;
					}
				}
				key[0] = '\0';
				break;
			case YAML_MAPPING_END_EVENT:
				return true;
			case YAML_SEQUENCE_START_EVENT:
				dbg(2, "YAML_SEQUENCE_START_EVENT '%s'", key);
				g_return_val_if_fail(key[0], false);
				{
					int i = 0;
					YamlSequenceHandler* h;
					while ((h = &sequences[i]) && h->callback) {
						if (!h->key || !strcmp(h->key, key)) {
							h->callback(parser, key, h->data);
							break;
						}
						i++;
					}
				}
				key[0] = '\0';
				break;
			case YAML_SEQUENCE_END_EVENT:
				perr("YAML_SEQUENCE_END_EVENT never get here");
				return true;
			default:
				pwarn("unexpected event %i", event.type);
				return false;
		}

		yaml_event_delete(&event);
	}
	return false;
}


bool
handle_scalar_event (yaml_parser_t* parser, const yaml_event_t* event, YamlHandler handlers[])
{
	char* key = (char*)event->data.scalar.value;
	int i = 0;
	YamlHandler* h;
	while ((h = &handlers[i]) && h->callback) {
		if (!h->key || !strcmp(h->key, key)) {
			g_auto(yaml_event_t) event2;
			get_expected_event(parser, &event2, YAML_SCALAR_EVENT);

			h->callback(&event2, key, h->data);
			return true;
		}
		i++;
	}

	return false;
}


bool
handle_mapping_event (yaml_parser_t* parser, const yaml_event_t* event, YamlMappingHandler handlers[])
{
	char* key = (char*)event->data.scalar.value;

	int i = 0;
	YamlMappingHandler* h;
	while ((h = &handlers[i++]) && h->callback) {
		if (!h->key || !strcmp(h->key, key)) {
			g_auto(yaml_event_t) event2;
			get_expected_event(parser, &event2, YAML_MAPPING_START_EVENT);
			return h->callback(parser, &event2, key, h->data);
		}
	}

	return false;
}


bool
handle_sequence_event (yaml_parser_t* parser, const yaml_event_t* event, YamlSequenceHandler handlers[])
{
	char* key = (char*)event->data.scalar.value;

	int i = 0;
	YamlSequenceHandler* h;
	while ((h = &handlers[i]) && h->callback) {
		if (!h->key || !strcmp(h->key, key)) {
			g_auto(yaml_event_t) event2;
			get_expected_event(parser, &event2, YAML_SEQUENCE_START_EVENT);
			h->callback(parser, key, h->data);
			return true;
		}
		i++;
	}

	return false;
}


bool
find_event (yaml_parser_t* parser, yaml_event_t* event, const char* name)
{
	bool found = false;
	while (!found && yaml_parser_parse(parser, event)) {
		switch (event->type) {
			case YAML_SCALAR_EVENT:
				if (!strcmp((char*)event->data.scalar.value, name)) {
					found = true;
				}
				break;
			case YAML_NO_EVENT:
				yaml_event_delete(event);
				return false;
			default:
				break;
		}
		yaml_event_delete(event);
	}
	return found;
}


bool
find_sequence (yaml_parser_t* parser, yaml_event_t* event, const char* name)
{
	if (!find_event(parser, event, name)) return false;
	get_expected_event(parser, event, YAML_SEQUENCE_START_EVENT);
	return true;
}


bool
yaml_set_string (const yaml_event_t* event, const char*, gpointer data)
{
	g_return_val_if_fail(event->type == YAML_SCALAR_EVENT, false);
	*((char**)data) = g_strdup((char*)event->data.scalar.value);
	return true;
}


void
yaml_set_int (const yaml_event_t* event, gpointer data)
{
	g_return_if_fail(event->type == YAML_SCALAR_EVENT);
	*((int*)data) = atoi((char*)event->data.scalar.value);
}


void
yaml_set_uint64 (const yaml_event_t* event, gpointer data)
{
	g_return_if_fail(event->type == YAML_SCALAR_EVENT);
	*((uint64_t*)data) = strtoull((char*)(event->data.scalar.value), NULL, 10);
}


bool
yaml_set_bool (const yaml_event_t*, const char*, gpointer val)
{
	bool* val_ = val;
	*val_ = true;

	return true;
}
