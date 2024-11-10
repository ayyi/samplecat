
void
test_1 ()
{
	START_TEST;

	{
		Sample* sample = sample_new();
		assert(sample->ref_count == 1, "ref_count %i (expected 1)", sample->ref_count);
		sample_unref(sample);
	}

	{
		const char* name = "mono_0:10.wav";

		char* path = find_wav(name);
		Sample* sample = sample_new_from_filename(path, true);
		assert(!strcmp(path, sample->full_path), "full_path");

		assert(!strcmp(sample->name, name), "mimetype %s (expected %s)", sample->name, name);
		assert(!strcmp(sample->mimetype, "audio/x-wav") || !strcmp(sample->mimetype, "audio/vnd.wave"), "mimetype %s (expected %s)", sample->mimetype, "audio/x-wav");

		bool info_ok = sample_get_file_info(sample);
		assert(info_ok, "get info");
		assert(sample->channels == 1, "channels %i (expected 1)", sample->channels);

		Sample* sample2 = sample_dup(sample);
		assert(sample2 != sample, "sample not duped");
		assert(!strcmp(sample2->name, name), "dupe mimetype %s (expected %s)", sample->name, name);

		sample_unref(sample);
		sample_unref(sample2);
	}

	FINISH_TEST;
}
