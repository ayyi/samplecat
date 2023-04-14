/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://samplecat.orford.org         |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
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

public class Search : Gtk.Box
{
	construct {
		base.orientation = Gtk.Orientation.VERTICAL;
	}

	public Search ()
	{
	}

	public override void unrealize ()
	{
	}
}

