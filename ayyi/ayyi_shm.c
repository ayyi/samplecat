#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <glib.h>
#include <jack/jack.h>

typedef struct _song_pos song_pos;
typedef struct _shm_seg  shm_seg;
typedef void             action;
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_msg.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_client.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/ayyi_time.h>
#include <ayyi/ayyi_seg0.h>
#include <ayyi/interface.h>

int shm_page_size = 0;

gboolean (*on_shm_callback)     (struct _shm_seg*) = NULL; //application callback

void*    ayyi_song_translate_address(void* address);
void*    translate_mixer_address(void* address); //tmp!!
gboolean is_song_shm_local_quiet(void* p); //tmp!!
shm_seg* ayyi_song__get_info    ();

static gboolean is_shm_seg_local(void*);

#define GET_TRANSLATOR \
	gboolean is_song = is_song_shm_local_quiet(container); \
	void* (*translator)(void* address); \
	if(is_song) translator = ayyi_song_translate_address; \
	else        translator = translate_mixer_address; \

char seg_strs[3][16] = {"", "song", "mixer"};


gboolean       is_mixer_shm_local(void*);
gboolean
ayyi_client_container_verify(AyyiContainer* c, void* (*translator)(void* address))
{
	ASSERT_POINTER_FALSE(c, "container");
	AyyiContainer* container = c;

	if(!is_song_shm_local_quiet(c) && !is_mixer_shm_local(c)){ gerr("bad address: %p", c); return FALSE; }

	if(c->last < -1 || c->last >= CONTAINER_SIZE) { gwarn("last out of range: %i", c->last); return FALSE; }
	if(c->full <  0 || c->full >  1             ) { gwarn("full out of range: %i", c->full); return FALSE; }
	if(!c->signature[0]) { gwarn("no signature."); return FALSE; }
	if(c->last == -1) { gwarn("'%s': no blocks allocated.", c->signature); return TRUE; }

	gboolean is_song = is_song_shm_local_quiet(container);
	//look at start of first block:
	int b = 0;
	struct block* blk = translator(container->block[b]);
	void** table = blk->slot;
	if(blk->last < -1 || blk->last >= BLOCK_SIZE) { gwarn("block %i: last out of range: %i", b, blk->last); return FALSE; }
	if(blk->full <  0 || blk->full >  1         ) { gwarn("block %i: full out of range: %i", b, blk->full); return FALSE; }
	int i; for(i=0;i<10;i++){
		if(!table[i]) continue;
		//dbg(0, "b=%i i=%i", b, i);
		void* item = translator(table[i]);
		if(!item) continue;
		if(is_song){ if(!is_song_shm_local_quiet(item)){ gwarn("slot=%i: bad song address: %p", i, item); continue; }}
		else if(!is_mixer_shm_local(item)){ gwarn("slot=%i: bad mixer address.", i); continue; }
	}

	return TRUE;
}


void*
ayyi_container_next_item(AyyiContainer* container, void* prev, void* (*translator)(void* address))
{
	//used to iterate through the shm data. Returns the item following the one given, or NULL.
	//-if arg is NULL, we return the first region.

	if(container->last < 0) return NULL;

	if(prev && (ayyi_container_find(container, prev, translator) >= BLOCK_SIZE)) gwarn ("FIXME");

	//gboolean is_song = is_song_shm_local_quiet(container);

	int not_first = 0;

	int b; for(b=0;b<=container->last;b++){
		struct block* blk = translator(container->block[b]);
		void** table = blk->slot;

		int i = (not_first++ || !prev) ? 0 : ayyi_container_find(container, prev, translator) + 1;
		if(i < 0) { gwarn("cannot find starting point"); return NULL; }
		for(;i<=blk->last;i++){
			dbg (2, "b=%i i=%i", b, i);
			region_shared* next = translator(table[i]);
			if(!next) continue;
			if(!is_shm_seg_local(next)){ gerr ("bad shm pointer. table[%u]=%p\n", i, table[i]); return NULL; }
			return next;
		}
	}
	return NULL;
}


#if 0 //see ayyi_song.c
void*
ayyi_container_prev_item(AyyiContainer* container, void* old)
{
	//used to iterate backwards through the shm data. Returns the item before the one given, or NULL.
	//-if arg is NULL, we return the last region.

	if(container->last < 0) return NULL;

	if(old && (ayyi_container_find(container, old) >= BLOCK_SIZE)) gwarn ("FIXME");

	int not_first_block = 0;

	int b; for(b=container->last;b>=0;b--){
		struct block* blk = ayyi_song_translate_address(container->block[b]);
		void** table = blk->slot;

		if(!old){
			//return end item
			dbg (2, "first! region=%p", ayyi_song_translate_address(table[blk->last]));
			return ayyi_song_translate_address(table[blk->last]);
		}

		int i = (not_first_block++) ? blk->last : ayyi_container_find(container, old) - 1;
		if(i == -2) { gwarn("cannot find orig item in container."); return NULL; }
		if(i < 0) return NULL; //no more items.
		for(;i>=0;i--){
			dbg (0, "b=%i i=%i", b, i);
			void* next = ayyi_song_translate_address(table[i]);
			if(!next) continue; //unused slot.
			if(!is_shm_seg_local(next)){ gerr ("bad shm pointer. table[%u]=%p\n", i, table[i]); return NULL; }
			return next;
		}
	}
	return NULL;
}
#endif


void*
ayyi_container_get_item(AyyiContainer* container, int shm_idx, void* (*translator)(void* address))
{
	if(shm_idx >= CONTAINER_SIZE * BLOCK_SIZE){ gerr("shm_idx out of range: %i", shm_idx); return NULL; }

	int b = shm_idx / BLOCK_SIZE;
	int i = shm_idx % BLOCK_SIZE;
	//struct block* blk = ayyi_song_translate_address(container->block[b]);
	struct block* blk = translator(container->block[b]);
	void** table = blk->slot;
	//void* item = ayyi_song_translate_address(table[i]);
	void* item = table[i] ? translator(table[i]) : NULL;
	if(!item) gwarn("no such item. idx=0x%x container=%s", shm_idx, container->signature);
	return item;
}


int
ayyi_container_find(AyyiContainer* container, void* content, void* (*translator)(void* address))
{
	//return the index number for the given struct.

	ASSERT_POINTER_FALSE(container, "container");
	ASSERT_POINTER_FALSE(content, "content");

	if(container->last < 0) return -1;

	int b; for(b=0;b<=container->last;b++){
		//struct block* block = ayyi_song_translate_address(container->block[b]);
		struct block* block = translator(container->block[b]);

		//if(block->full){
		//	dbg (0, "block %i full.", b);
		//	count += BLOCK_SIZE;
		//}else{
			if(block->last < 0) break;

			void** table = block->slot;
			int i; for(i=0;i<=block->last;i++){
				void* item = ayyi_song_translate_address(table[i]);
				if(item == content) return b * BLOCK_SIZE + i;
			}
		//}
	}
	dbg (0, "content not found. content=%p", content);
	return -1;
}


int
ayyi_container_get_last(AyyiContainer* container)
{
	struct block* block = ayyi_song_translate_address(container->block[0]);
	return block->last;
}


int
ayyi_container_count_items(AyyiContainer* container, void* (*translator)(void* address))
{
	//return the number of items that are allocated in the given shm container.

	int count = 0;
	if(container->last < 0) return count;

	int b; for(b=0;b<=container->last;b++){
		//struct block* block = ayyi_song_translate_address(container->block[b]);
		struct block* block = translator(container->block[b]);
		if(block->full){
			gwarn ("block %i full.", b);
			count += BLOCK_SIZE;
		}else{
			if(block->last < 0) break;

			void** table = block->slot;
			int i; for(i=0;i<=block->last;i++){
				if(!table[i]) continue;
				//void* item = ayyi_song_translate_address(table[i]);
				void* item = translator(table[i]);
				if(item) count++;
			}
		}
	}
	dbg (2, "count=%i", count);
	return count;
}


gboolean
ayyi_container_index_ok(AyyiContainer* container, int shm_idx)
{
	//for song containers, use ayyi_song__container_index_ok() instead.
	GET_TRANSLATOR

	int b = shm_idx / BLOCK_SIZE;
	int i = shm_idx % BLOCK_SIZE;
	//struct block* block = ayyi_song_translate_address(container->block[b]);
	struct block* block = translator(container->block[b]);
	return (shm_idx > -1 && i <= block->last);
}


gboolean
ayyi_container_index_is_used(AyyiContainer* container, int shm_idx)
{
	GET_TRANSLATOR

	int b = shm_idx / BLOCK_SIZE;
	int i = shm_idx % BLOCK_SIZE;
	//struct block* block = ayyi_song_translate_address(container->block[b]);
	struct block* block = translator(container->block[b]);
	if (shm_idx < 0 || i > block->last) return FALSE;
	return (block->slot[i] != NULL);
}


static gboolean
is_shm_seg_local(void* p)
{
	//FIXME change this so it works with any shm segment.

	//returns true if the pointer is in the local shm segment.
	//returns false if the pointer is in a foreign shm segment.

	shm_seg* seg_info = ayyi_song__get_info();
	unsigned segment_end = (unsigned)ayyi.song + seg_info->size;
	if(((unsigned int)p>(unsigned int)ayyi.song) && ((unsigned int)p<segment_end)) return TRUE;

	gerr ("address is not in song shm: %p (expected %p - %p)", p, ayyi.song, (void*)segment_end);
	if((unsigned)p > (unsigned)ayyi.song) gerr ("shm offset=%u", (unsigned)p - (unsigned)ayyi.song);
	return FALSE;
}


void
ayyi_container_print(AyyiContainer* container)
{
	AyyiContainer* c = container;
	GET_TRANSLATOR
	printf("---------------------------------------\n");
	printf("outer: last=%i full=%i sig='%s'\n", c->last, c->full, c->signature);

	//look at the first block:
	int b = 0;
	struct block* blk = translator(container->block[b]);
	void** table = blk->slot;
	printf("block %i: last=%i full=%i\n", b, blk->last, blk->full);
	int i; for(i=0;i<10;i++){
		void* item = translator(table[i]);
		printf("  *%p\n", item);
	}
	printf("---------------------------------------\n");
}


void
ayyi_shm_init()
{
	shm_page_size = sysconf(_SC_PAGESIZE);
}


shm_seg*
shm_seg_new(int id, int type)
{
	if(type >= SEG_TYPE_LAST){ gerr ("bad segment type. %i", type); return NULL; }

	shm_seg* shm_seg = g_new0(struct _shm_seg, 1);
	shm_seg->id      = id;
	shm_seg->type    = type;
	shm_seg->size    = 4096 * AYYI_SHM_BLOCK_SIZE; //FIXME
	ayyi.segs = g_list_append(ayyi.segs, shm_seg);
	return shm_seg;
}


gboolean
ayyi_shm_import()
{
	//attach all available shm segments.

	gboolean good = TRUE;

	GList* list = ayyi.segs;
	for(;list;list=list->next){
		shm_seg* shm_seg = list->data;
		if(shm_seg->id){
			if(shm_seg->attached){ /*printf("%s(): seg already attached.\n", __func__);*/ continue; }

			void* shmptr;
			if((shmptr = shmat(shm_seg->id, 0, 0)) == (void *)-1){
				gerr ("shmat() %s", fail);
				return FALSE;
			}

			printf("ayyi data import: %s \n%s %s\n", seg_strs[shm_seg->type], go_rhs, ok);
			shm_seg->attached = TRUE;
			shm_seg->address = shmptr;

			switch(shm_seg->type){
				case 1:
					ayyi.song = shmptr;
					if(!on_shm_callback){ gerr("callback not set"); break; }
					if(!(good = on_shm_callback(shm_seg))){
#if 0
						log_printf(LOG_FAIL, "imported song data invalid.");
#endif
						gwarn("imported song data invalid.");
						ayyi.song = NULL;
					}
					break;
				case 2:
					ayyi.amixer = shmptr;
					if(!(good = on_shm_callback(shm_seg))){
#if 0
						log_printf(LOG_FAIL, "imported mixer data invalid.");
#endif
						gwarn("imported mixer data invalid.");
						ayyi.amixer = NULL;
					}
					break;
				default:
					//gerr ("unexpected shm segment type %i", shm_seg->type);
					break;
			}
		}// else printf("%s(): segment has no id!\n", __func__);
	}

	return good;
}


