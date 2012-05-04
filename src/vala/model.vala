using Config;
using GLib;
using Gtk;
using Samplecat;

namespace Samplecat
{
public class Model : GLib.Object
{
	public int state = 0;
	public char* cache_dir;

	public signal void sample_changed(Sample* sample, int what, void* data);

	construct
	{
	}

	public Model()
	{
		state = 1; //dummy
		cache_dir = Path.build_filename(Environment.get_home_dir(), ".config", PACKAGE, "cache", null);
	}
}
}

