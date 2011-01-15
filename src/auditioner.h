
#ifdef __audtioner_c__
struct _auditioner
{
	DBusGProxy*    proxy;
};
#endif

void auditioner_connect();
void auditioner_play(Sample*);
void auditioner_stop(Sample*);
void toggle_playback(Sample*);

