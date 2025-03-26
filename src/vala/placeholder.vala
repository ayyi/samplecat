/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://samplecat.orford.org         |
 | copyright (C) 2025-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

using GLib;
using Gtk;
using Cairo;

public class Dock.Placeholder : Gtk.Widget
{
	public char name_[32];
	public GLib.ObjectClass* object_class;

	construct {
	}
}

