#ifndef __SM_ARDOUR__
#define __SM_ARDOUR__

#define AYYI_SHM_VERSION 1
#define CONTAINER_SIZE 128
#define BLOCK_SIZE 128
#define AYYI_BLOCK_SIZE 128
#define MAX_LOC 10
#define AYYI_FILENAME_MAX 256
#define AYYI_NAME_MAX 64
#define AYYI_PLUGINS_PER_CHANNEL 3
#define AYYI_AUX_PER_CHANNEL 3

struct _container {
	struct block* block[CONTAINER_SIZE];
	int           last;
	int           full;
	char          signature[16];
};

struct block
{
	void*       slot[BLOCK_SIZE];
	int         last;
	int         full;
	void*       next;
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
	uint32_t    sample_rate;       //TODO move to global segment
	struct _song_pos start;
	struct _song_pos end;
	struct _song_pos locators[MAX_LOC];
	double      bpm;
	uint32_t    transport_frame;
	float       transport_speed;
	int         play_loop;         //boolean
	int         rec_enabled;       //boolean

	struct _container regions;
	struct _container midi_regions;
	struct _container filesources;
	struct _container connections;
	struct _container audio_tracks;
	struct _container midi_tracks;
	struct _container playlists;
	struct block ports;
	struct _container vst_plugin_info;
};
typedef struct _shm_song Shm_song;

enum {
	ARDOUR_TYPE_MASK  = 0xf0000000,
	ARDOUR_TYPE_TRACK = 0x10000000,
	ARDOUR_TYPE_PART  = 0x20000000,
	ARDOUR_TYPE_FILE  = 0x30000000,
	ARDOUR_TYPE_SONG  = 0x40000000,
};

struct _region_base_shared {
	int            shm_idx; // slot_num | (block_num / BLOCK_SIZE)
	char           name[64];
	uint64_t       id;
	void*          ardour_object;
	char           playlist_name[64];
	jack_nframes_t position;
	jack_nframes_t length;
};

struct _region_shared {
	int            shm_idx;
	char           name[64];
	uint64_t       id;
	void*          ardour_object;
	char           playlist_name[64];
	jack_nframes_t position;
	jack_nframes_t length;
	// end base.
	jack_nframes_t start;
	int            flags;
	uint64_t       source0;
	char           channels;
	uint32_t       fade_in_length;
	uint32_t       fade_out_length;
	float          level;
};
typedef struct _region_shared region_shared;

struct _midi_region_shared {
	int            shm_idx;
	char           name[64];
	uint64_t       id;
	void*          ardour_object;
	char           playlist_name[64];
	jack_nframes_t position;
	jack_nframes_t length;
	// end base.
	struct _container events;
};

struct _filesource_shared {
	int            shm_idx;
	char           name[AYYI_FILENAME_MAX];
	uint64_t       id;
	void*          object;
	uint32_t       length;
};

struct _ayyi_connection {
	int            shm_idx;
	char           device[32];
	char           name[256];
	void*          object;
	uint32_t       nports;
	char           io;     // 0=output 1=input
};

struct _route_shared {
	int            shm_idx;
	char           name[256];
	uint64_t       id;
	void*          object;
	char           input_name[128];
	int            input_idx;
	int            output_idx;
	int            input_minimum;
	int            input_maximum;
	int            output_minimum;
	int            output_maximum;

	//routing:
	char           output_name[128];
	struct _ayyi_list* output_routing;

	uint32_t       gui_colour;
	int/*ChanFlags*/ flags;
	char           muted;  //deprecated - use flags
	char           solod;  //deprecated - use flags
	char           armed;  //deprecated - use flags
	int            nchans;
	float          visible_peak_power[2];
};
typedef struct _route_shared route_shared;

struct _midi_track_shared {
	int            shm_idx;
	char           name[256];
	uint64_t       id;
	void*          object;
	char           muted;
	char           solod;
	char           armed;
};
typedef struct _midi_track_shared midi_track_shared;

struct _playlist_shared {
	int            pod_idx;
	char           name[256];
	uint64_t       id;
	void*          object;
};

struct _port_shared {
	char           name[256];
	void*          object;
};

struct _ayyi_channel {
	int            shm_idx;
	char           name[256];
	void*          object;
	double         level;
	int            has_pan;
	float          pan;
	float          visible_peak_power[2];
	int            plugin         [AYYI_PLUGINS_PER_CHANNEL];
	char           insert_active  [AYYI_PLUGINS_PER_CHANNEL];
	char           insert_bypassed[AYYI_PLUGINS_PER_CHANNEL];
	struct _ayyi_aux* aux         [AYYI_AUX_PER_CHANNEL];
	struct _container automation[2];
	struct _ayyi_list* automation_list;
};
typedef struct _ayyi_channel AyyiChannel;

struct _plugin_shared {
	char           name[256];
	char           category[64];
	uint32_t       n_inputs;
	uint32_t       n_outputs;
	uint32_t       latency;
	struct _container controls;
};
typedef struct _plugin_shared plugin_shared;

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

struct _ayyi_aux
{
	int           idx;
	float         level;
	float         pan;
	int           flags;
	int           bus_num;
};

struct _shm_seg_mixer
{
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
typedef struct _shm_seg_mixer Shm_seg_mixer;

#endif //__SM_ARDOUR__
