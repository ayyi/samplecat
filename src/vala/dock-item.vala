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

[CCode(
   cheader_filename = "stdint.h",
   lower_case_cprefix = "", cprefix = "")]

public class DockItem : Gtk.Widget, Gtk.ShortcutManager
{
	public bool property1;

	private uint row_id;

	construct {
	}

	public new void add_controller (Gtk.ShortcutController controller)
	{
	}

	public new void remove_controller (Gtk.ShortcutController controller)
	{
	}

	public override void unrealize ()
	{
	}
}
