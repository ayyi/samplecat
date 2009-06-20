struct _ayyi_server
{
	int                  session_num;      // server should support multiple sessions. This is normally 1.
	int                  got_messaging;
	int                  got_shm;
	gboolean             shm_connect_sent;

	GList*               shm_segs;         // list of AyyiShmSeg* owned by us.
	GList*               foreign_shm_segs; // list of foreign segments we are interested in. They may or may not be available and/or imported.
};

AyyiServer* ayyi_server__new               (GList* segs);
void        ayyi_server__add_shm_segment   (AyyiServer*, AyyiShmSeg*);
void        ayyi_server__delete_shm_segment(AyyiServer*, AyyiShmSeg*);
void        ayyi_server__shm_segment_reset (AyyiServer*, char* name);

void        ayyi_server__add_foreign_shm_segment(AyyiServer*, void* shmptr);

gboolean    shm_import                     (AyyiServer*);

