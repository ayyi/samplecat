
#define ayyi_song__audio_track_get(A) ayyi_song_container_get_item(&ayyi.song->audio_tracks, A)
#define ayyi_song__midi_track_get(A) ayyi_song_container_get_item(&ayyi.song->midi_tracks, A)
#define ayyi_song__audio_region_get(A) ayyi_song_container_get_item(&ayyi.song->regions, A)
#define ayyi_song__midi_region_get(A) ayyi_song_container_get_item(&ayyi.song->midi_regions, A)
#define ayyi_song__connection_get(A) ayyi_song_container_get_item(&ayyi.song->connections, A)

#define ayyi_song__audio_track_next(A) ayyi_song_container_next_item(&ayyi.song->audio_tracks, A)
#define ayyi_song__midi_track_next(A) ayyi_song_container_next_item(&ayyi.song->midi_tracks, A)
#define ayyi_song__audio_region_next(A) ayyi_song_container_next_item(&ayyi.song->regions, A)
#define ayyi_song__midi_region_next(A) ayyi_song_container_next_item(&ayyi.song->midi_regions, A)
#define ayyi_song__midi_region_pod_index_ok(A) ayyi_container_index_ok(&ayyi.song->midi_regions, A)
#define ayyi_song__audio_connection_next(A) ayyi_song_container_next_item(&ayyi.song->connections, A)
#define ayyi_song__playlist_next(A) ayyi_song_container_next_item(&ayyi.song->playlists, A)
#define ayyi_song__filesource_next(A) ayyi_song_container_next_item(&ayyi.song->filesources, A)

#define ayyi_song__get_audio_track_count() ayyi_song_container_count_items(&ayyi.song->audio_tracks)
#define ayyi_song__get_file_count() ayyi_song_container_count_items(&ayyi.song->filesources)
#define ayyi_song__get_channel_count() ayyi_song_container_count_items(&ayyi.song->connections)
#define ayyi_song__get_region_count() ayyi_song_container_count_items(&ayyi.song->regions)

#define IS_MASTER(A) ((A->flags & master) !=  0)

void*    ayyi_song_translate_address     (void* address);
void     ayyi_song__print_automation_list(AyyiChannel*);
int      ayyi_song__track_lookup_by_id   (uint64_t id);
gboolean ayyi_song__is_shm               (void*);
shm_seg* ayyi_song__get_info             ();
GList*   ayyi_song__get_audio_part_list  ();

void*    ayyi_song_container_get_item    (AyyiContainer*, int idx);
void*    ayyi_song_container_next_item   (AyyiContainer*, void* prev);
void*    ayyi_song_container_prev_item   (AyyiContainer*, void* old);
int      ayyi_song_container_count_items (AyyiContainer*);
int      ayyi_song_container_find        (AyyiContainer*, void*);
gboolean ayyi_song_container_verify      (AyyiContainer*);

gboolean      ayyi_track_pod_index_ok    (int pod_index);
gboolean      ayyi_region_pod_index_ok   (int shm_index);

int           ayyi_part_get_track_num    (region_base_shared*);

route_shared* ayyi_track_next_armed      (route_shared*);

gboolean      ayyi_file_get_other_channel(filesource_shared*, char* other);
void          ayyi_song__pool_print      ();

