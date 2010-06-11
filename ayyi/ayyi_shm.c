#define __ayyi_shm_c__
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <glib.h>
#include <jack/jack.h>

#include <ayyi/ayyi_typedefs.h>
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_msg.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_client.h>
#include <ayyi/ayyi_dbus.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/ayyi_time.h>
#include <ayyi/ayyi_seg0.h>
#include <ayyi/ayyi_log.h>
#include <ayyi/interface.h>

int shm_page_size = 0;
char nodes_dir[32] = "/tmp/ayyi/nodes/";
char tmp_dir[32] = "/tmp/ayyi/";

extern void* ayyi_song__translate_address(void* address);
void*    translate_mixer_address(void*);
gboolean is_song_shm_local_quiet(void* p); //tmp!!
shm_seg* ayyi_song__get_info    ();

static gboolean is_shm_seg_local(void*);
static gboolean ayyi_shm_is_complete();
static gboolean engine_on_shm(shm_seg*);

#define GET_TRANSLATOR \
	gboolean is_song = is_song_shm_local_quiet(container); \
	void* (*translator)(void* address); \
	if(is_song) translator = ayyi_song__translate_address; \
	else        translator = translate_mixer_address; \

//char seg_strs[3][16] = {"", "song", "mixer"};


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

	int b; for(b=0;b<CONTAINER_SIZE;b++){
		if(!container->block[b]) break;
		struct block* blk = translator(container->block[b]);
		void** table = blk->slot;
		if(blk->last < -1 || blk->last >= BLOCK_SIZE) { gwarn("block %i: last out of range: %i", b, blk->last); return FALSE; }
		if(blk->full <  0 || blk->full >  1         ) { gwarn("block %i: full out of range: %i", b, blk->full); return FALSE; }
		int i; for(i=0;i<BLOCK_SIZE;i++){
			if(!table[i]) continue;
			//dbg(0, "b=%i i=%i", b, i);
			AyyiRegion* item = translator(table[i]);
			if(!item) continue;
			if(is_song){ if(!is_song_shm_local_quiet(item)){ gwarn("slot=%i: bad song address: %p", i, item); continue; }}
			else if(!is_mixer_shm_local(item)){ gwarn("slot=%i: bad mixer address.", i); continue; }
			if(item->shm_idx != b * BLOCK_SIZE + i){ gwarn("bad idx! %i.%i container='%s'", b, i, c->signature); return FALSE; }
		}
	}

	return TRUE;
}


void*
ayyi_container_next_item(AyyiContainer* container, AyyiItem* prev, void* (*translator)(void* address))
{
	//used to iterate through the shm data. Returns the item following the one given, or NULL.
	//-if arg is NULL, we return the first region.

	if(container->last < 0) return NULL;

	//if(prev && (ayyi_container_find(container, prev, translator) >= BLOCK_SIZE)) gwarn ("FIXME");

	//gboolean is_song = is_song_shm_local_quiet(container);

	int not_first = 0;
//	dbg(0, "p=%i", prev ? prev->shm_idx : 0);
	int first_block = prev ? prev->shm_idx / BLOCK_SIZE : 0;

	int b; for(b=first_block;b<=container->last;b++){

		struct block* blk = translator(container->block[b]);
		void** table = blk->slot;

		int i = (not_first++ || !prev) ? 0 : (ayyi_container_find(container, prev, translator) % BLOCK_SIZE + 1);
		if(i < 0) { gwarn("cannot find starting point"); return NULL; }
//dbg(0, "start=%i.%i", b, i);
		for(;i<=blk->last;i++){
			dbg (2, " b=%i i=%i last=%i", b, i, blk->last);
			AyyiRegion* next = translator(table[i]);
			if(!next) continue;
			if(!is_shm_seg_local(next)){ gerr ("bad shm pointer. table[%u]=%p\n", i, table[i]); return NULL; }
//			dbg(0, "found %i", next->shm_idx);
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
ayyi_container_get_item(AyyiContainer* container, AyyiIdx shm_idx, void* (*translator)(void* address))
{
	if(shm_idx >= CONTAINER_SIZE * BLOCK_SIZE){ gerr("shm_idx out of range: %i", shm_idx); return NULL; }

	int b = shm_idx / BLOCK_SIZE;
	int i = shm_idx % BLOCK_SIZE;
	//struct block* blk = ayyi_song_translate_address(container->block[b]);
	struct block* blk = translator(container->block[b]);
	void** table = blk->slot;
	//void* item = ayyi_song_translate_address(table[i]);
	void* item = table[i] ? translator(table[i]) : NULL;
	if(!item && ayyi.debug) gwarn("no such item. idx=0x%x container=%s", shm_idx, container->signature);
	return item;
}


void*
ayyi_container_get_item_quiet(AyyiContainer* container, int shm_idx, void* (*translator)(void* address))
{
	if(shm_idx >= CONTAINER_SIZE * BLOCK_SIZE){ gerr("shm_idx out of range: %i", shm_idx); return NULL; }

	int b = shm_idx / BLOCK_SIZE;
	int i = shm_idx % BLOCK_SIZE;
	struct block* blk = translator(container->block[b]);
	void** table = blk->slot;
	void* item = table[i] ? translator(table[i]) : NULL;
	return item;
}


int
ayyi_container_find(AyyiContainer* container, void* content, void* (*translator)(void* address))
{
	//return the index number for the given struct.

	g_return_val_if_fail(container, -1);
	g_return_val_if_fail(content, -1);

	if(container->last < 0) return -1;

	int b; for(b=0;b<=container->last;b++){
		//struct block* block = ayyi_song_translate_address(container->block[b]);
		struct block* block = translator(container->block[b]);

		if(block->last < 0) break;

		void** table = block->slot;
		int i; for(i=0;i<=block->last;i++){
			void* item = ayyi_song__translate_address(table[i]);
			if(item == content) return b * BLOCK_SIZE + i;
		}
	}
	dbg (0, "content not found. content=%p", content);
	return -1;
}


int
ayyi_container_get_last(AyyiContainer* container)
{
	struct block* block = ayyi_song__translate_address(container->block[0]);
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
	unsigned segment_end = (unsigned)ayyi.service->song + seg_info->size;
	if(((unsigned int)p>(unsigned int)ayyi.service->song) && ((unsigned int)p<segment_end)) return TRUE;

	gerr ("address is not in song shm: %p (expected %p - %p)", p, ayyi.service->song, (void*)segment_end);
	if((unsigned)p > (unsigned)ayyi.service->song) gerr ("shm offset=%u", (unsigned)p - (unsigned)ayyi.service->song);
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

	ayyi.priv->on_shm = engine_on_shm;
}


shm_seg*
ayyi_shm_seg_new(int id, int type)
{
	if(type >= AYYI_SEG_TYPE_LAST){ gerr ("bad segment type. %i", type); return NULL; }

	shm_seg* shm_seg = g_new0(struct _shm_seg, 1);
	shm_seg->id      = id;
	shm_seg->type    = type;
	shm_seg->size    = 4096 * AYYI_SHM_BLOCK_SIZE; //FIXME
	ayyi.service->segs = g_list_append(ayyi.service->segs, shm_seg);
	return shm_seg;
}


gboolean
ayyi_shm_import()
{
	//attach all available shm segments.

	gboolean good = TRUE;

	GList* l = ayyi.service->segs;
	for(;l;l=l->next){
		shm_seg* shm_seg = l->data;
		if(shm_seg->id){
			if(shm_seg->attached) continue;

			void* shmptr;
			if((shmptr = shmat(shm_seg->id, 0, 0)) == (void *)-1){
				gerr ("shmat() %s", fail);
				return FALSE;
			}

			log_print(LOG_OK, "ayyi data import: %s", seg_strs[shm_seg->type]);
			shm_seg->attached = TRUE;
			shm_seg->address = shmptr;

			switch(shm_seg->type){
				case SEG_TYPE_NOT_SET:
					gerr ("unexpected shm segment type %i", shm_seg->type);
					good = FALSE;
					break;
				case SEG_TYPE_SONG:
					ayyi.service->song = shmptr;
					if(ayyi.priv->on_shm){
						if(!(good = ayyi.priv->on_shm(shm_seg))){
							log_print(LOG_FAIL, "imported song data invalid.");
							ayyi.service->song = NULL;
						}
					}
					break;
				case SEG_TYPE_MIXER:
					ayyi.service->amixer = shmptr;
					if(ayyi.priv->on_shm){
						if(!(good = ayyi.priv->on_shm(shm_seg))){
							log_print(LOG_FAIL, "imported mixer data invalid.");
							ayyi.service->amixer = NULL;
						}
					}
					break;
				default:
					//gerr ("unexpected shm segment type %i", shm_seg->type);
					break;
			}

			if(good && ayyi_shm_is_complete()){
				ayyi.got_shm = TRUE;
			}
		}// else printf("%s(): segment has no id!\n", __func__);
	}

	return good;
}


#include "ayyi/ayyi_song.h"  //tmp. make the verify callback part of the service.
#include "ayyi/ayyi_mixer.h"
static gboolean
engine_on_shm(shm_seg* seg)
{
	gboolean ok = FALSE;

	shm_virtual* shmv = seg->address;
	if(!strlen(shmv->service_name)) ok = FALSE;
	if(!shmv->owner_shm){ gerr("bad shm. owner_shm not set."); ok = FALSE; }

	switch(seg->type){
		case SEG_TYPE_SONG:
			ok = ayyi_song__verify(seg);
			break;
		case SEG_TYPE_MIXER:
			ok = ayyi_mixer__verify(/*seg*/);
			break;
		default:
			gwarn("!!");
			break;
	}
	return ok;
}


static gboolean
ayyi_shm_is_complete()
{
	GList* l = ayyi.service->segs;
	for(;l;l=l->next){
		shm_seg* shm_seg = l->data;
		if(!shm_seg->attached || shm_seg->invalid){ dbg(2, "not complete."); return FALSE; }
	}
	return TRUE;
}


gboolean
shm_seg__attach(AyyiShmSeg* seg)
{
	char proj = 'D'; //FIXME

	if(!g_file_test(tmp_dir, G_FILE_TEST_IS_DIR)){
		if(mkdir(tmp_dir, O_RDWR) == -1){
			gerr("cannot create tmp dir '%s'", tmp_dir);
			return FALSE;
		}
	}

	if(!g_file_test(nodes_dir, G_FILE_TEST_IS_DIR)){
		if(mkdir(nodes_dir, O_RDWR) == -1){
			gerr("cannot create tmp dir '%s'", nodes_dir);
			return FALSE;
		}
	}

	char node_name[256];
	snprintf(node_name, 255, "%s%s-%d", nodes_dir, seg->name, (int)getpid());
	if(!g_file_test(node_name, G_FILE_TEST_EXISTS)) creat(node_name, O_RDWR);
	if((seg->_shmkey = ftok(node_name, proj)) == -1){
		gerr("ftok failed: node_name=%s %s", node_name, fail);
		return FALSE;
	}

	if((seg->_shmfd = shmget(seg->_shmkey, seg->size, IPC_CREAT|0400)) == -1){
		gerr("shmget %s", fail);
		return FALSE;
	}
	//printf("%s(): _shmfd=%i\n", __func__, _shmfd);

	if((seg->address = (char*)shmat(seg->_shmfd, 0, 0)) == (char*)-1){
		/*if (debug > -1)*/ gerr("shmat [FAIL]");
		return FALSE;
	}

	//for (i=0;i<num_pages;i++) memset(_addr + i*DAE_SHM_PAGE_SIZE, 0, DAE_SHM_PAGE_SIZE);

	//strcpy(name, "song"); //FIXME hack so that malloc can cast correctly. It acts as a seg "type" field. 

	return TRUE;
}


gboolean
shm_seg__validate()
{
	return TRUE;
}


