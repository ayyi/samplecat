/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |                                                                      |
 | Now that Gtk does not support sending events to the application      |
 | for testing, this file temporarily provides access to private        |
 | functions until a better solution can be found.                      |
 |                                                                      |
 +----------------------------------------------------------------------+
 |
 */

GtkWidget*
test_get_context_menu ()
{
	GtkWidget* widget = (GtkWidget*)gtk_application_get_active_window(GTK_APPLICATION(app));

	if (!window.context_menu)
		window.context_menu = make_context_menu(widget);

	return window.context_menu;
}
