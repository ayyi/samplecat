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
	int                  id;  // database index.
	int                  ref_count;
	GtkTreeRowReference* row_ref;

	/* TODO: consolidate dir/name: use only 'full_path' 
	 * and 'derive' others on demand
	 * -> requires changing the database model
	 */
	char*        dir; // directory
	char*        sample_name; // basename
	char*        full_path; // filename

	unsigned int sample_rate;
	int64_t      length;        //milliseconds
	int64_t      frames;        //total number of frames (eg a frame for 16bit stereo is 4 bytes).
	unsigned int channels;
	int          bitdepth;
	float        peak_level;
	int          colour_index;

	gboolean     online;
	char*        updated; //TODO char* ? int? date?

	char*        keywords;
	char*        misc;
	char*        notes;
	char*        mimetype;

	GdkPixbuf*   overview; // pixbuf
};

/** create new sample structures
 * all _new_ functions imply sample_ref() 
 *
 * @return needs to be sample_unref();
 */
Sample*     sample_new               ();
Sample*     sample_new_from_model    (GtkTreePath*);
Sample*     sample_new_from_fileview (GtkTreeModel*, GtkTreeIter*);
Sample*     sample_new_from_filename (char *path, gboolean path_alloced);
Sample*     sample_dup               (Sample*);
Sample*     sample_get_by_row_ref    (GtkTreeRowReference* ref); //< does not increase reference count ; may return NULL

void        sample_ref               (Sample*);
void        sample_unref             (Sample*);
void        sample_free              (Sample*);

gboolean    sample_get_file_info     (Sample*);

#endif
