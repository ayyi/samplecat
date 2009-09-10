using GLib;
using Gtk;

public class Ayyi.Filemanager : GLib.Object
{
	//public int state = 0;

	public signal void theme_changed(string s);

	construct
	{
	}

	public void
	set_icon_theme (string theme)
	{
		stdout.printf("set_icon_theme(): theme=%s", theme);
	}

	public Gtk.Widget
	new_window(string path)
	{
		Gtk.Widget w = null;

		//GtkWidget* file_view = view_details_new(&filer);
		//!!there are no binding for view_details, so this is not going to be easy!!

		return w;
	}

	public void
	emit_theme_changed()
	{
		theme_changed("hello");
	}

	/*
	public void
	set_state(int state)
	{
		state = state;
	}
	*/
}

