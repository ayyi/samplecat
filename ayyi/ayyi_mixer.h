
//#define ayyi_mixer__get_plugin_controls(A) ayyi_container_get_item(&ayyi.mixer->plugins, A)
#define ayyi_mixer__channel_at(A) ayyi_mixer_container_get_item(&ayyi.service->amixer->tracks, A)
#define ayyi_mixer__channel_next(A) ayyi_mixer__container_next_item(&ayyi.service->amixer->tracks, A)
#define ayyi_mixer__count_channels() ayyi_mixer_container_count_items(&ayyi.service->amixer->tracks)
#define ayyi_mixer__plugin_next(A) ayyi_mixer__container_next_item(&ayyi.service->amixer->plugins, A)

gboolean               ayyi_mixer__verify              ();

AyyiChannel*           ayyi_mixer__channel_at_quiet    (AyyiIdx);
int                    ayyi_channel__count_plugins     (AyyiChannel*);
AyyiPlugin*            ayyi_plugin_at                  (AyyiIdx);
gboolean               ayyi_plugin_pod_index_ok        (AyyiIdx);
AyyiContainer*         ayyi_plugin_get_controls        (AyyiIdx plugin_slot);
AyyiControl*           ayyi_plugin_get_control         (AyyiPlugin*, AyyiIdx control_idx);

void*                  ayyi_mixer_container_get_item   (AyyiContainer*, AyyiIdx);
void*                  ayyi_mixer_container_get_quiet  (AyyiContainer*, AyyiIdx);
//struct _ayyi_control*  ayyi_mixer__get_plugin_control(route_shared* route, int slot);
AyyiControl*           ayyi_mixer__plugin_control_next (AyyiContainer*, AyyiControl*);
void*                  ayyi_mixer__container_next_item (AyyiContainer*, void*);
int                    ayyi_mixer_container_count_items(AyyiContainer*);
gboolean               ayyi_mixer_container_verify     (AyyiContainer*);

struct _ayyi_aux*      ayyi_mixer__aux_get             (AyyiChannel*, AyyiIdx);
struct _ayyi_aux*      ayyi_mixer__aux_get_            (AyyiIdx channel, AyyiIdx);
int                    ayyi_mixer__aux_count           (AyyiIdx channel);

AyyiTrack*             ayyi_channel__get_track         (AyyiChannel*);
shm_event*             ayyi_channel__get_auto_event    (AyyiChannel*, int i, int auto_type);
void                   ayyi_automation_print           (AyyiChannel*);

void                   ayyi_channel__set_float         (int prop, int chan, double val);
void                   ayyi_channel__set_bool          (int prop, AyyiIdx chan, gboolean val);
AyyiList*              ayyi_channel__get_routing       (AyyiChannel*);

void                   ayyi_print_mixer                ();

void*                  translate_mixer_address         (void*);
