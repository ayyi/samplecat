/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://samplecat.orford.org          |
* | copyright (C) 2007-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

using GLib;
using Gtk;
using Cairo;

[CCode(
   cheader_filename = "stdint.h",
   lower_case_cprefix = "", cprefix = "")]

public class Inspector : Gtk.ScrolledWindow
{
	public bool  show_waveform;

	private uint row_id;

	construct {
	}

	public override void unrealize ()
	{
	}

	public override void size_request (out Gtk.Requisition requisition)
	{
		requisition.width = 10;
		requisition.height = 80;

		Gtk.WidgetFlags flags = this.get_flags();
		if((flags & Gtk.WidgetFlags.REALIZED) != 0){
		}
	}

	public override void size_allocate (Gdk.Rectangle allocation)
	{
		base.size_allocate(allocation);
	}

	public override bool expose_event (Gdk.EventExpose event)
	{
		return true;
	}
}

