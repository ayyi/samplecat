/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2020-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

#define __runner_c__

#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include "debug/debug.h"
#include "src/support.h"
#include "runner.h"

/*
 *  The test must provide the list of tests and the setup and teardown functions
 */
extern gpointer tests[];
extern void setup();
extern void teardown();


static gboolean
__exit ()
{
	exit(TEST.n_failed ? EXIT_FAILURE : EXIT_SUCCESS);
	return G_SOURCE_REMOVE;
}


static gboolean
run_test (gpointer test)
{
	((Test)test)();
	return G_SOURCE_REMOVE;
}


static gboolean
on_test_timeout (gpointer _user_data)
{
	FAIL_TEST_TIMER("TEST TIMEOUT\n");
	return G_SOURCE_REMOVE;
}


void
next_test ()
{
	printf("\n");
	TEST.current.test++;
	if(TEST.timeout) g_source_remove (TEST.timeout);
	if(TEST.current.test < TEST.n_tests){
		TEST.current.finished = false;
		gboolean (*test)() = tests[TEST.current.test];
		dbg(2, "test %i of %i.", TEST.current.test + 1, TEST.n_tests);
		g_timeout_add(300, run_test, test);

		TEST.timeout = g_timeout_add(20000, on_test_timeout, NULL);
	} else {
		printf("finished all. passed=%s %i %s failed=%s %i %s\n", green, TEST.n_passed, white, (TEST.n_failed ? red : white), TEST.n_failed, white);
		teardown();
		g_timeout_add(TEST.n_failed ? 4000 : 1000, __exit, NULL);
	}
}


void
test_finished_ ()
{
	dbg(2, "... passed=%i", passed);

	for(GList* l = TEST.current.timers; l; l = l->next){
		g_source_remove(GPOINTER_TO_INT(l->data));
	}
	g_clear_pointer(&TEST.current.timers, g_list_free);

	if(passed) TEST.n_passed++; else TEST.n_failed++;
	if(!passed && abort_on_fail) TEST.current.test = 1000;

	next_test();
}


void
test_log_start (const char* func)
{
	printf("%srunning %i of %i: %s%s ...\n", bold, TEST.current.test + 1, TEST.n_tests, func, white);
}


static gboolean fn(gpointer user_data) { next_test(); return G_SOURCE_REMOVE; }

void
set_log_handlers ()
{
	void log_handler (const gchar* log_domain, GLogLevelFlags log_level, const gchar* message, gpointer user_data)
	{
	  switch(log_level){
		case G_LOG_LEVEL_CRITICAL:
		  printf("%s %s\n", ayyi_err, message);
		  break;
		case G_LOG_LEVEL_WARNING:
		  printf("%s %s\n", ayyi_warn, message);
		  break;
		default:
		  printf("log_handler(): level=%i %s\n", log_level, message);
		  break;
	  }
	}

	g_log_set_handler (NULL, G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, log_handler, NULL);

	char* domain[] = {NULL, "Waveform", "GLib-GObject", "GLib", "Gdk", "Gtk", "AGl"};
	for(int i=0;i<G_N_ELEMENTS(domain);i++){
		g_log_set_handler (domain[i], G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, log_handler, NULL);
	}
}


int
main (int argc, char* argv[])
{
	const gchar* display = g_getenv("DISPLAY");
	if(display && strlen(display))
		gtk_test_init(&argc, &argv);

	set_log_handlers();

	setup();

	dbg(2, "n_tests=%i", TEST.n_tests);

	g_idle_add(fn, NULL);

	g_main_loop_run (g_main_loop_new (NULL, 0));

	exit(1);
}


void
test_errprintf (char* format, ...)
{
	char str[256];

	va_list argp;
	va_start(argp, format);
	vsprintf(str, format, argp);
	va_end(argp);

	printf("%s%s%s\n", red, str, white);
}


void
wait_for (ReadyTest test, WaitCallback on_ready, gpointer user_data)
{
	typedef struct {
		ReadyTest    test;
		int          i;
		WaitCallback on_ready;
		gpointer     user_data;
	} C;

	gboolean _check (C* c)
	{
		if(c->test(c->user_data)){
			TEST.current.timers = g_list_remove(TEST.current.timers, GINT_TO_POINTER(g_source_get_id(g_main_current_source())));
			c->on_ready(c->user_data);
			g_free(c);
			return G_SOURCE_REMOVE;
		}

		if(c->i++ > 100){
			TEST.current.timers = g_list_remove(TEST.current.timers, GINT_TO_POINTER(g_source_get_id(g_main_current_source())));
			g_free(c);
			return G_SOURCE_REMOVE;
		}

		return G_SOURCE_CONTINUE;
	}

	TEST.current.timers = g_list_prepend(TEST.current.timers, GINT_TO_POINTER(
		g_timeout_add(100, (GSourceFunc)_check, SC_NEW(C,
			.test = test,
			.on_ready = on_ready,
			.user_data = user_data
		)
	)));
}


GtkWidget*
find_widget_by_name (GtkWidget* root, const char* name)
{
	GtkWidget* result = NULL;

	void find (GtkWidget* widget, GtkWidget** result)
	{
		if(GTK_IS_CONTAINER(widget)){
			GList* children = gtk_container_get_children(GTK_CONTAINER(widget));
			for(GList* l = children; l; l = l->next){
				if(!strcmp(name, gtk_widget_get_name(l->data))){
					*result = l->data;
					break;
				}
				find(l->data, result);

				if(*result) break;
			}
		}
	}

	find (root, &result);

	return result;
}

