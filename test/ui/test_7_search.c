#include "waveform/view_plus.h"

void
test_7_search ()
{
	START_TEST;

	assert(view_is_visible("Search"), "expected search panel visible");

	void _do_search (gpointer _)
	{
		GtkTreeView* library = (GtkTreeView*)find_widget_by_type((GtkWidget*)find_panel("Library"), GTK_TYPE_TREE_VIEW);
		#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		int n_rows = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(library), NULL);
		#pragma GCC diagnostic warning "-Wdeprecated-declarations"
		assert(n_rows > 0, "no library items");

		search("piano");

		bool has_one_item ()
		{
			GtkTreeView* library = (GtkTreeView*)find_widget_by_type((GtkWidget*)find_panel ("Library"), GTK_TYPE_TREE_VIEW);
			#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
			return gtk_tree_model_iter_n_children(gtk_tree_view_get_model(library), NULL) == 1;
			#pragma GCC diagnostic warning "-Wdeprecated-declarations"
		}

		void then (gpointer _)
		{
			Sample* sample = samplecat_list_store_get_sample_by_row_index(0);
			assert(samplecat.model->selection == sample, "unexpected selection: selection=%s sample=%s", samplecat.model->selection, samplecat.model->selection ? samplecat.model->selection->name : NULL, sample->name);

			assert(!strcmp(sample->name, "piano.wav"), "name=%s", sample->name);
			assert(sample->sample_rate == 44100, "rate=%i", sample->sample_rate);
			assert(sample->length == 1000, "length=%"PRIi64, sample->length);
			assert(sample->frames == 44100, "frames=%"PRIi64, sample->frames);
			assert(sample->bit_depth == 16, "bit_depth=%i", sample->bit_depth);

			GtkTreeIter iter;
			gchar *keywords, *name, *fname;
			#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
			assert(gtk_tree_model_get_iter_first((GtkTreeModel*)samplecat.store, &iter), "iter");
			gtk_tree_model_get(GTK_TREE_MODEL(samplecat.store), &iter, COL_KEYWORDS, &keywords, COL_NAME, &name, COL_FNAME, &fname, -1);
			#pragma GCC diagnostic warning "-Wdeprecated-declarations"
			assert(!strcmp(keywords, ""), "tags=%s", keywords);
			assert(!strcmp(name, "piano.wav"), "name=%s", name);
			assert(!strcmp(g_strrstr(fname, "lib/waveform/test/data"), "lib/waveform/test/data"), "fname=%s", fname);

			FINISH_TEST;
		}

		wait_for(has_one_item, then, NULL);
	}

	g_timeout_add_once(250, _do_search, NULL);
}
