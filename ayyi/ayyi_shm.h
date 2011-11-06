#ifndef __ayyi_shm_h__
#define __ayyi_shm_h__

#define AYYI_SHM_BLOCK_SIZE 4096

enum _seg_type
{
	SEG_TYPE_NOT_SET,
	SEG_TYPE_SONG,
	SEG_TYPE_MIXER,
	AYYI_SEG_TYPE_PLUGIN, //TODO
	AYYI_SEG_TYPE_BUFFER,
	AYYI_SEG_TYPE_LAST
};

#ifdef __ayyi_shm_c__
char seg_strs[AYYI_SEG_TYPE_LAST][16] = {"", "song", "mixer", "plugin", "buffer"};
#else
extern char seg_strs[5][16];
#endif

struct _shm_seg
{
	int      id;
	int      type;
	int      size; //bytes
	gboolean attached;
	gboolean invalid;
	void*    address;
};

//server version of _shm_seg - probably should be merged with client version.
struct _ayyi_shm_seg
{
	void*    address;
	char     name[64];
	int      type;
	int      id;
	int      size; //bytes
	gboolean attached;

	long     _shmkey;
	int      _shmfd;
};


gboolean ayyi_client_container_verify   (AyyiContainer*, void* (*translator)(void* address));
void*    ayyi_container_next_item       (AyyiContainer*, AyyiItem* prev, void* (*translator)(void* address));
//void*    ayyi_container_prev_item       (AyyiContainer*, void* old);
void*    ayyi_container_get_item        (AyyiContainer*, AyyiIdx, void* (*translator)(void* address));
void*    ayyi_container_get_item_quiet  (AyyiContainer*, int shm_idx, void* (*translator)(void* address));
int      ayyi_container_find            (AyyiContainer*, void* content, void* (*translator)(void* address));
int      ayyi_container_get_last        (AyyiContainer*);
int      ayyi_container_count_items     (AyyiContainer*, void* (*translator)(void* address));
gboolean ayyi_container_index_ok        (AyyiContainer*, int shm_idx);
gboolean ayyi_container_index_is_used   (AyyiContainer*, int shm_idx);
void     ayyi_container_print           (AyyiContainer*);

void     ayyi_shm_init                  ();
shm_seg* ayyi_shm_seg_new               (int id, int type);
gboolean ayyi_shm_import                ();
void     ayyi_shm_unattach              ();
void     ayyi_client__reattach_shm      (Service*, AyyiHandler2, gpointer);

gboolean shm_seg__attach                (AyyiShmSeg*);
gboolean shm_seg__validate              ();

#endif // __ayyi_shm_h__
