/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://samplecat.orford.org          |
* | copyright (C) 2007-2015 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
using Config;
using Posix;
using GLib;
using Gdk;
using Gtk;
using Samplecat;

namespace Samplecat
{
public class ListStore : Gtk.ListStore
{
	public int row_count = 0;
	public TreeRowReference playing;

	public signal void content_changed(); // a new search has been completed.

	public ListStore() {
		Type types[14] = {
			typeof(Gdk.Pixbuf), // COL_ICON
//#ifdef USE_AYYI
//			typeof(Gdk.Pixbuf), // COL_AYYI_ICON
//#endif
			typeof(int),        // COL_IDX
			typeof(string),     // COL_NAME
			typeof(string),     // COL_FNAME
			typeof(string),     // COL_KEYWORDS
			typeof(Gdk.Pixbuf), // COL_OVERVIEW
			typeof(string),     // COL_LENGTH,
			typeof(string),     // COL_SAMPLERATE
			typeof(int),        // COL_CHANNELS
			typeof(string),     // COL_MIMETYPE
			typeof(float),      // COL_PEAKLEVEL
			typeof(int),        // COL_COLOUR
			typeof(void*),      // COL_SAMPLEPTR
			typeof(int64)       // COL_LEN
		};
		set_column_types(types);
	}

	public void clear_()
	{
		Gtk.TreeIter iter;
		while(get_iter_first(out iter)){
			Gdk.Pixbuf* pixbuf = null;
			Sample* sample = null;
			get(iter, Column.OVERVIEW, &pixbuf, -1);
			get(iter, Column.SAMPLEPTR, &sample, -1);
			remove(iter);
			if((bool)pixbuf) pixbuf->unref();
			if((bool)sample) sample->unref();
		}

		row_count = 0;
	}

	public void add(/*unowned */Sample* sample)
	{
		return_if_fail(sample);

#if 1
		/* these has actualy been checked _before_ here
		 * but backend may 'inject' mime types. ?!
		 */
		if(!sample->mimetype){
			dbg(0,"no mimetype given -- this should NOT happen: fix backend");
			return;
		}
		if(mimestring_is_unsupported(sample->mimetype)){
			dbg(0, "unsupported MIME type: %s", sample->mimetype);
			return;
		}
#endif

		if(!(bool)sample->sample_rate){
			// needed w/ tracker backend.
			sample->get_file_info();
		}

		char samplerate_s[32]; samplerate_format(samplerate_s, (int)sample->sample_rate);
		char length_s[64]; format_smpte(length_s, sample->length);

#if 0 //#ifdef USE_AYYI
		Gdk.Pixbuf* ayyi_icon = null;

		//is the file loaded in the current Ayyi song?
		if(ayyi.got_shm){
			gchar* fullpath = g_build_filename(sample->sample_dir, sample->name, NULL);
			if(ayyi_song__have_file(fullpath)){
				dbg(1, "sample is used in current project TODO set icon");
			} else dbg(2, "sample not used in current project");
			g_free(fullpath);
		}
#endif

		//icon (only shown if the sound file is currently available):
		Gdk.Pixbuf* iconbuf = sample->online ? get_iconbuf_from_mimetype(sample->mimetype) : null;

		Gtk.TreeIter iter;
		append(out iter);
		set(iter,
			Column.ICON,       iconbuf,
			Column.NAME,       sample->name,
			Column.FNAME,      sample->sample_dir,
			Column.IDX,        sample->id,
			Column.MIMETYPE,   sample->mimetype,
			Column.KEYWORDS,   (bool)sample->keywords ? sample->keywords : "",
			Column.PEAKLEVEL,  sample->peaklevel,
			Column.OVERVIEW,   sample->overview,
			Column.LENGTH,     length_s,
			Column.SAMPLERATE, samplerate_s,
			Column.CHANNELS,   sample->channels, 
			Column.COLOUR,     sample->colour_index,
//#ifdef USE_AYYI
//			COL_AYYI_ICON,  ayyi_icon,
//#endif
			Column.SAMPLEPTR,  sample,
			Column.LENGTH,     sample->length,
			-1);
		Gtk.TreePath* treepath;
		if((bool)(treepath = app->store->get_path(iter))){
			sample->row_ref = new TreeRowReference(app->store, treepath);
			//treepath->unref();
		}

		if((bool)sample->row_ref){
			sample->request_analysis();
		}

		sample->ref();
	}

	public void on_sample_changed(Sample* sample, int prop, void* val)
	{
		return_if_fail((bool)sample->row_ref);

		Gtk.TreePath path = sample->row_ref->get_path(); // vala will free when goes out of scope
		if(!(bool)path) return;
		Gtk.TreeIter iter;
		get_iter(out iter, path);

		switch(prop){
			case Column.ICON: // online/offline, mtime
				{
					Gdk.Pixbuf* iconbuf = null;
					if (sample->online) {
						MIME_type* mime_type = mime_type_lookup(sample->mimetype);
						type_to_icon(mime_type);
						//if (!mime_type->image) dbg(0, "no icon.");
						iconbuf = mime_type->image->sm_pixbuf;
					}
					set(iter, Column.ICON, iconbuf, -1);
				}
				break;
			case Column.KEYWORDS:
				set(iter, Column.KEYWORDS, (char*)val, -1);
				break;
			case Column.OVERVIEW:
				set(iter, Column.OVERVIEW, sample->overview, -1);
				break;
			case Column.COLOUR:
				set(iter, Column.COLOUR, *((uint*)val), -1);
				break;
			case Column.PEAKLEVEL:
				set(iter, Column.PEAKLEVEL, sample->peaklevel, -1);
				break;
			case Column.X_EBUR:
			case Column.X_NOTES:
				// nothing to do.
				break;
			case -1:
				{
					//char* metadata = sample_get_metadata_str(s);

					char samplerate_s[32]; samplerate_format(samplerate_s, (int)sample->sample_rate);
					char length_s[64]; format_smpte(length_s, sample->length);
					set(iter, 
							Column.CHANNELS, sample->channels,
							Column.SAMPLERATE, samplerate_s,
							Column.LENGTH, length_s,
							Column.PEAKLEVEL, sample->peaklevel,
							Column.OVERVIEW, sample->overview,
							-1);
					dbg(1, "file info updated.");
					//if(metadata) g_free(metadata);
				}
				break;
			default:
				dbg(0, "property not handled");
				break;
		}
	}

#if 0
	private bool update_by_rowref (TreeRowReference* row_ref, int prop, void* val)
	{
		TreePath path = row_ref->get_path();
		return_val_if_fail((bool)path, false);

		TreeIter iter;
		get_iter(out iter, path);
		update_by_tree_iter(&iter, prop, val);

		return true;
	}

	private void update_by_tree_iter(TreeIter* iter, int what, void* data)
	{
		switch (what) {
			case Column.ICON:
				set(*iter, Column.ICON, data, -1);
				break;
			case Column.KEYWORDS:
				set(*iter, Column.KEYWORDS, (char*)data, -1);
				break;
			case Column.COLOUR:
				set(*iter, what, *((uint*)data), -1);
				break;
			default:
				dbg(0, "update for this type is not yet implemented"); 
				break;
		}
	}
#endif

	public void do_search()
	{
		content_changed();
	}
}
}

