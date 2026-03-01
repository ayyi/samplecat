#include "waveform/view_plus.h"

void
test_6_update_waveform ()
{
	START_TEST;

	assert(view_is_visible("Waveform"), "expected waveform panel visible");
	assert(view_is_visible("Library"), "expected library panel visible");

	observable_string_set(samplecat.model->filters2.search, g_strdup(""));

	int n_rows = samplecat.store->row_count;
	assert(n_rows > 0, "failed to get rows - library has no rows?");
	g_autoptr(Sample) first = samplecat_list_store_get_sample_by_index(0);
	assert(first, "failed to get first row");
	if (!samplecat.model->selection) {
		samplecat_model_set_selection(samplecat.model, first);
	}

	void idle ()
	{
		static char* orig_sample;

		int n_rows = samplecat.store->row_count;
		assert(n_rows > 1, "failed to get iter - libraryview has less than 2 rows?");

		assert(samplecat.model->selection, "model has no selection");
		orig_sample = g_strdup(samplecat.model->selection->name);

		bool selection_changed (void* _)
		{
			Sample* sel = samplecat.model->selection;
			return sel && strcmp(orig_sample, sel->name);
		}

		bool waveform_match ()
		{
			WaveformViewPlus* wave_view = WAVEFORM_VIEW_PLUS(find_panel("Waveform")->child);
			Sample* sample = samplecat.model->selection;
			g_return_val_if_fail(sample && sample->online, false);
			Waveform* waveform = wave_view->waveform;

			g_return_val_if_fail(waveform, false);
			assert_and_stop(waveform->filename, "no waveform loaded");

			return g_strrstr(waveform->filename, sample->name);
		}

		assert(waveform_match(), "unable to match waveform");

		g_autoptr(Sample) next = samplecat_list_store_get_sample_by_index(1);
		assert(next, "failed to get 2nd row");
		samplecat_model_set_selection(samplecat.model, next);

		void on_change (gpointer _)
		{
			void on_match (gpointer _)
			{
				g_free(orig_sample);
				FINISH_TEST;
			}

			wait_for((ReadyTest)waveform_match, on_match, "Waveform match");
		}

		wait_for(selection_changed, on_change, "Selection change");
	}
	g_idle_add_once((GSourceOnceFunc)idle, NULL);
}
