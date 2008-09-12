/*

Ayyi uses 2 time formats; one sample based and one musical-time based.

The musical formats are high precision in order to allow conversion to and from other formats.

*/
#ifdef __cplusplus
extern "C" {
#endif

#define CORE_MU_PER_SUB 11025
#define CORE_SUBS_PER_BEAT 3840

void           cpos2bbst        (song_pos*, char* bbst);
long long      cpos2mu          (song_pos*);
void           cpos_add_mu      (song_pos*, unsigned long long mu);
gboolean       song_pos_is_valid(song_pos*);
void           samples2cpos     (uint32_t samples, song_pos*);
long long      samples2mu       (unsigned long long samples);
long long      mu2samples       (long long mu);
void           mu2cpos          (uint64_t len, song_pos*);
void           corepos_set      (song_pos*, int beat, uint16_t sub, uint16_t mu);
long long      beats2mu         (double beats);
long long      beats2samples_float(float beats);
gboolean       pos_cmp          (const song_pos*, const song_pos*);

#ifdef __cplusplus
}
#endif
