#ifndef __SAMPLECAT_TYPES_H_
#define __SAMPLECAT_TYPES_H_
//TODO this is annoyingly similar to struct Sample - consider merging...
#if 0 // merge in progress
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
	char*                ebur;
	char*                notes;
	int                  colour;
	gboolean             online;
	char*                updated; //TODO char* ? int? date?
};
#endif

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
	GtkWidget*     ebur;
	GtkWidget*     image;
	GtkWidget*     slider;
	GtkWidget*     text;
	GtkTextBuffer* notes;
	GtkWidget*     edit;
	int            min_height;
	int            user_height;
} Inspector;
#endif
