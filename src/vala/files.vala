/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://samplecat.orford.org         |
 | copyright (C) 2007-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

using GLib;
using Gtk;

[CCode(
   cheader_filename = "stdint.h",
   lower_case_cprefix = "", cprefix = "")]

public class Filemanager : DockItem
{
	public string _path;

	public string path {
		get {
			return _path;
		}
		set {
			_path = value;
		}
	}

	public override void constructed ()
	{
		base.constructed();
	}

	public override void unrealize ()
	{
		base.unrealize();
	}
}

