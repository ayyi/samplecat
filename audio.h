typedef struct __audition
{
	SF_INFO        sfinfo;   //the libsndfile struct pointer
	SNDFILE        *sffile;
	int            nframes;  
	
} _audition;

int    jack_init();
//int    jack_process(jack_nframes_t nframes, void *arg);
void   jack_shutdown(void *arg);

int    playback_init(char* filename, int id);
void   playback_stop();

gboolean                       get_file_info_flac(sample* sample);
void                           flac_decode(char* filename);
FLAC__StreamDecoderWriteStatus flac_write_cb(const FLAC__FileDecoder *dec, const FLAC__Frame *frame, const FLAC__int32 * const buf[], void *data);
FLAC__StreamDecoderReadStatus  flac_read_cb(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
void                           flac_error_cb(const FLAC__FileDecoder *dec, FLAC__StreamDecoderErrorStatus status, void *data);
