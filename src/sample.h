#ifndef __HAVE_SAMPLE_H_
#define __HAVE_SAMPLE_H_

#include <stdlib.h>
#include <stdint.h>

#ifndef PATH_MAX
#define PATH_MAX (1024)
#endif

#include <gtk/gtk.h> // TODO get rid of GTK tpes in the struct
struct _sample
{
	int          id;            //database index.
	int          idx; // result id

	int          ref_count;
	GtkTreeRowReference* row_ref;

	char*                sample_name;
	char*                full_path; // filename
	char         filename[PATH_MAX]; //full path.

	char*                dir;
	char*                keywords;

	GdkPixbuf*           overview; // pixbuf
	GdkPixbuf*   pixbuf;

	unsigned int sample_rate;
	int64_t length;        //milliseconds
	int64_t frames;        //total number of frames (eg a frame for 16bit stereo is 4 bytes).

	unsigned int channels;
	int          bitdepth;
	short        minmax[OVERVIEW_WIDTH]; //peak data before it is copied onto the pixbuf.
	float                peak_level;
	GdkColor     bg_colour;
	int                  colour;

	gboolean             online;

	char*                misc;
	char*                notes;

	char*                mimetype;
	char*                updated; //TODO char* ? int? date?
};

Sample*     sample_new               ();
Sample*     sample_new_from_model    (GtkTreePath*);
Sample*     sample_new_from_fileview (GtkTreeModel*, GtkTreeIter*);
Sample*     sample_new_from_result   (Sample*);
void        sample_ref               (Sample*);
void        sample_unref             (Sample*);
void        sample_free              (Sample*);

gboolean    sample_get_file_info     (Sample*);
gboolean    result_get_file_info     (Sample*);

Sample*     result_new_from_model            (GtkTreePath*);
void        result_free                      (Sample*);

#endif
