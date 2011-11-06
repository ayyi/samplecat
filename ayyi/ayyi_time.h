/*

Ayyi uses 2 time formats; one sample based and one musical-time based.

The musical formats are high precision in order to allow conversion to and from other formats.

*/
#ifdef __cplusplus
extern "C" {
#endif
#include "glib.h"

#define CORE_MU_PER_SUB 11025
#define CORE_SUBS_PER_BEAT 3840
#define TICKS_PER_SUBBEAT 384     // there seems to be confusion over whether this is 960 or 384!
#define BBST_FORMAT "%03i:%02i:%02i:%03i"

void           cpos2bbst           (SongPos*, char* bbst);
long long      cpos2mu             (SongPos*);
void           cpos_add_mu         (SongPos*, unsigned long long mu);
gboolean       song_pos_is_valid   (SongPos*);
void           samples2cpos        (uint32_t samples, SongPos*);
long long      samples2mu          (unsigned long long samples);
long long      mu2samples          (long long mu);
void           mu2cpos             (uint64_t len, SongPos*);
void           corepos_set         (SongPos*, int beat, uint16_t sub, uint16_t mu);
long long      beats2mu            (double beats);
long long      ayyi_beats2samples  (int beats);
long long      ayyi_beats2samples_float (float beats);
gboolean       pos_cmp             (const SongPos*, const SongPos*);
void           samples2bbst        (uint64_t samples, char* str);

#ifdef __cplusplus
}
#endif
