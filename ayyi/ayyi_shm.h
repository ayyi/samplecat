#ifndef __ayyi_shm_h__
#define __ayyi_shm_h__

#define AYYI_SHM_BLOCK_SIZE 4096

enum {
	SEG_TYPE_NOT_SET,
	SEG_TYPE_SONG,
	SEG_TYPE_MIXER,
	SEG_TYPE_PLUGIN, //TODO
	SEG_TYPE_LAST
};

struct _shm_seg
{
	int      id;
	int      type;
	int      size; //bytes
	gboolean attached;
	void*    address;
};

extern gboolean (*on_shm_callback)      (struct _shm_seg*);

gboolean ayyi_client_container_verify   (AyyiContainer*, void* (*translator)(void* address));
void*    ayyi_container_next_item       (AyyiContainer*, void* prev, void* (*translator)(void* address));
//void*    ayyi_container_prev_item       (AyyiContainer*, void* old);
void*    ayyi_container_get_item        (AyyiContainer*, int shm_idx, void* (*translator)(void* address));
int      ayyi_container_find            (AyyiContainer*, void* content, void* (*translator)(void* address));
int      ayyi_container_get_last        (AyyiContainer*);
int      ayyi_container_count_items     (AyyiContainer*, void* (*translator)(void* address));
gboolean ayyi_container_index_ok        (AyyiContainer*, int shm_idx);
gboolean ayyi_container_index_is_used   (AyyiContainer*, int shm_idx);
void     ayyi_container_print           (AyyiContainer*);

void     ayyi_shm_init                  ();
shm_seg* shm_seg_new                    (int id, int type);
gboolean ayyi_shm_import                ();

#endif // __ayyi_shm_h__
