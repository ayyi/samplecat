/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2015 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <debug/debug.h>

#include "model.h"
#include "application.h"
#include "support.h"
#include "sample.h"
#include "listview.h"
#include "console_view.h"

static void console__show_result_footer (int);


void console__init()
{
#if 0
	// for this to work, we would have to not add blank rows before populating them which is not possible.

	void store_row_inserted(GtkListStore* store, GtkTreePath* path, GtkTreeIter* iter, gpointer user_data)
	{
		PF0;
		Sample* sample = samplecat_list_store_get_sample_by_iter(iter);
		if(sample){
			console__show_result(sample);
			sample_unref(sample);
		}
	}
	g_signal_connect(G_OBJECT(samplecat.store), "row-inserted", G_CALLBACK(store_row_inserted), NULL);
#endif

	void store_content_changed(GtkListStore* store, gpointer data)
	{
		PF;
		GtkTreeIter iter;
		if(!gtk_tree_model_get_iter_first((GtkTreeModel*)store, &iter)){ gerr ("cannot get iter."); return; }
		int row_count = 0;
		do {
			if(++row_count < 100){
				Sample* sample = samplecat_list_store_get_sample_by_iter(&iter);
				if(sample){
					console__show_result(sample);
					sample_unref(sample);
				}
			}
		} while (gtk_tree_model_iter_next((GtkTreeModel*)store, &iter));

		console__show_result_footer(row_count);
	}

	if(!app->args.add)
		g_signal_connect(G_OBJECT(samplecat.store), "content-changed", G_CALLBACK(store_content_changed), NULL);
}


void
console__show_result_header()
{
	printf("filters: text='%s' dir=%s\n", samplecat.model->filters.search->value, strlen(samplecat.model->filters.dir->value) ? samplecat.model->filters.dir->value : "<all directories>");

	printf("  name                 directory                            length ch rate mimetype\n");
}


void
console__show_result(Sample* result)
{
	int w = get_terminal_width();

	#define DIR_MAX (35)
	char dir[DIR_MAX];
	strncpy(dir, dir_format(result->sample_dir), DIR_MAX-1);
	dir[DIR_MAX-1] = '\0';

	#define SNAME_MAX (20)
	int max = SNAME_MAX + (w > 100 ? 10 : 0);
	char name[max];
	g_strlcpy(name, result->name, max);

	char format[256] = {0,};
	snprintf(format, 255, "  %%-%is %%-35s %%7"PRIi64" %%d %%5d %%s\n", max);

	printf(format, name, dir, result->length, result->channels, result->sample_rate, result->mimetype);
}


static void
console__show_result_footer(int row_count)
{
	printf("total %i samples found.\n", row_count);
}

