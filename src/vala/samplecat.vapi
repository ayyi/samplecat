namespace Samplecat {

	[Compact]
	[CCode (cname = "Sample", cprefix = "sample_", cheader_filename = "sample.h", ref_function = "sample_ref", unref_function = "sample_unref")]
	public class Sample {
		[CCode (has_construct_function = false)]
		public Sample.from_filename(char* full_path, bool path_alloced = false);

		public bool         get_file_info();
		public char*        get_metadata_str();

		[CCode (cname = "request_overview", cheader_filename = "worker.h")]
		public void         request_overview  ();
		[CCode (cname = "request_peaklevel", cheader_filename = "worker.h")]
		public void         request_peaklevel ();
		[CCode (cname = "request_ebur128", cheader_filename = "worker.h")]
		public void         request_ebur128   ();

		public int          id;
		public int          ref_count;
		public Gtk.TreeRowReference* row_ref;
		public char*        full_path;

		public uint         sample_rate;
		public int64        length;        //milliseconds
		public int64        frames;        //total number of frames (eg a frame for 16bit stereo is 4 bytes).
		public uint          channels;
		public int          bit_depth;
		public int          bit_rate;
		//public GPtrArray*   meta_data;
		public float        peaklevel;
		public int          colour_index;

		//public char*        keywords;
		public string       keywords;
		public char*        ebur;
		public string       notes;
		public char*        mimetype;

		public Gdk.Pixbuf*  overview;

		public bool         online;
		public time_t       mtime;

		public void         unref ();
		public void         ref   ();
	}

	[CCode (cname = "Application")]
	public struct Application
	{
		Model* model;
	}

	[CCode(cname = "SamplecatBackend", has_type_id = false, cheader_filename = "db/db.h")]
	[SimpleType]
	public struct Backend
	{
		[CCode(cname = "backend.remove", cheader_filename = "db/db.h")]
		public static bool remove(int id);
		[CCode(cname = "backend.update_string", cheader_filename = "db/db.h")]
		public static bool update_string(int id, char* prop, char* val);
		[CCode(cname = "backend.update_int")]
		public static bool update_int(int id, char* prop, uint val);
		[CCode(cname = "backend.update_float")]
		public static bool update_float (int id, char* prop, float val);
		[CCode(cname = "backend.update_blob")]
		public static bool update_blob (int id, char* prop, uint8* data, uint len);
	}
	[CCode(cname = "backend", cheader_filename = "application.h")]
	Backend backend;

	[CCode(cname = "app", cheader_filename = "application.h")]
	Application* app;

	[CCode(cname = "listmodel__add_result", cheader_filename = "listmodel.h")]
	public void listmodel__add_result(Sample* sample);

	[CCode(cname = "request_ebur128", cheader_filename = "worker.h")]
	void        request_ebur128        (Sample* sample);

	[CCode(cprefix = "COL_", cheader_filename = "listmodel.h")]
	enum Column {
		ICON = 0,
		IDX,
		NAME,
		FNAME,
		KEYWORDS,
		OVERVIEW,
		LENGTH,
		SAMPLERATE,
		CHANNELS,
		MIMETYPE,
		PEAKLEVEL,
		COLOUR,
		SAMPLEPTR,
		NUM_COLS, ///< end of columns in the store
		// these are NOT in the store but in the sample-struct (COL_SAMPLEPTR)
		X_EBUR,
		X_NOTES
	}

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

	[CCode(cname = "file_mtime", cheader_filename = "samplecat/support.h")]
	public time_t file_mtime (char* file);

	[CCode(cname = "samplerate_format", cheader_filename = "support.h")]
	public void samplerate_format (char* f, int samplerate);
	[CCode(cname = "smpte_format", cheader_filename = "support.h")]
	public void smpte_format (char* f, int64 t);
	[CCode(cname = "pixbuf_to_blob", cheader_filename = "support.h")]
	public uint8* pixbuf_to_blob (Gdk.Pixbuf* in, out uint len);
}
