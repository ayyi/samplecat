using Config;
using GLib;

public class SamplecatApplication : GLib.Application
{
	public char* cache_dir;

	construct
	{
	}

	public SamplecatApplication ()
	{
		cache_dir = Path.build_filename(Environment.get_home_dir(), ".config", PACKAGE, "cache", null);
	}

	public override void startup ()
	{
	}

	public override void activate ()
	{
	}
}

