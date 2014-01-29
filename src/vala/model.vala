/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2014 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
using Config;
using GLib;
using Gtk;
using Samplecat;

namespace Samplecat
{

public struct Filters
{
	char       phrase[256]; // XXX TODO increase to PATH_MAX
	char*      dir;
	char*      category;
}

public class Model : GLib.Object
{
	public int state = 0;
	public char* cache_dir;
	public Filters filters;
	public Sample* selection;

	public signal void selection_changed(Sample* sample);
	public signal void sample_changed(Sample* sample, int what, void* data);

	construct
	{
	}

	public Model()
	{
		state = 1; //dummy
		cache_dir = Path.build_filename(Environment.get_home_dir(), ".config", PACKAGE, "cache", null);
//		filters.phrase[0] = '\0';
	}

	public void set_search_dir(char* dir)
	{
		//this doesnt actually do the search. When calling, follow with do_search() if neccesary.

		//if string is empty, we show all directories?

		app.model->filters.dir = dir;
	}

	public void set_selection(Sample* sample)
	{
		if(sample != selection){
			if((bool)selection) selection->unref();

			selection = (owned) sample;
			selection->ref();

			selection_changed(selection);
		}
	}
}
}

