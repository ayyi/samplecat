/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. httpd://ayyi.github.io/samplecat/    |
 | copyright (C) 2020-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include "test/runner.h"
#include "utils/fs.h"
#include "yaml/load.h"
#include "yaml/save.h"

TestFn test_load, test_save_empty, test_save;

gpointer tests[] = {
	test_load,
	test_save_empty,
	test_save,
};


void
setup ()
{
	TEST.n_tests = G_N_ELEMENTS(tests);
}


void
teardown ()
{
}


void
test_load ()
{
	START_TEST;

	static bool have_section = false;
	static bool have_subsection = false;
	static bool have_property2 = false;
	static bool have_bad_property = false;
	static char* property1_value = NULL;

	bool property1 (const yaml_event_t* event, const char* key, gpointer)
	{
		property1_value = g_strdup((char*)event->data.scalar.value);
		return true;
	}

	bool property2 (const yaml_event_t*, const char* key, gpointer user_data)
	{
		return have_property2 = true;
	}

	bool bad_property (const yaml_event_t* event, const char* key, gpointer user_data)
	{
		return have_bad_property = true;
	}

	bool subsection (yaml_parser_t* parser, const yaml_event_t*, const char*, gpointer user_data)
	{
		have_subsection = true;

		return yaml_load_section(parser,
			(YamlHandler[]){
				{"property-2", property2, user_data},
				{NULL}
			},
			NULL,
			NULL,
			user_data
		);
	}

	bool section (yaml_parser_t* parser, const yaml_event_t* event, const char*, gpointer user_data)
	{
		have_section = true;

		return yaml_load_section(parser,
			(YamlHandler[]){
				{"property-1", property1, user_data},
				{"property-2", bad_property, user_data},
				{NULL}
			},
			(YamlMappingHandler[]){
				{NULL, subsection, user_data},
				{NULL}
			},
			NULL,
			user_data
		);
	}

	bool fn (FILE* fp, gpointer _master)
	{
		return yaml_load(fp, (YamlMappingHandler[]){
			{"section", section, NULL},
			{NULL}
		});
	}

	if (!with_fp("data/1.yaml", "rb", fn, NULL)) {
		FAIL_TEST("error loading");
	}

	assert(have_section, "expected section to be found");
	assert(have_subsection, "expected sub-section to be found");
	assert(property1_value && !strcmp(property1_value, "value-1"), "expected property-1 to have correct value");
	assert(have_property2, "expected property-2 to be found");
	assert(!have_bad_property, "expected bad property not to be found");

	FINISH_TEST;
}


static bool
files_match (const char* path)
{
	g_autofree gchar* contents1;
	g_file_get_contents ("/tmp/test.yaml", &contents1, NULL, NULL);

	g_autofree gchar* contents2;
	g_file_get_contents (path, &contents2, NULL, NULL);

	return !strcmp(contents1, contents2);
}


void
test_save_empty ()
{
	START_TEST;

	bool fn (FILE* fp, gpointer _master)
	{
		g_auto(yaml_emitter_t) emitter;
		g_auto(yaml_event_t) event;

		yaml_start(fp);

		end_map(&event);
		end_document;

		return true;

	  error:
	  out:
		return false;
	}

	assert(with_fp("/tmp/test.yaml", "wb", fn, NULL), "fp");
	assert(files_match ("data/empty.yaml"), "mismatch");

	FINISH_TEST;
}


void
test_save ()
{
	START_TEST;

	bool fn (FILE* fp, gpointer _master)
	{
		g_auto(yaml_event_t) event;

		yaml_start(fp);

		{
			map_open_(&event, "root");
			{
				map_open_(&event, "section");
				if (!yaml_add_key_value_pair("property-1", "value-1")) goto error;
				{
					map_open_(&event, "sub-section");
					if (!yaml_add_key_value_pair("property-2", "value-2")) goto error;
					end_map(&event);
				}
				end_map(&event);
			}
			end_map(&event);
		}

		end_map(&event);
		end_document;

		return true;

	  error:
	  out:
		return false;
	}

	assert(with_fp("/tmp/test.yaml", "wb", fn, NULL), "fp");
	assert(files_match ("data/1.yaml"), "mismatch");

	FINISH_TEST;
}
