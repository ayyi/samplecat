#include "config.h"
#include <stdio.h>
#include <string.h>
#include <debug/debug.h>

#include "main.h"
#include "support.h"
#include "sample.h"
#include "console_view.h"

static void console__show_result_footer (int);


void console__init()
{
#if 0
	// for this to work, we would have to not add blank rows before populating them which is not possible.

	void store_row_inserted(GtkListStore* store, GtkTreePath* path, GtkTreeIter* iter, gpointer user_data)
	{
		PF0;
		Sample* sample = sample_get_by_tree_iter(iter);
		if(sample){
			console__show_result(sample);
			sample_unref(sample);
		}
	}
	g_signal_connect(G_OBJECT(app->store), "row-inserted", G_CALLBACK(store_row_inserted), NULL);
#endif

	void store_content_changed(GtkListStore* store, gpointer data)
	{
		PF;
		GtkTreeIter iter;
		if(!gtk_tree_model_get_iter_first((GtkTreeModel*)store, &iter)){ gerr ("cannot get iter."); return; }
		int row_count = 0;
		do {
			if(++row_count < 100){
				Sample* sample = sample_get_by_tree_iter(&iter);
				if(sample){
					console__show_result(sample);
					sample_unref(sample);
				}
			}
		} while (gtk_tree_model_iter_next((GtkTreeModel*)store, &iter));

		console__show_result_footer(row_count);
	}
	g_signal_connect(G_OBJECT(app->store), "content-changed", G_CALLBACK(store_content_changed), NULL);
}


void
console__show_result_header()
{
	printf("filters: text='%s' dir=%s\n", app->model->filters.search->value, strlen(app->model->filters.dir->value) ? app->model->filters.dir->value : "<all directories>");

	printf("  name                 directory                            length ch rate mimetype\n");
}


void
console__show_result(Sample* result)
{
	#define DIR_MAX (35)
	char dir[DIR_MAX];
	strncpy(dir, dir_format(result->sample_dir), DIR_MAX-1);
	dir[DIR_MAX-1] = '\0';

	#define SNAME_MAX (20)
	char name[SNAME_MAX];
	strncpy(name, result->sample_name, SNAME_MAX-1);
	name[SNAME_MAX-1] = '\0';

	printf("  %-20s %-35s %7"PRIi64" %d %5d %s\n", name, dir, result->length, result->channels, result->sample_rate, result->mimetype);
}


static void
console__show_result_footer(int row_count)
{
	printf("total %i samples found.\n", row_count);
}

#ifdef NEVER
#include <sys/ioctl.h>
static int
get_terminal_width()
{
	struct winsize ws;

	if(ioctl(1, TIOCGWINSZ, (void *)&ws) != 0) printf("ioctl failed\n");

	dbg(1, "terminal width = %d\n", ws.ws_col);

	return ws.ws_col;
}
#endif
