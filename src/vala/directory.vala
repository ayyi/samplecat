using Gtk;

class Directory : GLib.Object, Gtk.TreeModel {
	private bool needs_update;

	public Type get_column_type(int index)
	{
		return (Type)null;
	}

	public TreeModelFlags get_flags()
	{
		return TreeModelFlags.LIST_ONLY;
	}

	public bool get_iter (out Gtk.TreeIter iter, Gtk.TreePath path)
	{
		return true;
	}

	public int get_n_columns()
	{
		return 0;
	}

	public Gtk.TreePath? get_path (Gtk.TreeIter iter)
	{
		return null;
	}

	public void get_value(Gtk.TreeIter iter, int column, out GLib.Value value)
	{
	}

	public bool iter_children (out Gtk.TreeIter iter, Gtk.TreeIter? parent)
	{
		return true;
	}

	public bool iter_has_child (Gtk.TreeIter iter)
	{
		return true;
	}

	public int iter_n_children (Gtk.TreeIter? iter)
	{
		return 0;
	}

	public bool iter_next (ref Gtk.TreeIter iter)
	{
		return true;
	}

	public bool iter_nth_child (out Gtk.TreeIter iter, Gtk.TreeIter? parent, int n)
	{
		return true;
	}

	public bool iter_parent (out Gtk.TreeIter iter, Gtk.TreeIter child)
	{
		return true;
	}
}
