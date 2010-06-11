
//#define ayyi_mixer__get_plugin_controls(A) ayyi_container_get_item(&ayyi.mixer->plugins, A)
#define ayyi_mixer__channel_get(A) ayyi_mixer_container_get_item(&ayyi.service->amixer->tracks, A)
#define ayyi_mixer__channel_next(A) ayyi_mixer__container_next_item(&ayyi.service->amixer->tracks, A)
#define ayyi_mixer__count_channels() ayyi_mixer_container_count_items(&ayyi.service->amixer->tracks)
#define ayyi_mixer__plugin_next(A) ayyi_mixer__container_next_item(&ayyi.service->amixer->plugins, A)

gboolean               ayyi_mixer__verify              ();

AyyiChannel*           ayyi_mixer_track_get            (AyyiIdx);
int                    ayyi_mixer_track_count_plugins  (AyyiChannel*);
AyyiPlugin*            ayyi_plugin_get                 (AyyiIdx);
gboolean               ayyi_plugin_pod_index_ok        (AyyiIdx);
AyyiContainer*         ayyi_plugin_get_controls        (AyyiIdx plugin_slot);
struct _ayyi_control*  ayyi_plugin_get_control         (AyyiPlugin*, AyyiIdx control_idx);

void*                  ayyi_mixer_container_get_item   (AyyiContainer*, AyyiIdx);
void*                  ayyi_mixer_container_get_quiet  (AyyiContainer*, AyyiIdx);
//struct _ayyi_control*  ayyi_mixer__get_plugin_control(route_shared* route, int slot);
AyyiControl*           ayyi_mixer__plugin_control_next (AyyiContainer*, AyyiControl*);
void*                  ayyi_mixer__container_next_item (AyyiContainer*, void*);
int                    ayyi_mixer_container_count_items(AyyiContainer*);
gboolean               ayyi_mixer_container_verify     (AyyiContainer*);

struct _ayyi_aux*      ayyi_mixer__aux_get             (AyyiChannel*, AyyiIdx);
struct _ayyi_aux*      ayyi_mixer__aux_get_            (int channel_idx, AyyiIdx);
int                    ayyi_mixer__aux_count           (int channel_idx);

void                   ayyi_mixer__block_print         (AyyiContainer*, int s1);

shm_event*             ayyi_auto_get_event             (AyyiChannel*, int i, int auto_type);
void                   ayyi_automation_print           (AyyiChannel*);

void                   ayyi_channel__set_float         (int prop, int chan, double val);
void                   ayyi_channel__set_bool          (int prop, AyyiIdx chan, gboolean val);

void*                  translate_mixer_address         (void*);
