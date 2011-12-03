using GLib;
using Gtk;
using FM;

public class Ayyi.Libfilemanager : GLib.Object
{
	public FM.Filer* file_window;

	public signal void dir_changed(string s);

	construct
	{
	}

	public Libfilemanager(FM.Filer* _file_window)
	{
		file_window = _file_window;
	}

	public Gtk.Widget
	new_window(string path)
	{
		Gtk.Widget w = null;

		//Gtk.Widget file_view;
		//file_view = file_window.view_details_new(file_window);

		(*file_window).update_dir(true);

		//file_window.window.hide();

		return w;
	}

	public void
	emit_dir_changed()
	{
		dir_changed((*file_window).real_path);
	}
}

