
//TODO this is annoyingly similar to struct Sample - consider merging...
struct _samplecat_result
{
	int                  idx;
	GtkTreeRowReference* row_ref;
	char*                sample_name;
	char*                full_path;
	char*                dir;
	char*                keywords;
	GdkPixbuf*           overview;
	char*                mimetype;
	unsigned             length;
	unsigned             sample_rate;
	int                  channels;
	float                peak_level;
	char*                misc;
	char*                notes;
	int                  colour;
	gboolean             online;
	char*                updated; //TODO char* ? int? date?
};

struct _menu_def
{
	char*                label;
	GCallback            callback;
	char*                stock_id;
	gboolean             sensitive;
};

typedef struct _inspector
{
	unsigned       row_id;
	GtkTreeRowReference* row_ref;
	GtkWidget*     widget;     //scrollwin
	GtkWidget*     vbox;
	GtkWidget*     name;
	GtkWidget*     filename;
	GtkWidget*     tags;
	GtkWidget*     tags_ev;    //event box for mouse clicks
	GtkWidget*     length;
	GtkWidget*     samplerate;
	GtkWidget*     channels;
	GtkWidget*     mimetype;
	GtkWidget*     level;
	GtkWidget*     misc;
	GtkWidget*     image;
	GtkWidget*     text;
	GtkTextBuffer* notes;
	GtkWidget*     edit;
	int            min_height;
	int            user_height;
} Inspector;


