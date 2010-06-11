#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <jack/jack.h>
#include <glib.h>
typedef void             action;
#include <ayyi/ayyi_typedefs.h>
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/interface.h>
#include <ayyi/ayyi_msg.h>
#include <ayyi/ayyi_time.h>
#include <ayyi/ayyi_client.h>

extern struct _ayyi_client ayyi;

static const uint32_t ticks_per_beat = 4 * TICKS_PER_SUBBEAT;


void
cpos2bbst(SongPos* pos, char* str)
{
  //converts a *songcore* song position to a display string in bars, beats, subbeats, ticks.

  gerr("FIXME shouldnt be using gui fns.");
#if 0
  sprintf(str, "%03i:%02i:%02i:%03i",
						pos_bar ((struct _gsong_pos*)pos),
						pos_beat((struct _gsong_pos*)pos) +1,
						pos->sub / 960 + 1, //0-3
						pos->sub % 960);
#endif
}


long long
cpos2mu(SongPos* pos)
{
  return pos->mu + (pos->sub * CORE_MU_PER_SUB) + (pos->beat * CORE_MU_PER_SUB*CORE_SUBS_PER_BEAT);
}


void
cpos_add_mu(SongPos* pos, unsigned long long mu)
{
  //based on the core function recorder/time.c/song_pos_add_dur().

  mu += pos->mu;
  unsigned long subs = mu / CORE_MU_PER_SUB; //carry overflow into the subs field.
  pos->mu            = mu % CORE_MU_PER_SUB; //use the remainder.
  dbg (0, "mu=%Li remainder=%Li", mu, mu % CORE_MU_PER_SUB);
 
  subs += pos->sub;
  long beats         = subs / CORE_SUBS_PER_BEAT;
  pos->sub           = subs % CORE_SUBS_PER_BEAT;

  dbg (0, "overflow=%li pos->beat=%i", beats, pos->beat);
  pos->beat += beats; //FIXME too high

  dbg(0, "%i:%i:%i", pos->beat, pos->sub, pos->mu);
}


gboolean
song_pos_is_valid(SongPos* core_pos)
{
  return TRUE;
}


void
samples2cpos(uint32_t samples, SongPos* pos)
{
  long long mu = samples2mu(samples);
  mu2cpos(mu, pos);
}


long long 
samples2mu(unsigned long long samples)
{
  //returns the length in mu, of the given number of audio samples.

  //normally, the ratio is of the order of 1924

  long long mu;

  int sample_rate = ayyi.service->song->sample_rate;
  if(!sample_rate){ gerr ("samplerate zero!"); sample_rate=44100; }
  //printf("samples2mu(): samplerate=%i.\n", sample_rate);
 
  float secs = (float)samples / (float)sample_rate; //length in seconds.

  float bpm = ayyi.service->song->bpm;
  float length_of_1_beat_in_secs = 60.0/bpm;

  float beats = secs / length_of_1_beat_in_secs;

  //float remainder = secs - beats * 

  mu = beats2mu(beats);
  //printf("samples2mu(): float=%.3fsecs 1beat=%.3fsecs beats=%.3f mu=%Li\n", secs, length_of_1_beat_in_secs, beats, mu);

  //FIXME this check below indicates there are some largish rounding errors:
  dbg (1, "samples=%Li mu=%Li check=%Li.", samples, mu, mu2samples(mu));
  return mu;
}


long long
mu2samples(long long mu)
{
  float mu_f = (float)mu;
  float beats = mu_f / (CORE_SUBS_PER_BEAT * CORE_MU_PER_SUB);

  dbg(3, "mu=%Li (%.4f) beats=%.4f", mu, mu_f, beats);

  return beats2samples_float(beats);
}


void
mu2cpos(uint64_t mu, SongPos* pos)
{
  ASSERT_POINTER(pos, "pos");

  uint32_t beats = mu / (CORE_MU_PER_SUB * CORE_SUBS_PER_BEAT);

  uint32_t remainder  = mu % (CORE_MU_PER_SUB * CORE_SUBS_PER_BEAT);
  uint32_t subs = remainder / CORE_MU_PER_SUB;
  remainder = remainder % CORE_MU_PER_SUB;

  pos->beat = beats;
  pos->sub  = subs;
  pos->mu   = remainder;
}


void
corepos_set(SongPos* pos, int beat, uint16_t sub, uint16_t mu)
{
  ASSERT_POINTER(pos, "gpos");

  pos->beat = beat;
  pos->sub  = sub;
  pos->mu   = mu;
}


long long
beats2mu(double beats)
{
  //note beats is float.

  return beats * (CORE_MU_PER_SUB * CORE_SUBS_PER_BEAT); //this is the definition of a mu.
}


long long
beats2samples(int beats)
{
	//returns the number of audio samples equivalent to the given number of beats.

	float bpm = ayyi.service->song->bpm;

	float length_of_1_beat_in_secs = 60.0/bpm;
	float samples = length_of_1_beat_in_secs * ayyi.service->song->sample_rate * beats;

	//printf("beats2samples(): %ibeats = %.2fsamples. length_of_1_beat_in_secs=%.2f\n", beats, samples, length_of_1_beat_in_secs);

	return (int)samples;
}


long long
beats2samples_float(float beats)
{
	//returns the number of audio samples equivalent to the given number of beats.

	float bpm = ayyi.service->song->bpm;

	float length_of_1_beat_in_secs = 60.0 / bpm;
	float samples = length_of_1_beat_in_secs * ayyi.service->song->sample_rate * beats;

	dbg (3, "%.4fbeats = %.4fsamples. length_of_1_beat_in_secs=%.4f", beats, samples, length_of_1_beat_in_secs);

	return (int)(samples + 0.5);
}


gboolean
pos_cmp(const SongPos* a, const SongPos* b)
{
	//return TRUE if the two positions are different.
	return (a->beat != b->beat) || (a->sub != b->sub) || (a->mu != b->mu);
}


void
samples2bbst(uint64_t samples, char* str)
{
	if(ayyi.debug>-1) if(samples > beats2samples(6400)){ gwarn ("samples too high? %Lu", samples); str[0]='\0'; return; }

	struct _shm_song* song = ayyi.service->song;

	float beats_per_second = song->bpm / 60.0;

	float secs = (float)samples / (float)ayyi.service->song->sample_rate;
	int ticks = secs * beats_per_second * ticks_per_beat;

	float beats2bar = 4.0;
	int bar = ticks / (ticks_per_beat * (int)beats2bar);
	int ticks_remaining = ticks % (ticks_per_beat * (int)beats2bar);

	int beat = 0;
	if     (ticks_remaining < 1*ticks_per_beat) beat = 0;
	else if(ticks_remaining < 2*ticks_per_beat) beat = 1;
	else if(ticks_remaining < 3*ticks_per_beat) beat = 2;
	else if(ticks_remaining < 4*ticks_per_beat) beat = 3;
	ticks_remaining = ticks_remaining - beat * ticks_per_beat;

	int subbeat = ticks_remaining / TICKS_PER_SUBBEAT;
	ticks_remaining = ticks_remaining % TICKS_PER_SUBBEAT;

	//float seconds_remaining = secs - (subbeat2 * beats_per_second / 4);
	//int tick = seconds_remaining * 3840 / beats_per_second;

	sprintf(str, BBST_FORMAT, bar, beat+1, subbeat+1, ticks_remaining);
	//printf("samples2bbst(): FIXME samples=%Lu %.3fs ticks=%i sub=%i %s %i/4\n", samples, secs, ticks, subbeat, str, (int)beats2bar); 
}


