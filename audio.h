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

