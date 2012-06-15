using Config;
using GLib;
using Gdk;
using Gtk;
using Samplecat;

namespace Samplecat
{
public class ListStore : Gtk.ListStore
{
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
}
}

