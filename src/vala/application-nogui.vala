using Config;
using GLib;
using Samplecat;

public class NoGuiApplication : GLib.Application
{
	public char* cache_dir;

	public signal void on_quit();

	construct
	{
	}

	public NoGuiApplication()
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

