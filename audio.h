typedef struct __decoder_session _decoder_session;

typedef struct __audition
{
	char           type;     //either TYPE_SNDFILE or TYPE_FLAC

	//sndfile stuff:
	SF_INFO        sfinfo;   //the libsndfile struct pointer
	SNDFILE*       sffile;
	int            nframes;  

	//flac stuff:
	jack_ringbuffer_t* rb1;  //probably move this to __decoder_session ...?
	jack_ringbuffer_t* rb2;
	_decoder_session*  session;
	
} _audition;

struct __decoder_session
{
	sample*       sample;
	unsigned      sample_num;          // not being used currently 
	unsigned      frame_num;           // not being used currently 
	uint64_t      total_samples;       // total_frames * channels
	uint64_t      total_frames;
	short         max[OVERVIEW_WIDTH];
	short         min[OVERVIEW_WIDTH];

	FLAC__FileDecoder* flacstream;     //not used when file is decoded all at once.

	_audition*    audition;            //set only if we are playing rather than generating an overview. Used as a flag? Its a global afterall.....

	char          output_peakfile;     //boolean - true if we are making a pixbuf, false if we are outputting to jack.
};

int    jack_init();
void   jack_close();
//int    jack_process(jack_nframes_t nframes, void *arg);
void   jack_shutdown(void *arg);
int    jack_process_flac(jack_nframes_t nframes, void *arg);

//int    playback_init(char* filename, int id);
int    playback_init(sample* sample);
void   playback_stop();

void   audition_init();
void   audition_reset();
_decoder_session*              flac_decoder_session_new();
gboolean                       flac_decoder_sesssion_init(_decoder_session* session, sample* sample);
void                           decoder_session_free(_decoder_session* session);

gboolean                       get_file_info_flac(sample* sample);
FLAC__FileDecoder*             flac_open(_decoder_session* session);
gboolean                       flac_close(FLAC__FileDecoder* flacstream);
gboolean                       flac_read(FLAC__FileDecoder* flacstream);
void                           flac_decode_file(char* filename);
FLAC__StreamDecoderWriteStatus flac_write_cb(const FLAC__FileDecoder *dec, const FLAC__Frame *frame, const FLAC__int32 * const buf[], _decoder_session* session);
FLAC__StreamDecoderReadStatus  flac_read_cb(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
void                           flac_error_cb(const FLAC__FileDecoder *dec, FLAC__StreamDecoderErrorStatus status, void *data);
void                           flac_metadata_cb(const FLAC__FileDecoder *dec, const FLAC__StreamMetadata *meta, void *data);
gboolean                       flac_fill_ringbuffer(_decoder_session* session);

gboolean                       jack_process_finished(gpointer data);
gboolean                       jack_process_stop_playback(gpointer data);

