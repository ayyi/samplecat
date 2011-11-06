#ifndef __ayyi_interface_h__
#define __ayyi_interface_h__
#ifdef __cplusplus
namespace Ayi {
using namespace Ayi;
#endif //__cplusplus

#include "stdint.h"
#include <jack/jack.h>

#define AYYI_SHM_VERSION 4
#define CONTAINER_SIZE 128
#define AYYI_BLOCK_SIZE 128
#define MAX_LOC 10
#define AYYI_FILENAME_MAX 256
#define AYYI_NAME_MAX 64
#define AYYI_PLUGINS_PER_CHANNEL 3
#define AYYI_AUX_PER_CHANNEL 3

struct _block
{
	void*       slot[AYYI_BLOCK_SIZE];
	int         last;
	int         full;
	void*       next;
};

struct _container {
	struct _block* block[CONTAINER_SIZE];
	int           last;
	int           full;
	char          signature[16];
	AyyiObjType   obj_type;
};

struct _ayyi_list
{
	struct _container* data;
	void*              next;
	int                id;
	char               name[32];
};

typedef struct _shm_virtual
{
	char        service_name[16];
	char        service_type[16];
	int         version;
	void*       owner_shm;
	int         num_pages;

	void*       tlsf_pool;
} shm_virtual;

struct _shm_song
{
	char        service_name[16];
	char        service_type[16];
	int         version;
	void*       owner_shm;         //clients need this so they can calculate address offsets.
	int         num_pages;
	void*       tlsf_pool;

	char        path[256];
	char        snapshot[256];
	char        peaks_dir[128];
	uint32_t    sample_rate;       //TODO move to global segment
	struct _song_pos start;
	struct _song_pos end;
	struct _song_pos locators[MAX_LOC];
	double      bpm;
	uint32_t    transport_frame;
	float       transport_speed;
	int         play_loop;         //boolean
	int         rec_enabled;       //boolean

	struct _container audio_regions;
	struct _container midi_regions;
	struct _container filesources;
	struct _container connections;
	struct _container audio_tracks;
	struct _container midi_tracks;
	struct _container playlists;
	struct _block ports;
	struct _container vst_plugin_info;
};

struct _ayyi_base_item {
	int            shm_idx; // slot_num | (block_num / AYYI_BLOCK_SIZE)
	char           name[AYYI_NAME_MAX];
	AyyiId         id;
	void*          server_object;
	int            flags;
};

struct _region_base_shared {
	int            shm_idx; // slot_num | (block_num / AYYI_BLOCK_SIZE)
	char           name[AYYI_NAME_MAX];
	AyyiId         id;
	void*          server_object;
	int            flags;
	int            playlist;
	jack_nframes_t position;
	jack_nframes_t length;
};

struct _ayyi_audio_region {
	int            shm_idx;
	char           name[AYYI_NAME_MAX];
	AyyiId         id;
	void*          server_object;
	int            flags;
	int            playlist;
	jack_nframes_t position;
	jack_nframes_t length;
	// end base.
	jack_nframes_t start;
	uint64_t       source0;
	char           channels;
	uint32_t       fade_in_length;
	uint32_t       fade_out_length;
	float          level;
};

struct _midi_region_shared {
	int            shm_idx;
	char           name[AYYI_NAME_MAX];
	AyyiId         id;
	void*          server_object;
	int            flags;
	int            playlist;
	jack_nframes_t position;
	jack_nframes_t length;
	// end base.
	struct _container events;
};

struct _filesource_shared {
	int            shm_idx;
	char           name[AYYI_FILENAME_MAX]; //can be either absolute path, or relative to current song path.
	char           original_name[AYYI_NAME_MAX];
	uint64_t       id;
	void*          object;
	uint32_t       length;
};

struct _ayyi_connection {
	int            shm_idx;
	char           name[AYYI_NAME_MAX];
	uint64_t       id;
	void*          object;
	int            flags;
	char           device[32];
	uint32_t       nports;
	char           io;     // 0=output 1=input
};

struct _route_shared {
	int            shm_idx;
	char           name[AYYI_NAME_MAX];
	uint64_t       id;
	void*          object;
	uint32_t/*ChanFlags*/ flags;
	int            colour;
	char           input_name[128];
	int            input_idx;
	struct _ayyi_list* input_routing;
	struct _ayyi_list* output_routing;
	int            nchans;
	float          visible_peak_power[2];
};

struct _midi_track_shared {
	int            shm_idx;
	char           name[AYYI_NAME_MAX];
	uint64_t       id;
	void*          object;
	int            flags;
	uint32_t       colour;
};
typedef struct _midi_track_shared midi_track_shared;

struct _playlist_shared {
	int            shm_idx;
	char           name[AYYI_NAME_MAX];
	uint64_t       id;
	void*          object;
	int            flags;
	int            track;
};

struct _port_shared {
	char           name[AYYI_NAME_MAX];
	void*          object;
};

struct _ayyi_channel {
	int            shm_idx;
	char           name[AYYI_FILENAME_MAX];
	void*          object;
	double         level;
	int            has_pan;
	float          pan;
	float          visible_peak_power[2];
	int            plugin          [AYYI_PLUGINS_PER_CHANNEL];
	char           insert_active   [AYYI_PLUGINS_PER_CHANNEL];
	char           insert_bypassed [AYYI_PLUGINS_PER_CHANNEL];
	struct _ayyi_aux* aux          [AYYI_AUX_PER_CHANNEL];
	struct _container automation[2];
	struct _ayyi_list* automation_list;
};

struct _plugin_shared {
	int            shm_idx;
	char           name[AYYI_FILENAME_MAX];
	char           category[64];
	uint32_t       n_inputs;
	uint32_t       n_outputs;
	uint32_t       latency;
	struct _container controls;
};

struct _ayyi_control {
	char           name[32];
};

#ifdef USE_VST
struct _shm_vst_info {
	FSTInfo        fst_info;
};
typedef struct _shm_vst_info shm_vst_info;
#endif

struct _shm_event {
	double when;
	double value;
};
typedef struct _shm_event shm_event;

struct _midi_note {
	uint16_t       idx;
	uint8_t        note;
	uint8_t        velocity;
	jack_nframes_t start;
	jack_nframes_t length;
};

struct _ayyi_aux {
	int           idx;
	float         level;
	float         pan;
	int           flags;
	int           bus_num;
};

struct _shm_seg_mixer {
	char          service_name[16];
	char          service_type[16];
	int           version;
	void*         owner_shm;         //clients need this so they can calculate address offsets.
	int           num_pages;
	void*         tlsf_pool;

	AyyiChannel       master;
	struct _container tracks;
	struct _container plugins;
};

#ifdef __cplusplus
}      //namspace Ayi
#endif //__cplusplus
#endif //__ayyi_interface_h__
