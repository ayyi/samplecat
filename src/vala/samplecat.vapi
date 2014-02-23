namespace Samplecat {

	[Compact]
	[CCode (cname = "Sample", cprefix = "sample_", cheader_filename = "sample.h", ref_function = "sample_ref", unref_function = "sample_unref")]
	public class Sample {
		public int          id;
		public int          ref_count;
		public Gtk.TreeRowReference* row_ref;

		public char*        keywords;
		public char*        ebur;
		public char*        notes;
		public char*        mimetype;

		public void         unref ();
		public void         ref   ();
	}

	[CCode (cname = "Application")]
	public struct Application
	{
		Model* model;
	}

	[CCode(cname = "SamplecatBackend", has_type_id = false)]
	[SimpleType]
	public struct Backend
	{
		[CCode(cname = "backend.remove", cheader_filename = "db/db.h")]
		public static bool remove(int id);
	}
	[CCode(cname = "backend", cheader_filename = "application.h")]
	Backend backend;

	[CCode(cname = "app", cheader_filename = "application.h")]
	Application* app;

	[CCode(cname = "listmodel__add_result", cheader_filename = "listmodel.h")]
	public void listmodel__add_result(Sample* sample);

	[CCode(cname = "request_ebur128", cheader_filename = "overview.h")]
	void        request_ebur128        (Sample* sample);

	[CCode (cname = "ObjectCallback", has_target = false, has_type_id = false)]
	public delegate void ObjectCallback (GLib.Object ?object, void* user_data);

	[CCode(cname = "Idle", cheader_filename = "support.h")]
	public class SignalIdle {
	   [CCode(cname = "idle_new", cheader_filename = "support.h")]
	   public SignalIdle           (owned ObjectCallback fn, void* user_data);
	   //public void idle_free             (SignalIdle idle);

	   public uint           id;
	   public GLib.Object    object;
	   public ObjectCallback fn;
	   public void*          user_data;
	   public ObjectCallback run;
	}
}
