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
	public signal void content_changed(); // a new search has been completed.

	public ListStore() {
		Type types[13] = {
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
			typeof(void*)       // COL_SAMPLEPTR
		};
		set_column_types(types);
	}

	public void add(/*unowned */Sample* sample)
	{
		listmodel__add_result(sample);

		if((!(bool)sample->ebur || !(bool)strlen((string)sample->ebur)) && (bool)sample->row_ref){
			dbg(1, "regenerate ebur128");
			request_ebur128(sample);
		}

		sample.ref();
	}

	public void do_search()
	{
		content_changed();
	}
}
}

