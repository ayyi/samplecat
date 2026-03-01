#include "waveform/view_plus.h"

void
test_7_search ()
{
	START_TEST;

	assert(view_is_visible("Search"), "expected search panel visible");

	void _do_search (gpointer _)
	{
		int n_rows = samplecat.store->row_count;
		assert(n_rows > 0, "no library items");

		search("piano");

		bool has_one_item (void* _)
		{
			return samplecat.store->row_count == 1;
		}

		void then (gpointer _)
		{
			Sample* sample = samplecat_list_store_get_sample_by_index(0);
			assert(samplecat.model->selection == sample, "unexpected selection: selection=%s sample=%s", samplecat.model->selection, samplecat.model->selection ? samplecat.model->selection->name : NULL, sample->name);

			assert(!strcmp(sample->name, "piano.wav"), "name=%s", sample->name);
			assert(sample->sample_rate == 44100, "rate=%i", sample->sample_rate);
			assert(sample->length == 1000, "length=%"PRIi64, sample->length);
			assert(sample->frames == 44100, "frames=%"PRIi64, sample->frames);
			assert(sample->bit_depth == 16, "bit_depth=%i", sample->bit_depth);

			assert(!sample->keywords || !strcmp(sample->keywords, ""), "tags=%s", sample->keywords);
			assert(!strcmp(sample->name, "piano.wav"), "name=%s", sample->name);
			assert(sample->full_path && g_strrstr(sample->full_path, "lib/waveform/test/data"), "fname=%s", sample->full_path);

			sample_unref(sample);
			FINISH_TEST;
		}

		wait_for(has_one_item, then, NULL);
	}

	g_timeout_add_once(150, _do_search, NULL);
}
