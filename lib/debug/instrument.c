/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2024-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#define _GNU_SOURCE

#include "config.h"
#include <dlfcn.h>
#include <stdio.h>
#include <sys/time.h>
#include <glib.h>

#define MAX_FNS 64000
#define STACK_SIZE 20

typedef struct {
   void*  func_address;
   char*  name;
   double start;
   double end;
   GArray* children;
} Call;

typedef struct {
   GArray** array;
   int      index;
} CallRef;

typedef struct {
   int    depth;
   CallRef stack[STACK_SIZE];
} GraphIter;

static Call root;
static GraphIter iter;

static double t0;
static int _i;

#define NOW() ({ \
	struct timeval t; \
	gettimeofday(&t, 0); \
	(t.tv_sec + t.tv_usec * 0.000001) * 1000; \
})

void main_constructor (void) __attribute__ ((no_instrument_function, constructor));
void main_destructor (void) __attribute__ ((no_instrument_function, destructor));
void __cyg_profile_func_enter (void* func_address, void* call_site ) __attribute__ ((no_instrument_function));
void __cyg_profile_func_exit  (void* func_address, void* call_site ) __attribute__ ((no_instrument_function));
void instrumentation_print ();
void instrumentation_free();


void
main_constructor (void)
{
	t0 = NOW();

	root.start = root.end = t0;
	root.children = g_array_new(FALSE, TRUE, sizeof(Call));

	Call call = (Call){ NULL, g_strdup("root"), t0, t0, g_array_new(FALSE, TRUE, sizeof(Call)) };
	g_array_append_val(root.children, call);

	iter.stack[0] = (CallRef){&root.children};
}


void
main_destructor (void)
{
}


static inline void
iter_down (CallRef ref)
{
	g_return_if_fail(ref.array);

	iter.depth++;
	g_assert(!iter.stack[iter.depth].array);
	iter.stack[iter.depth] = ref;
}


static void
iter_up ()
{
	iter.stack[iter.depth] = (CallRef){NULL,};
	iter.depth--;
}


static inline Call*
iter_current ()
{
	return &g_array_index(*iter.stack[iter.depth].array, Call, iter.stack[iter.depth].index);
}


void
__cyg_profile_func_enter (void* func_address, void *callsite)
{
	Dl_info info;

	if (_i++ < MAX_FNS && root.children)
		if (dladdr(func_address, &info) && info.dli_sname) {
			if (iter.depth < STACK_SIZE - 1) {
				double t = NOW() - t0;

				Call* current_call = iter_current();

				Call call = (Call){ func_address, g_strdup(info.dli_sname), t, t, g_array_new(FALSE, TRUE, sizeof(Call)) };
				g_array_append_val(current_call->children, call);
				iter_down((CallRef){&current_call->children, current_call->children->len - 1});
			} else {
				iter.depth++;
			}
		}
}


void
__cyg_profile_func_exit (void* func_address, void* callsite)
{
	Dl_info info;

	if (_i < MAX_FNS && root.children)
		if (dladdr(func_address, &info))
			if (info.dli_sname) {
				if (!(iter.depth < STACK_SIZE)) {
					iter.depth--;
					return;
				}

				double t = NOW() - t0;
				iter_current()->end = t;
				iter_up();
			}
}


void
instrumentation_print ()
{
	g_return_if_fail(root.children);

	static int depth; depth = 0;

	void print_item (Call* call)
	{
		g_return_if_fail(call);
		g_return_if_fail(call->children);

		char f[64];
		sprintf(f, "%%s %%%is %%s\n", depth * 2);
		char time[16] = {0,};
		{
			double dt = call->end - call->start;
			if (dt > 0) snprintf(time, 15, "%6.3f", dt); else g_strlcpy(time, " -----", 32);
		}
		printf(f, time, "", call->name);

		depth++;
		for (int i=0;i<call->children->len;i++) {
			print_item(&g_array_index(call->children, Call, i));
		}
		depth--;
	}

	extern void underline();
	underline();

	Call* top = &g_array_index(root.children, Call, 0);
	print_item(top);

	underline();

	instrumentation_free();
}


void
instrumentation_free ()
{
	static int depth; depth = 0;

	void free_item (Call* call)
	{
		depth++;
		for (int i=0;i<call->children->len;i++) {
			free_item(&g_array_index(call->children, Call, i));
		}
		depth--;

		g_clear_pointer(&call->name, g_free);
	}

	free_item(&g_array_index(root.children, Call, 0));

	g_clear_pointer(&root.children, g_array_unref);
}
