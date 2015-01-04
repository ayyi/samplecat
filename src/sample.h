#ifndef __have_sample_h__
#define __have_sample_h__

#include <stdlib.h>
#include <stdint.h>
#include <gtk/gtk.h>
#include "typedefs.h"

struct _sample
{
	int                  id;  // database index.
	int                  ref_count;
	GtkTreeRowReference* row_ref;

	char*        sample_dir;  ///< directory UTF8
	char*        sample_name; ///< basename UTF8
	char*        full_path;   ///< filename as native to system

	unsigned int sample_rate;
	int64_t      length;        //milliseconds
	int64_t      frames;        //total number of frames (eg a frame for 16bit stereo is 4 bytes).
	unsigned int channels;
	int          bit_depth;
	int          bit_rate;
	GPtrArray*   meta_data;
	float        peaklevel;
	int          colour_index;

	gboolean     online;
	time_t       mtime;

	char*        keywords;
	char*        ebur;
	char*        notes;
	char*        mimetype;

	GdkPixbuf*   overview;
};


/** create new sample structures
 * all _new, _dup functions imply sample_ref() 
 *
 * @return needs to be sample_unref();
 */
Sample*     sample_new               ();
Sample*     sample_new_from_filename (char* path, gboolean path_alloced);
Sample*     sample_dup               (Sample*);

/** return a reference to the existing sample in the tree.
 * implies sample_ref() 
 * @return needs to be sample_unref();
 */
Sample*     sample_get_from_model    (GtkTreePath*);

/** sample_get_by_tree_iter returns a pointer to
 * the sample struct in the data model or NULL if not found.
 * @return needs to be sample_unref();
 */
Sample*     sample_get_by_tree_iter  (GtkTreeIter*);

/** sample_get_by_row_ref returns a pointer to
 * the sample struct in the data model or NULL if not found.
 * @return needs to be sample_unref();
 */
Sample*     sample_get_by_row_ref    (GtkTreeRowReference*); 

Sample*     sample_get_by_filename   (const char* abspath);

Sample*     sample_ref               (Sample*);
void        sample_unref             (Sample*);

bool        sample_get_file_info     (Sample*);

char*       sample_get_metadata_str  (Sample*);
void        sample_set_metadata      (Sample*, const char*);

#endif
