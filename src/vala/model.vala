/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
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

	public Filter (char* _name)
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

	public Idle (SourceFunc _fn)
	{
		fn = _fn;
	}

	public void queue()
	{
		if (!(bool)id) {
			id = GLib.Idle.add(() => {
				fn();
				id = 0;
				return false;
			});
		}
	}
}

public struct SampleChange
{
	public Sample* sample;
	public int     prop;
	public void*   val;
}

public class Model : GLib.Object
{
	public int state = 0;
	public char* cache_dir; // this belongs in Application?
	public GLib.List<char*> backends;
	public Filters filters;
	public GLib.List<Filter*> filters_;

	private Idle idle;
	private Idle sample_changed_idle;
	public GLib.List<SampleChange*> modified;
	private static char unk[32];
	private uint selection_change_timeout;

	public signal void dir_list_changed (Node node);
	public signal void selection_changed (Sample* sample);
	public signal void sample_changed (Sample* sample, int what, void* data);

	public Sample* selection { get; set; }

	construct
	{
	}

	public Model ()
	{
		state = 1; //dummy
		cache_dir = Path.build_filename(Environment.get_home_dir(), ".config", PACKAGE, "cache", null);
		Node node = null;

		idle = new Idle(() => {
			dir_list_changed(node);
			return false;
		});

		sample_changed_idle = new Idle(() => {
			foreach (SampleChange* change in modified) {
				sample_changed(change.sample, change.prop, change.val);
				free(change);
			};
			modified = null;
			return false;
		});
	}

	public bool add ()
	{
		idle.queue();

		return true;
	}

	public bool remove (int id)
	{
		idle.queue();

		return Backend.remove(id);
	}

	public void set_search_dir (char* dir)
	{
		//if string is empty, all directories are shown

		filters.dir->set_value(dir);
	}

	/*
	public void set_selection (Sample* sample)
	{
		if (sample != selection) {
			if ((bool)selection) selection->unref();

			selection = (owned) sample;
			selection->ref();

			if((bool)selection_change_timeout) Source.remove(selection_change_timeout);
			selection_change_timeout = GLib.Timeout.add(250, queue_selection_changed);
		}
	}
	*/

	private bool queue_selection_changed ()
	{
		selection_change_timeout = 0;
		selection_changed(selection);
		return false;
	}

	private void queue_sample_changed(Sample* sample, int prop, void* val)
	{
		foreach (SampleChange* change in modified) {
			if (change.sample == sample) {
				// when merging multiple change signals it is not possible to specify the change type
				change.prop = -1;
				change.val = null;
				return;
			}
		}
		SampleChange* c = GLib.malloc(sizeof(SampleChange));
		*c = SampleChange(){sample = sample, prop = prop, val = val};
		modified.append(c);
		sample_changed_idle.queue();
	}

	public void add_filter(Filter* filter)
	{
		filters_.prepend(filter);
	}

	public void refresh_sample (Sample* sample, bool force_update)
	{
		bool online = sample->online;

		time_t mtime = file_mtime(sample->full_path);
		if (mtime > 0) {
			if (sample->mtime < mtime || force_update) {
				/* file may have changed - FULL UPDATE */
				//dbg(0, "file modified: full update: %s", sample->full_path);

				// re-check mime-type
				Sample test = new Sample.from_filename(sample->full_path);
				if (!(bool)test) return;

				if (sample->get_file_info()) {
					sample->mtime = mtime;
					sample_changed(sample, -1, null);

					sample->request_peaklevel();
					sample->request_overview();
					sample->request_ebur128();
				} else {
					dbg(0, "full update - reading file info failed!");
				}
			}
			sample->mtime = mtime;
			sample->online = true;
		}else{
			/* file does not exist */
			sample->online = false;
		}
		if(sample->online != online) update_sample(sample, Column.ICON, null);
	}

	public bool update_sample(Sample* sample, int prop, void* val)
	{
		// use prop=-1 to update all properties.
		// ownership of val is not taken.

		bool ok = false;

		switch(prop){
			case Column.ICON: // online/offline, mtime
				if(Backend.update_int(sample->id, "online", sample->online ? 1 : 0)){
					ok = Backend.update_int(sample->id, "mtime", (uint)sample->mtime);
				}
				break;
			case Column.KEYWORDS:
				if((ok = Backend.update_string(sample->id, "keywords", (char*)val))) {
					sample->keywords = (string)val;
				}
				break;
			case Column.OVERVIEW:
				if((bool)sample->overview){
					uint len;
					uint8* blob = pixbuf_to_blob(sample->overview, out len);
					ok = Backend.update_blob(sample->id, "pixbuf", blob, len);
				}
				break;
			case Column.COLOUR:
				uint colour_index = *((uint*)val);
				if((ok = Backend.update_int(sample->id, "colour", colour_index))) {
					sample->colour_index = (int)colour_index;
				}
				break;
			case Column.PEAKLEVEL:
				if ((ok = Backend.update_float(sample->id, "peaklevel", sample->peaklevel))){
				}
				break;
			case Column.X_NOTES:
				if((ok = Backend.update_string(sample->id, "notes", (char*)val))) {
					sample->notes = (string)val;
				}
				break;
			case Column.X_EBUR:
				ok = Backend.update_string(sample->id, "ebur", sample->ebur);
				break;
			case -1:
				char* metadata = sample->get_metadata_str();

				ok = true;
				ok &= Backend.update_int    (sample->id, "channels", sample->channels);
				ok &= Backend.update_int    (sample->id, "sample_rate", sample->sample_rate);
				ok &= Backend.update_int    (sample->id, "length", (uint)sample->length);
				ok &= Backend.update_int    (sample->id, "frames", (uint)sample->frames);
				ok &= Backend.update_int    (sample->id, "bit_rate", sample->bit_rate);
				ok &= Backend.update_int    (sample->id, "bit_depth", sample->bit_depth);
				ok &= Backend.update_string (sample->id, "meta_data", metadata);

				if((bool)metadata) g_free(metadata);
				break;
			default:
				pwarn("model.update_sample", "unhandled property");
				break;
		}

		if(ok){
			queue_sample_changed(sample, prop, val);
		}else{
			pwarn("model.update_sample", "database update failed for %s", print_col_name(prop));
		}
		return ok;
	}

	public static char* print_col_name (uint prop_type)
	{
		switch (prop_type){
			case Column.ICON: return "ICON";
			case Column.IDX: return "IDX";
			case Column.NAME: return "NAME";
			case Column.FNAME: return "FILENAME";
			case Column.KEYWORDS: return "KEYWORDS";
			case Column.OVERVIEW: return "OVERVIEW";
			case Column.LENGTH: return "LENGTH";
			case Column.SAMPLERATE: return "SAMPLERATE";
			case Column.CHANNELS: return "CHANNELS";
			case Column.MIMETYPE: return "MIMETYPE";
			case Column.PEAKLEVEL: return "PEAKLEVEL";
			case Column.COLOUR: return "COLOUR";
			case Column.SAMPLEPTR: return "SAMPLEPTR";
			//case Column.NUM_COLS: return "NUM_COLS";
			case Column.X_EBUR: return "X_EBUR";
			case Column.X_NOTES: return "X_NOTES";
			case Column.ALL: return "ALL";
			default:
				string a = "UNKNOWN PROPERTY (%u)".printf(prop_type);
				Posix.memcpy(unk, a, 31);
				break;
		}
		return unk;
	}
}
}

