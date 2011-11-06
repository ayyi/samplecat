#ifndef __ayyi_typedefs_h__
#define __ayyi_typedefs_h__

#include <stdint.h>
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ayyi_server         AyyiServer;
typedef struct _ayyi_client         AyyiClient;
typedef struct _ayyi_shm_seg        AyyiShmSeg;
typedef struct _shm_song            AyyiShmSong;
typedef struct _shm_seg_mixer       AyyiShmMixer;

typedef struct _ayyi_action         AyyiAction;
typedef struct _ayyi_obj_idx        AyyiObjIdx;
typedef enum   _ayyi_op_type        AyyiOpType;
typedef enum   _ayyi_prop_type      AyyiPropType;
typedef struct _ident               AyyiIdent;
typedef struct _service             Service;
typedef struct _shm_seg             shm_seg;
typedef enum   _seg_type            SegType;
typedef struct _song_pos            SongPos;
typedef struct _container           AyyiContainer;
typedef struct _block               block;
typedef struct _ayyi_base_item      AyyiItem;
typedef struct _route_shared        AyyiTrack;
typedef struct _ayyi_channel        AyyiChannel;
typedef struct _region_base_shared  AyyiRegionBase;
typedef struct _ayyi_audio_region   AyyiRegion;
typedef struct _ayyi_audio_region   AyyiAudioRegion;
typedef struct _midi_region_shared  AyyiMidiRegion;
typedef struct _filesource_shared   AyyiFilesource;
typedef struct _playlist_shared     AyyiPlaylist;
typedef struct _ayyi_connection     AyyiConnection;
typedef struct _midi_track_shared   AyyiMidiTrack;
typedef struct _ayyi_aux            AyyiAux;
typedef struct _ayyi_control        AyyiControl;
typedef struct _plugin_shared       AyyiPlugin;
typedef struct _midi_note           MidiNote;
typedef struct _launch_info         AyyiLaunchInfo;
typedef struct _handler_data        HandlerData;

typedef int                         AyyiIdx;
typedef uint32_t                    AyyiObjType;
typedef uint64_t                    AyyiId;

typedef void   (*AyyiCallback)      ();
typedef void   (*AyyiCallback22)    (gpointer user_data); //tmp!
typedef void   (*AyyiFinish)        (AyyiIdent, gpointer);
typedef void   (*AyyiHandler2)      (GError*, gpointer);
typedef void   (*AyyiHandler)       (AyyiIdent, GError**, gpointer);
typedef void   (*AyyiPropHandler)   (AyyiIdent, GError**, AyyiPropType, gpointer);
//typedef void   (*AyyiHandler6)      (AyyiIdent, gpointer, GError**);
typedef void   (*AyyiActionCallback)(AyyiAction*);

#ifndef true
#define true  1
#define false 0
#endif

#endif // __ayyi_typedefs_h__

#ifdef __cplusplus
}
#endif
