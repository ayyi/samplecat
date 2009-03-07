//#define ayyi_mixer__get_plugin_controls(A) ayyi_container_get_item(&ayyi.mixer->plugins, A)
#define ayyi_mixer__channel_get(A) ayyi_mixer_container_get_item(&ayyi.amixer->tracks, A)
#define ayyi_mixer__channel_next(A) ayyi_mixer__container_next_item(&ayyi.amixer->tracks, A)
#define ayyi_mixer__count_channels() ayyi_mixer_container_count_items(&ayyi.amixer->tracks)

AyyiChannel*           ayyi_mixer_track_get(int slot);
int                    ayyi_mixer_track_count_plugins(AyyiChannel*);
struct _plugin_shared* ayyi_plugin_get     (int slot);
gboolean               ayyi_plugin_pod_index_ok(int pod_index);
AyyiContainer*         ayyi_plugin_get_controls(int plugin_slot);
struct _ayyi_control*  ayyi_plugin_get_control(plugin_shared*, int control_idx);

void*                  ayyi_mixer_container_get_item(AyyiContainer*, int idx);
//struct _ayyi_control*  ayyi_mixer__get_plugin_control(route_shared* route, int slot);
struct _ayyi_control*  ayyi_mixer__plugin_control_next(AyyiContainer*, struct _ayyi_control*);
void*                  ayyi_mixer__container_next_item(AyyiContainer*, void*);
int                    ayyi_mixer_container_count_items(AyyiContainer*);
gboolean               ayyi_mixer_container_verify(AyyiContainer*);

struct _ayyi_aux*      ayyi_mixer__aux_get(AyyiChannel*, int idx);
struct _ayyi_aux*      ayyi_mixer__aux_get_(int channel_idx, int idx);
int                    ayyi_mixer__aux_count(int channel_idx);

void                   ayyi_mixer__block_print(AyyiContainer*, int s1);

shm_event*             ayyi_auto_get_event(AyyiChannel*, int i, int auto_type);
void                   ayyi_automation_print(AyyiChannel*);

void*                  translate_mixer_address(void*);
