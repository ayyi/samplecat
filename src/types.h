
//TODO this is annoying similar to struct Sample - consider merging...
struct _samplecat_result
{
	int                  idx;
	GtkTreeRowReference* row_ref;
	char*                sample_name;
	char*                dir;
	char*                keywords;
	GdkPixbuf* overview;
	char*      mimetype;
	unsigned   length;
	unsigned   sample_rate;
	int        channels;
	char*      notes;
	int        colour;
	gboolean   online;
};

