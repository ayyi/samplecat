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

public class Filter
{
	public char*  name;
	public string value;

	public signal void changed();

	public Filter(char* _name)
	{
		name = _name;
	}

	public void set_value(char* val)
	{
		value = (string)val;
		changed();
	}
}

public struct Filters
{
	Filter*    search;
	Filter*    dir;
	Filter*    category;
}

public class Idle
{
	uint id = 0;
	SourceFunc fn;

	public Idle(SourceFunc _fn)
	{
		fn = _fn;
	}

	public void queue()
	{
		if(!(bool)id){
			id = GLib.Idle.add(() => {
				fn();
				id = 0;
				return false;
			});
		}
	}
}

public class Model : GLib.Object
{
	public int state = 0;
	public char* cache_dir; // this belongs in Application?
	public Filters filters;
	public GLib.List<Filter*> filters_;
	public Sample* selection;

	private Idle idle;

	public signal void dir_list_changed();
	public signal void selection_changed(Sample* sample);
	public signal void sample_changed(Sample* sample, int what, void* data);

	construct
	{
	}

	public Model()
	{
		state = 1; //dummy
		cache_dir = Path.build_filename(Environment.get_home_dir(), ".config", PACKAGE, "cache", null);

		idle = new Idle(() => {
			dir_list_changed();
			return false;
		});
	}

	public bool add()
	{
		// TODO actually add something

		idle.queue();

		return true;
	}

	public bool remove(int id)
	{
		idle.queue();

		return Backend.remove(id);
	}

	public void set_search_dir(char* dir)
	{
		//if string is empty, all directories are shown

		filters.dir->set_value(dir);
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

	public void add_filter(Filter* filter)
	{
		filters_.prepend(filter);
	}
}
}

