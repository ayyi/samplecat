#ifndef __ayyi_typedefs_h__
#define __ayyi_typedefs_h__

typedef struct _ayyi_server        AyyiServer;
typedef struct _ayyi_shm_seg       AyyiShmSeg;

typedef struct _ayyi_action        AyyiAction;
typedef struct _ayyi_obj_idx       AyyiObjIdx;
typedef struct _shm_seg            shm_seg;
typedef enum   _seg_type           SegType;
typedef struct _song_pos           song_pos;
typedef struct block               block;
typedef struct _route_shared       AyyiRoute;
typedef struct _route_shared       AyyiTrack;
typedef struct _region_base_shared AyyiRegionBase;
typedef struct _region_shared      AyyiRegion;
typedef struct _region_shared      AyyiAudioRegion;
typedef struct _midi_region_shared AyyiMidiRegion;
typedef struct _filesource_shared  filesource_shared;
typedef struct _playlist_shared    AyyiPlaylist;
typedef struct _ayyi_connection    AyyiConnection;

#endif // __ayyi_typedefs_h__
