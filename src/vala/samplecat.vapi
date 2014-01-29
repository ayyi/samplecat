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

	public struct _filters {
			char*     phrase;
			char*     dir;
			char*     category;
	}

	[CCode (cname = "SamplecatModel")]
	public struct _samplecat_model
	{
		_filters filters;
	}

	[CCode (cname = "Application")]
	public struct Application
	{
		public _samplecat_model* model;
	}

	[CCode(cname = "app", cheader_filename = "application.h")]
	Application* app;

	[CCode(cname = "listmodel__add_result", cheader_filename = "listmodel.h")]
	public void listmodel__add_result(Sample* sample);

	[CCode(cname = "request_ebur128", cheader_filename = "overview.h")]
	void        request_ebur128        (Sample* sample);
}
