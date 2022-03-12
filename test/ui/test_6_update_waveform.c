#include "waveform/view_plus.h"

void
test_6_update_waveform ()
{
	START_TEST;

#ifdef USE_GDL
	assert(view_is_visible("Waveform"), "expected waveform panel visible");
	assert(view_is_visible("Library"), "expected library panel visible");

	observable_string_set(samplecat.model->filters2.search, g_strdup(""));

	GtkTreeView* library = (GtkTreeView*)gtk_bin_get_child ((GtkBin*)find_dock_item("Library")->child);
	GtkTreeSelection* selection = gtk_tree_view_get_selection(library);
	GtkTreeModel* model = gtk_tree_view_get_model(library);

	GtkTreeIter iter;
	if (gtk_tree_selection_count_selected_rows(selection) <= 1) {
		assert (gtk_tree_model_get_iter_first (model, &iter), "failed to get iter - libaryview has no rows?");
		gtk_tree_selection_select_iter (selection, &iter);
	}

	static char* orig_sample;
	orig_sample = g_strdup(samplecat.model->selection->name);

	bool selection_changed ()
	{
		return strcmp(orig_sample, samplecat.model->selection->name);
	}

	bool waveform_match ()
	{
		WaveformViewPlus* wave_view = (WaveformViewPlus*)find_dock_item("Waveform")->child;
		Sample* sample = samplecat.model->selection;
		Waveform* waveform = wave_view->waveform;

		return g_strrstr(waveform->filename, sample->name);
	}

	waveform_match();

	assert (gtk_tree_model_iter_next (model, &iter), "failed to get 2nd row");
	GtkTreePath* path = gtk_tree_model_get_path (model, &iter);
	gtk_tree_view_set_cursor (library, path, NULL, false);
	gtk_tree_path_free (path);

	void on_change (gpointer _)
	{
		void on_match (gpointer _)
		{
			g_free(orig_sample);
			FINISH_TEST;
		}

		wait_for(waveform_match, on_match, "Waveform match");
	}

	wait_for(selection_changed, on_change, "Selection change");
#else
	FINISH_TEST;
#endif
}
