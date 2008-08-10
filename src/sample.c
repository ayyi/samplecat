#include "config.h"
#define __USE_GNU
#include <string.h>
#include <gtk/gtk.h>
#include <jack/jack.h>
#include <sndfile.h>
#include "typedefs.h"
#include "mysql/mysql.h"
#include "dh-link.h"
#include "support.h"
#include "gqview_view_dir_tree.h"
#include "main.h"
#include "audio.h"
#include "overview.h"
#include "listview.h"
#include "sample.h"

#include "file_manager.h"

extern struct _app app;
extern Filer filer;


sample*
sample_new()
{
	sample* sample = g_new0(struct _sample, 1);
	return sample;
}


sample*
sample_new_from_model(GtkTreePath *path)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));
	gtk_tree_model_get_iter(model, &iter, path);
	gchar *fpath, *fname, *mimetype;
	int id;
	gtk_tree_model_get(model, &iter, COL_FNAME, &fpath, COL_NAME, &fname, COL_IDX, &id, COL_MIMETYPE, &mimetype, -1);

	sample* sample = g_new0(struct _sample, 1);
	sample->id = id;
	snprintf(sample->filename, 256, "%s/%s", fpath, fname);
	if(!strcmp(mimetype, "audio/x-flac")) sample->filetype = TYPE_FLAC; else sample->filetype = TYPE_SNDFILE; 
	sample->pixbuf = NULL;
	return sample;
}


//FIXME this is an internal detail of file_view. Use an accessor instead.
#define FILE_VIEW_COL_LEAF 0

sample*
sample_new_from_fileview(GtkTreeModel* model, GtkTreeIter* iter)
{
	gchar* fname;
	gtk_tree_model_get(model, iter, FILE_VIEW_COL_LEAF, &fname, -1);

	sample* sample = g_new0(struct _sample, 1);
	snprintf(sample->filename, 255, "%s/%s", filer.real_path, fname);
	return sample;
}


void
sample_free(sample* sample)
{
	gtk_tree_row_reference_free(sample->row_ref);
	g_free(sample);
}


void
sample_set_type_from_mime_string(sample* sample, char* mime_string)
{
	if(!strcmp(mime_string, "audio/x-flac")) sample->filetype = TYPE_FLAC;
	dbg(2, "mimetype: %s type=%i", mime_string, sample->filetype);
}


