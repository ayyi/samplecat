
#define ayyi_song__audio_track_get(A) (AyyiTrack*)ayyi_song_container_get_item(&ayyi.service->song->audio_tracks, A)
#define ayyi_song__midi_track_get(A) (AyyiMidiTrack*)ayyi_song_container_get_item(&ayyi.service->song->midi_tracks, A)
#define ayyi_song__audio_region_get(A) (AyyiAudioRegion*)ayyi_song_container_get_item(&ayyi.service->song->audio_regions, A)
#define ayyi_song__midi_region_get(A) (AyyiMidiRegion*)ayyi_song_container_get_item(&ayyi.service->song->midi_regions, A)
#define ayyi_song__connection_get(A) (AyyiConnection*)ayyi_song_container_get_item(&ayyi.service->song->connections, A)
#define ayyi_song__filesource_get(A) (AyyiFilesource*)ayyi_song_container_get_item(&ayyi.service->song->filesources, A)

#define ayyi_song__audio_track_next(A) ayyi_song_container_next_item(&ayyi.service->song->audio_tracks, A)
#define ayyi_song__midi_track_next(A) ayyi_song_container_next_item(&ayyi.service->song->midi_tracks, A)
#define ayyi_song__audio_region_next(A) ayyi_song_container_next_item(&ayyi.service->song->audio_regions, A)
#define ayyi_song__midi_region_next(A) ayyi_song_container_next_item(&ayyi.service->song->midi_regions, A)
#define ayyi_song__midi_region_pod_index_ok(A) ayyi_container_index_ok(&ayyi.service->song->midi_regions, A)
#define ayyi_song__audio_connection_next(A) ayyi_song_container_next_item(&ayyi.service->song->connections, A)
#define ayyi_song__playlist_next(A) ayyi_song_container_next_item(&ayyi.service->song->playlists, A)
#define ayyi_song__filesource_next(A) ayyi_song_container_next_item(&ayyi.service->song->filesources, A)

#define ayyi_song__get_audio_track_count() ayyi_song_container_count_items(&ayyi.service->song->audio_tracks)
#define ayyi_song__get_midi_track_count() ayyi_song_container_count_items(&ayyi.service->song->midi_tracks)
#define ayyi_song__get_file_count() ayyi_song_container_count_items(&ayyi.service->song->filesources)
#define ayyi_song__get_channel_count() ayyi_song_container_count_items(&ayyi.service->song->connections)
#define ayyi_song__get_region_count() ayyi_song_container_count_items(&ayyi.service->song->audio_regions)

#define ayyi_midi_note_prev(A) ayyi_song_container_prev_item(container, A)

//#define ayyi_midi_region_get_block(A) ayyi_client_container_get_block(&ayyi.service->song->midi_regions, A)

#define AYYI_TRACK_IS_MASTER(A) ((A->flags & master) !=  0)

#define ARDOUR_SOUND_DIR1 "interchange/"
#define ARDOUR_SOUND_DIR2 "audiofiles"

void*           ayyi_song__translate_address    (void* address);
void            ayyi_song__print_automation_list(AyyiChannel*);
AyyiTrack*      ayyi_song__track_lookup_by_id   (uint64_t id);
gboolean        ayyi_song__is_shm               (void*);
shm_seg*        ayyi_song__get_info             ();
GList*          ayyi_song__get_audio_part_list  ();
const char*     ayyi_song__get_file_path        ();
void            ayyi_song__get_audiodir         (char* dir);
void            ayyi_song__print_pool           ();
void            ayyi_song__print_part_list      ();

AyyiItem*       ayyi_song_container_get_item    (AyyiContainer*, int idx);
void*           ayyi_song_container_next_item   (AyyiContainer*, void* prev);
void*           ayyi_song_container_prev_item   (AyyiContainer*, void* old);
int             ayyi_song_container_count_items (AyyiContainer*);
int             ayyi_song_container_find        (AyyiContainer*, void*);
gboolean        ayyi_song_container_verify      (AyyiContainer*);

gboolean        ayyi_region_pod_index_ok        (AyyiIdx);

AyyiAction*     ayyi_region__delete_async       (AyyiAudioRegion*);
AyyiAudioRegion*ayyi_region_lookup_by_name      (char*);
void            ayyi_region_name_make_unique    (char* source_name_unique, const char* source_name);
AyyiIdx         ayyi_region_get_pod_index       (AyyiRegionBase*);
uint32_t        ayyi_region_get_start_offset    (AyyiAudioRegion*);
AyyiAudioRegion*ayyi_region__get_from_id        (uint64_t id);

AyyiTrack*      ayyi_track__next_armed          (AyyiTrack*);
AyyiConnection* ayyi_track__get_output          (AyyiTrack*);
const char*     ayyi_track__get_output_name     (AyyiTrack*);
gboolean        ayyi_track__index_ok            (AyyiIdx);
AyyiChannel*    ayyi_track__get_channel         (AyyiTrack*);
void            ayyi_track__make_name_unique    (char* unique, const char* name);

void            ayyi_song__print_midi_tracks    ();

gboolean        ayyi_file_get_other_channel     (AyyiFilesource*, char* other);
AyyiFilesource* ayyi_filesource_get_from_id     (uint64_t id);

gboolean        ayyi_connection_is_input        (AyyiConnection*);
AyyiConnection* ayyi_connection_next_input      (AyyiConnection*, int n_chans);

GList*          ayyi_song__get_files_list       ();
gboolean        ayyi_song__have_file            (const char*);

gboolean        ayyi_song__verify               (shm_seg*);

