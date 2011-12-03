//this is manually created - do not delete!
[CCode(
	cheader_filename = "file_manager/filer.h",
	cheader_filename = "file_manager/rox_global.h",
	lower_case_cprefix = "", cprefix = "")]
namespace FM {
	[CCode(cname = "struct _Filer",
		cheader_filename = "file_manager/file_manager.h"
	)]
	public struct Filer {
		[CCode(cname = "window")]
		public Gtk.Widget window;
		public string real_path;
		//public void update_display(Directory *dir, DirAction action, GPtrArray* items, Filer* filer_window);
		public void destroy();
		public bool update_dir(bool warning);
	}
	[CCode(cheader_filename = "file_manager/file_view.h", lower_case_cprefix = "", cprefix = "")]
	[CCode(cname = "struct _ViewDetails")]
	public struct ViewDetails {
		public Gtk.TreeView treeview;
		[CCode(cname = "view_details_new")]
		public Gtk.Widget view_details_new(Filer f);
	}
	[CCode(cheader_filename = "file_manager/mimetype.h", lower_case_cprefix = "", cprefix = "")]
	char* theme_name;
	[CCode(cheader_filename = "src/icon_theme.h", lower_case_cprefix = "", cprefix = "")]
	void  set_icon_theme();

	public struct Mimetype {
	}
}
