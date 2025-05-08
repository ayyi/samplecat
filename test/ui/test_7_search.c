#include "waveform/view_plus.h"

void
test_7_search ()
{
	START_TEST;

	assert(view_is_visible("Search"), "expected search panel visible");

	gboolean _do_search (gpointer _)
	{
		int n_rows = gtk_tree_model_iter_n_children(gtk_tree_view_get_model((GtkTreeView*)app->libraryview->widget), NULL);
		assert_and_stop(n_rows > 0, "no library items");

		search("piano");

		bool has_one_item (void* _)
		{
			return gtk_tree_model_iter_n_children(gtk_tree_view_get_model((GtkTreeView*)app->libraryview->widget), NULL) == 1;
		}

		void then (gpointer _)
		{
			Sample* sample = samplecat_list_store_get_sample_by_row_index(0);
			assert(samplecat.model->selection == sample, "selection=%p", samplecat.model->selection);

			assert(!strcmp(sample->name, "piano.wav"), "name=%s", sample->name);
			assert(sample->sample_rate == 44100, "rate=%i", sample->sample_rate);
			assert(sample->length == 1000, "length=%"PRIi64, sample->length);
			assert(sample->frames == 44100, "frames=%"PRIi64, sample->frames);
			assert(sample->bit_depth == 16, "bit_depth=%i", sample->bit_depth);

			GtkTreeIter iter;
			gchar *keywords, *name, *fname;
			assert(gtk_tree_model_get_iter_first((GtkTreeModel*)samplecat.store, &iter), "iter");
			gtk_tree_model_get(GTK_TREE_MODEL(samplecat.store), &iter, COL_KEYWORDS, &keywords, COL_NAME, &name, COL_FNAME, &fname, -1);
			assert(!strcmp(keywords, ""), "tags=%s", keywords);
			assert(!strcmp(name, "piano.wav"), "name=%s", name);
			assert(!strcmp(g_strrstr(fname, "lib/waveform/test/data"), "lib/waveform/test/data"), "fname=%s", fname);

			FINISH_TEST;
		}

		wait_for(has_one_item, then, NULL);

		return G_SOURCE_REMOVE;
	}

	g_timeout_add(1000, _do_search, NULL);
}
