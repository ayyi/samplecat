typedef struct __decoder_session _decoder_session;

typedef struct __audition
{
	char           type;     //either TYPE_SNDFILE or TYPE_FLAC

	//sndfile stuff:
	SF_INFO        sfinfo;   //the libsndfile struct pointer
	SNDFILE        *sffile;
	int            nframes;  

	//flac stuff:
	jack_ringbuffer_t* rb;  //probably move this to __decoder_session ...?
	_decoder_session*  session;
	
} _audition;

struct __decoder_session
{
	sample*       sample;
	unsigned      sample_num; 
	uint64_t      total_samples;
	short         max[OVERVIEW_WIDTH];
	short         min[OVERVIEW_WIDTH];

	_audition*    audition;            //set only if we are playing rather than generating an overview. Used as a flag? Its a global afterall.....

};

int    jack_init();
//int    jack_process(jack_nframes_t nframes, void *arg);
void   jack_shutdown(void *arg);
int    jack_process_flac(jack_nframes_t nframes, void *arg);

//int    playback_init(char* filename, int id);
int    playback_init(sample* sample);
void   playback_stop();

gboolean                       get_file_info_flac(sample* sample);
void                           flac_sesssion_init(_decoder_session* session, sample* sample);
FLAC__FileDecoder*             flac_open(_decoder_session* session);
gboolean                       flac_close(FLAC__FileDecoder* flacstream);
gboolean                       flac_read(FLAC__FileDecoder* flacstream);
void                           flac_decode(char* filename);
FLAC__StreamDecoderWriteStatus flac_write_cb(const FLAC__FileDecoder *dec, const FLAC__Frame *frame, const FLAC__int32 * const buf[], void *data);
FLAC__StreamDecoderReadStatus  flac_read_cb(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
void                           flac_error_cb(const FLAC__FileDecoder *dec, FLAC__StreamDecoderErrorStatus status, void *data);
void                           flac_metadata_cb(const FLAC__FileDecoder *dec, const FLAC__StreamMetadata *meta, void *data);
void                           flac_fill_ringbuffer(_decoder_session* session);

