using GLib;
using Gtk;
using FM;

public class Ayyi.Libfilemanager : GLib.Object
{
	//public int state = 0;

	public FM.Filer* file_window;

	public signal void dir_changed(string s);

	construct
	{
	}

	public Libfilemanager(FM.Filer* _file_window)
	{
		file_window = _file_window;
	}

	public void
	set_icon_theme (string theme)
	{
		print("%s(): theme=%s\n", Log.METHOD, theme);

		//g_strlcpy(theme_name, theme, 63);
		Memory.copy(theme_name, theme, theme.length + 1);

		_set_icon_theme();
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
		//dir_changed("hello");
		//string a = (*file_window).real_path;
		//string a = "%s".printf((*file_window).real_path);
		dir_changed((*file_window).real_path);
	}
}

