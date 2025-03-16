/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://samplecat.orford.org          |
 | copyright (C) 2021-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */
using GLib;
using Gtk;
using AGl;
using GL;

[CCode(
   cheader_filename = "stdint.h",
   cheader_filename = "spectrogram.h",
   lower_case_cprefix = "", cprefix = "")]

public delegate void SpectrogramReady (char* filename, Gdk.Pixbuf* a, void* user_data_);
 
public class SpectrogramArea : AGlGtkArea
{
	public static SpectrogramArea instance;
	private AGl.Instance* agl;

	private string _filename;
	private Gdk.Pixbuf* pixbuf = null;
	private bool gl_init_done = false;
	private GL.GLuint textures[2];
	private const double near = 10.0;
	private const double far = -100.0;

	public SpectrogramArea ()
	{
	}

	~SpectrogramArea ()
	{
	}

	public override void unrealize ()
	{
		base.unrealize();
	}

	public override void dispose ()
	{
		base.dispose();
	}
}

