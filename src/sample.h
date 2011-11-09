#ifndef __HAVE_SAMPLE_H_
#define __HAVE_SAMPLE_H_

#include <gtk/gtk.h>
struct _sample
{
	int          id;            //database index.
	int          ref_count;
	GtkTreeRowReference* row_ref;
	char         filename[256]; //full path.
	unsigned int sample_rate;
	unsigned int length;        //milliseconds
	unsigned int frames;        //total number of frames (eg a frame for 16bit stereo is 4 bytes).
	unsigned int channels;
	int          bitdepth;
	short        minmax[OVERVIEW_WIDTH]; //peak data before it is copied onto the pixbuf.
	GdkPixbuf*   pixbuf;
	double       peak_level;
	GdkColor     bg_colour;
};

Sample*     sample_new               ();
Sample*     sample_new_from_model    (GtkTreePath*);
Sample*     sample_new_from_fileview (GtkTreeModel*, GtkTreeIter*);
Sample*     sample_new_from_result   (SamplecatResult*);
void        sample_ref               (Sample*);
void        sample_unref             (Sample*);
void        sample_free              (Sample*);

gboolean    sample_get_file_info     (Sample*);
gboolean    result_get_file_info     (Result*);

Result*     result_new_from_model            (GtkTreePath*);
void        result_free                      (Result*);

#endif
