/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2020-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#define __runner_c__

#include "config.h"
#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "samplecat/support.h"
#include "runner.h"

/*
 *  The test must provide the list of tests and the setup and teardown functions
 */
extern gpointer tests[];
extern void setup(char* argv[]);
extern void teardown();

void set_log_handlers ();
static void next_test ();


int
main (int argc, char* argv[])
{
#if 0 // unfortunately, gtk_test_init causes apps to abort on warnings
	const gchar* display = g_getenv("DISPLAY");
	if(display && strlen(display))
		gtk_test_init(&argc, &argv);
#endif

	set_log_handlers();

	setup (argv);

	dbg(2, "n_tests=%i", TEST.n_tests);

	gboolean fn (gpointer user_data) { next_test(); return G_SOURCE_REMOVE; }

	g_idle_add(fn, NULL);

	if (g_application_get_default()) {
		g_application_run (G_APPLICATION (g_application_get_default()), argc, argv);
	} else {
		g_main_loop_run (g_main_loop_new (NULL, 0));
	}

	exit(1);
}


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


static void
next_test ()
{
	printf("\n");

	TEST.current.test++;
	if (TEST.timeout) g_source_remove (TEST.timeout);
	if (TEST.current.test < TEST.n_tests) {
		TEST.current.finished = false;
		gboolean (*test)() = tests[TEST.current.test];
		dbg(2, "test %i of %i.", TEST.current.test + 1, TEST.n_tests);

		if (TEST.before_each) {
			void ready ()
			{
				g_timeout_add(1, run_test, tests[TEST.current.test]);
			}
			wait_for(TEST.before_each, ready, NULL);
		} else {
			g_timeout_add(300, run_test, test);
		}

		TEST.timeout = g_timeout_add(20000, on_test_timeout, NULL);
	} else {
		printf("finished all. passed=%s %i %s failed=%s %i %s\n", ayyi_green, TEST.n_passed, ayyi_white, (TEST.n_failed ? ayyi_red : ayyi_white), TEST.n_failed, ayyi_white);
		teardown();
		g_timeout_add(TEST.n_failed ? 4000 : 1000, __exit, NULL);
	}
}


void
test_finish ()
{
	dbg(2, "... passed=%i", passed);

	for (GList* l = TEST.current.timers; l; l = l->next) {
		g_source_remove(GPOINTER_TO_INT(l->data));
	}
	g_clear_pointer(&TEST.current.timers, g_list_free);

	if (passed) TEST.n_passed++; else TEST.n_failed++;
	if (!passed && abort_on_fail) TEST.current.test = 1000;

	next_test();
}


void
test_log_start (const char* func)
{
	printf("%srunning %i of %i: %s%s ...\n", ayyi_bold, TEST.current.test + 1, TEST.n_tests, func, ayyi_white);
}


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


void
test_errprintf (char* format, ...)
{
	char str[256];

	va_list argp;
	va_start(argp, format);
	vsprintf(str, format, argp);
	va_end(argp);

	printf("%s%s%s\n", ayyi_red, str, ayyi_white);
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
		GtkWidget* child = gtk_widget_get_first_child (widget);
		for (; child; child = gtk_widget_get_next_sibling (child)) {
			if (!strcmp(name, gtk_widget_get_name(child))) {
				*result = child;
				break;
			}
			find(child, result);

			if (*result) break;
		}
	}

	find (root, &result);

	return result;
}


GtkWidget*
find_widget_by_type (GtkWidget* root, GType type)
{
	GtkWidget* result = NULL;

	void find (GtkWidget* widget, GtkWidget** result)
	{
		GtkWidget* child = gtk_widget_get_first_child (widget);
		for (; child; child = gtk_widget_get_next_sibling (child)) {
			if (G_TYPE_CHECK_INSTANCE_TYPE((child), type)) {
				*result = child;
				break;
			}
			find(child, result);

			if (*result) break;
		}
	}

	find (root, &result);

	return result;
}
