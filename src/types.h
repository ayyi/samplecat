
struct _samplecat_result
{
	int        idx;
	char*      sample_name;
	char*      dir;
	char*      keywords;
	GdkPixbuf* overview;
	char*      mimetype;
	unsigned   length;
	unsigned   sample_rate;
	int        channels;
	char*      notes;
	int        colour;
	gboolean   online;
};

