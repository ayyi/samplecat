using Config;
using GLib;
using Gtk;
using Samplecat;

public class Application : GLib.Object
{
	public int state = 0;
	public char* cache_dir;

	public signal void on_quit();
	public signal void theme_changed();
	public signal void icon_theme(string s);

	construct
	{
	}

	public Application()
	{
		state = 1; //dummy
		cache_dir = Path.build_filename(Environment.get_home_dir(), ".config", PACKAGE, "cache", null);
	}

	public void
	emit_icon_theme_changed(string s)
	{
		icon_theme(s);
	}
}

