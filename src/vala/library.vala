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

public class Library : DockItem
{
	construct {
	}

	~Library ()
	{
	}

	public override void realize ()
	{
		base.realize();
	}

	public override void unrealize ()
	{
	}

	public override void dispose ()
	{
		base.dispose();
	}

	public override void size_allocate (int a, int b, int c)
	{
		base.size_allocate(a, b, c);
	}
}

