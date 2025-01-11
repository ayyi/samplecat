/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include <stdbool.h>
#include <stdio.h>
#include <glib.h>
#include <glib-object.h>
#define __wf_private__
#include "debug/debug.h"

typedef void (TestFn)();
typedef void (*Test) ();

typedef bool (*ReadyTest)    (gpointer);
typedef void (*WaitCallback) (gpointer);

#ifdef __runner_c__
bool passed = false;
bool abort_on_fail = true;
#else
extern bool passed;
extern bool abort_on_fail;
#endif

typedef struct {
	int n_tests;
	int n_passed;
	int n_failed;
	int timeout;
	ReadyTest before_each;
	struct {
		int    test;
		char   name[64];
		bool   finished;  // current test has finished. Go onto the next test.
		GList* timers;
	}   current;
} Test_t;

#ifdef __runner_c__
Test_t TEST = {.current = {-1}};
#else
extern Test_t TEST;
#endif

// public fns
void       test_errprintf      (char* format, ...);
void       test_log_start      (const char* func);
void       wait_for            (ReadyTest, WaitCallback, gpointer);
#ifdef GTK_TYPE_WIDGET
GtkWidget* find_widget_by_name (GtkWidget*, const char*);
GtkWidget* find_widget_by_type (GtkWidget*, GType);
#endif

// private fns
void       test_finish      ();

#define START_TEST \
	static int step = 0;\
	static int __test_idx; \
	__test_idx = TEST.current.test; \
	if(!step){ \
		g_strlcpy(TEST.current.name, __func__, 64); \
		test_log_start(__func__); \
	} \
	if(TEST.current.finished) return;

#define FINISH_TEST \
	{ \
	if(__test_idx != TEST.current.test) return; \
	printf("%s: finish\n", TEST.current.name); \
	TEST.current.finished = true; \
	passed = true; \
	test_finish(); \
	return; \
	}

#define FINISH_TEST_TIMER_STOP \
	if(__test_idx != TEST.current.test) return G_SOURCE_REMOVE; \
	TEST.current.finished = true; \
	passed = true; \
	test_finish(); \
	return G_SOURCE_REMOVE;

#define FAIL_TEST(msg, ...) \
	{TEST.current.finished = true; \
	passed = false; \
	test_errprintf(msg, ##__VA_ARGS__); \
	test_finish(); \
	return; }

#define FAIL_TEST_TIMER(msg) \
	{TEST.current.finished = true; \
	passed = false; \
	printf("%s%s%s\n", ayyi_red, msg, ayyi_white); \
	test_finish(); \
	return G_SOURCE_REMOVE;}

#define FAIL_TEST_NULL(msg) \
	{TEST.current.finished = true; \
	passed = false; \
	printf("%s%s%s\n", ayyi_red, msg, white); \
	test_finish(); \
	return NULL;}

#define assert(A, B, ...) \
	{bool __ok_ = ((A) != 0); \
	{if(!__ok_) perr(B, ##__VA_ARGS__); } \
	{if(!__ok_) FAIL_TEST("assertion failed") }}

#define assert_and_stop(A, B, ...) \
	{bool __ok_ = ((A) != 0); \
	{if(!__ok_) perr(B, ##__VA_ARGS__); } \
	{if(!__ok_) FAIL_TEST_TIMER("assertion failed") }}

#define assert_null(A, B, ...) \
	{bool __ok_ = ((A) != 0); \
	{if(!__ok_) perr(B, ##__VA_ARGS__); } \
	{if(!__ok_) FAIL_TEST_NULL("assertion failed") }}

