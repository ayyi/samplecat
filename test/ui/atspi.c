/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2025-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <atspi/atspi.h>
#include "debug/debug.h"
#include "atspi.h"

static gint pid;
static AtspiAccessible* desktop;
static AtspiAccessible* application;

void
find_actions (AtspiAccessible* accessible)
{
	AtspiAction* ai = atspi_accessible_get_action_iface(accessible);
	gint n_actions = ai ? atspi_action_get_n_actions (ai, NULL) : 0;
	for (int i=0;i<n_actions;i++) {
		gchar* action = atspi_action_get_action_name(ai, i, NULL);
		if (!strcmp(action, "click")) {
			g_autofree char* name = atspi_accessible_get_name(accessible, NULL);
			AtspiRole role = atspi_accessible_get_role(accessible, NULL);
			printf(" * %s (%s)\n", name, role ? atspi_role_get_name(role) : "");
		}
	}

	int child_count = atspi_accessible_get_child_count(accessible, NULL);
		for (int i = 0; i < child_count; i++) {
			AtspiAccessible* child = atspi_accessible_get_child_at_index(accessible, i, NULL);
			if (child) {
				find_actions(child);
			g_object_unref(child);
		}
	}
}

static AtspiAccessible*
find_by_name (AtspiAccessible* parent, const char* target_name)
{
	int child_count = atspi_accessible_get_child_count(parent, NULL);
	for (int i = 0; i < child_count; i++) {
		AtspiAccessible* child = atspi_accessible_get_child_at_index(parent, i, NULL);
		if (child) {
			g_autofree char* name = atspi_accessible_get_name(child, NULL);
			if (!strcmp(target_name, name)) {
				return child;
			}
			AtspiAccessible* grandchild = find_by_name(child, target_name);
			if (grandchild) {
				return grandchild;
			}
			g_object_unref(child);
		}
	}
	return NULL;
}

static AtspiAccessible*
find_by_role (AtspiAccessible* parent, AtspiRole target_role)
{
	int child_count = atspi_accessible_get_child_count(parent, NULL);
	for (int i = 0; i < child_count; i++) {
		AtspiAccessible* child = atspi_accessible_get_child_at_index(parent, i, NULL);
		if (child) {
			AtspiRole role = atspi_accessible_get_role(child, NULL);
			if (role == target_role) {
				return child;
			}
			g_object_unref(child);
		}
	}
	return NULL;
}

static AtspiAccessible*
find_by_panel_and_role (const char* panel_name, AtspiRole role)
{
	AtspiAccessible* panel = find_by_name(desktop, panel_name);
	if (panel) {
		return find_by_role(panel, role);
	}
	return NULL;
}

int
setup ()
{
	g_autofree char* cwd = g_get_current_dir();
	char* command = g_build_filename(cwd, "../../src/samplecat", NULL);

	g_autoptr(GError) error = NULL;
	if (g_spawn_async(NULL, &command, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, &error)) {
		g_print("Application launched with PID: %d\n", pid);
	} else {
		g_printerr("Failed to launch application: %s\n", error->message);
		return 1;
	}

	void on_samplecat_exit (GPid pid, gint wait_status, gpointer user_data)
	{
		PF0;
	}
	g_child_watch_add(pid, on_samplecat_exit, NULL);

	g_usleep(1000000);

	atspi_init();

	desktop = atspi_get_desktop(0);
	if (!desktop) {
		perr("Failed to get the accessibility desktop");
		return 1;
	}

	AtspiAccessible* get_application (const char* target_app_name)
	{
		int desktop_child_count = atspi_accessible_get_child_count(desktop, NULL);
		for (int i = 0; i < desktop_child_count; i++) {
			AtspiAccessible *child = atspi_accessible_get_child_at_index(desktop, i, NULL);
			if (child) {
				g_autofree char* child_name = atspi_accessible_get_name(child, NULL);
				if (child_name && strcmp(child_name, target_app_name) == 0) {
					return child;
				}
				g_object_unref(child);
			}
		}
		return NULL;
	}

	application = ({
		AtspiAccessible* application = NULL;
		for (int i = 0; !application && i < 10; i++) {
			application = get_application("samplecat");
			g_usleep(500000);
		}
		application;
	});

	if (!application) {
		fprintf(stderr, "Application not found on bus.\n");
		return 1;
	}

	return 0;
}

void
teardown ()
{
	if (pid > 0) {
		// Send SIGTERM to samplecat
		g_autoptr(GError) error = NULL;
		g_autofree char* pidstr = g_strdup_printf("%d", pid);
		g_spawn_async(NULL, (gchar*[]){ "kill", pidstr, NULL }, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);
		if (error) g_printerr("%s\n", error->message);
	}

	g_object_unref(application);
	g_object_unref(desktop);
}

void
test_tags ()
{
	START_TEST;

	find_actions(application);
	assert(find_by_panel_and_role("Tags", ATSPI_ROLE_TEXT), "Tags entry not found");

	FINISH_TEST;
}
