#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <glib.h>
#include <jack/jack.h>

typedef void             action;

#include <ayyi/ayyi_typedefs.h>
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/ayyi_list.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_client.h>
#include <ayyi/interface.h>
#include <ayyi/ayyi_song.h>

#define translate(A) ayyi_song_translate_address(A)

extern struct _ayyi_client ayyi;

gboolean is_song_shm_local_quiet(void*);

static struct block* route_get_block(int block_num);
#ifdef NEVER
static block* region_get_block(int block_num);
#endif
static gboolean ayyi_song__container_index_ok(AyyiContainer*, int shm_idx);


void*
ayyi_song_translate_address(void* address)
{
	//pointers in shm have to be translated to the address range of the imported segment.

	if(!address) return NULL;

	void* translated = (void*)((uint32_t)address + (uint32_t)ayyi.song - (uint32_t)ayyi.song->owner_shm);

	//dbg(0, "owner %p -> local %p", address, translated);
	return (void*)translated;
}


void
ayyi_song__print_automation_list(AyyiChannel* track)
{
	struct _ayyi_list* automation_list = track->automation_list;
	printf("-------------------------------------------\n");
	dbg(0, "track=%s", track->name);
	//dbg(0, "list=%p %p", automation_list, ayyi_song_translate_address(automation_list));
	dbg(0, "n_curves=%i", ayyi_list__length(automation_list));
	struct _ayyi_list* l = translate(automation_list);
	for(;l;l=translate(l->next)){
		//AyyiContainer* param = l->data;
		dbg(0, "param='%s' id=%i next=%p", l->name, l->id, translate(l->next));
	}
	printf("-------------------------------------------\n");
}


int
ayyi_song__track_lookup_by_id(uint64_t id)
{
	//fn is silent. caller must check for -1 error.

	route_shared* track = NULL;
	while((track = ayyi_song__audio_track_next(track))){
        dbg (3, "id=%Lu", track->id);
        if(track->id == id) return track->shm_idx;
	}
	while((track = ayyi_song__midi_track_next(track))){
        if(track->id == id) return track->shm_idx;
	}
	dbg (2, "not found. id=%Lu", id);
	return -1;
}


shm_seg*
ayyi_song__get_info()
{
	GList* list = ayyi.segs;
	for(;list;list=list->next){
		shm_seg* seg = list->data;
		if(seg->type == 1) return seg;
	}
	return NULL;
}


gboolean
ayyi_song__is_shm(void* p)
{
	//returns true if the pointer is in our imported song segment.

	void* a = translate(p);

	shm_seg* seg_info = ayyi_song__get_info();
	unsigned segment_end = (unsigned)ayyi.song + seg_info->size;
	if(((unsigned int)a>(unsigned int)ayyi.song) && ((unsigned int)a<segment_end)) return TRUE;

	gerr ("address is not in song shm: %p (orig=%p) (expected %p - %p)\n", a, p, ayyi.song, (void*)segment_end);
	if((unsigned)p > (unsigned)ayyi.song) gerr ("shm offset=%u translated_offset=%u\n", (unsigned)p - (unsigned)ayyi.song, (unsigned)a - (unsigned)ayyi.song);
	return FALSE;
}


gboolean
is_song_shm_local(void* p)
{
	//returns true if the pointer is in the local shm segment.
	//returns false if the pointer is in a foreign shm segment.

	//if(((unsigned int)p>(unsigned int)ayyi.song) && ((unsigned int)p<segment_end)) return TRUE;
	if(is_song_shm_local_quiet(p)) return TRUE;

	shm_seg* seg_info = ayyi_song__get_info();
	unsigned segment_end = (unsigned)ayyi.song + seg_info->size;
	gerr ("address is not in song shm: %p (expected %p - %p)\n", p, ayyi.song, (void*)segment_end);
	if((unsigned)p > (unsigned)ayyi.song) gerr ("shm offset=%u\n", (unsigned)p - (unsigned)ayyi.song);
	return FALSE;
}


gboolean
is_song_shm_local_quiet(void* p)
{
	//returns true if the pointer is in the local shm segment.
	//returns false if the pointer is in a foreign shm segment.

	shm_seg* seg_info = ayyi_song__get_info();
	unsigned segment_end = (unsigned)ayyi.song + seg_info->size;
	if(((unsigned int)p>(unsigned int)ayyi.song) && ((unsigned int)p<segment_end)) return TRUE;

	return FALSE;
}


GList*
ayyi_song__get_audio_part_list()
{
	/*
		gets ardour regions from the shared_regions struct in shm.

		parts that are 'not being used' (are not in any playlists), are ignored.
	*/

	GList* part_list = NULL;
	region_shared* region = NULL;
	while((region = ayyi_song__audio_region_next(region))){
		if(!strlen(region->playlist_name)) continue; //ignore non-playlist regions.
		part_list = g_list_append(part_list, GINT_TO_POINTER(region->shm_idx));
	}

	dbg (0, "num audio parts found: %i", g_list_length(part_list));
	//if(debug > 1) ardour_print_part_list();
	return part_list;
}


void*
ayyi_song_container_get_item(AyyiContainer* container, int idx)
{
	return ayyi_container_get_item(container, idx, ayyi_song_translate_address);
}


void*
ayyi_song_container_next_item(AyyiContainer* container, void* prev)
{
	return ayyi_container_next_item(container, prev, ayyi_song_translate_address);
}


void*
ayyi_song_container_prev_item(AyyiContainer* container, void* old)
{
	//used to iterate backwards through the shm data. Returns the item before the one given, or NULL.
	//-if arg is NULL, we return the last region.

	if(container->last < 0) return NULL;

	if(old && (ayyi_song_container_find(container, old) >= BLOCK_SIZE)) gwarn ("FIXME");

	int not_first_block = 0;

	int b; for(b=container->last;b>=0;b--){
		struct block* blk = ayyi_song_translate_address(container->block[b]);
		void** table = blk->slot;

		if(!old){
			//return end item
			dbg (2, "first! region=%p", ayyi_song_translate_address(table[blk->last]));
			return ayyi_song_translate_address(table[blk->last]);
		}

		int i = (not_first_block++) ? blk->last : ayyi_song_container_find(container, old) - 1;
		if(i == -2) { gwarn("cannot find orig item in container."); return NULL; }
		if(i < 0) return NULL; //no more items.
		for(;i>=0;i--){
			dbg (0, "b=%i i=%i", b, i);
			void* next = ayyi_song_translate_address(table[i]);
			if(!next) continue; //unused slot.
			if(!is_song_shm_local(next)){ gerr ("bad shm pointer. table[%u]=%p\n", i, table[i]); return NULL; }
			return next;
		}
	}
	return NULL;
}


int
ayyi_song_container_count_items(AyyiContainer* container)
{
	//return the number of items that are allocated in the given shm container.

	int count = 0;
	if(container->last < 0) return count;

	void* (*translator)(void* address) = ayyi_song_translate_address;

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


int
ayyi_song_container_find(AyyiContainer* container, void* content)
{
	return ayyi_container_find(container, content, ayyi_song_translate_address);
}


gboolean
ayyi_song_container_verify(AyyiContainer* container)
{
	return ayyi_client_container_verify(container, ayyi_song_translate_address);
}


gboolean
ayyi_track_pod_index_ok(int pod_index)
{
	block* routes = route_get_block(0);

	return (pod_index > -1 && pod_index <= routes->last);
}


gboolean
ayyi_region_pod_index_ok(int pod_index)
{
	//check that the given region slot number is valid.

	return ayyi_song__container_index_ok(&ayyi.song->regions, pod_index);
/*
	if(pod_index >= BLOCK_SIZE) return FALSE;

	struct block* block = ardour_region_msg_idx_get_block((unsigned)pod_index, &s1, &s2);
	return (pod_index > -1 && pod_index <= block->last);
*/
}


static gboolean
ayyi_song__container_index_ok(AyyiContainer* container, int shm_idx)
{
	int b = shm_idx / BLOCK_SIZE;
	int i = shm_idx % BLOCK_SIZE;
	struct block* block = translate(container->block[b]);
	return (shm_idx > -1 && i <= block->last);
}


static struct block*
route_get_block(int block_num)
{
	return translate(ayyi.song->audio_tracks.block[block_num]);
}


#ifdef NEVER
static block*
region_get_block(int block_num)
{
	return ayyi_song_translate_address(ayyi.song->regions.block[block_num]);
}
#endif


#if 0
int
ayyi_part_get_track_num(region_base_shared* region)
{
	g_return_val_if_fail(region, -1);
	if(!strlen(region->playlist_name)){ gwarn ("unable to determine track. part has no playlist! (%s)\n", region->name); return 0; }

	//FIXME this calls a fn from a higher layer
	int ayyi_song__get_tracknum_from_playlist(char* playlist_name); //temp declaration
	int t = ayyi_song__get_tracknum_from_playlist(region->playlist_name);

	return t;
}
#endif


route_shared*
ayyi_track_next_armed(route_shared* prev)
{
	AyyiContainer* container = &ayyi.song->audio_tracks;
	if(container->last < 0) return NULL;

	if(prev && (prev->shm_idx >= BLOCK_SIZE)) gwarn ("FIXME");

	int not_first = 0;

	int b; for(b=0;b<=container->last;b++){
		struct block* routes = translate(container->block[b]);
		void** route_table = routes->slot;

		if(!prev){
			//return first
			dbg (2, "first! route=%p", translate(route_table[0]));
			return translate(route_table[0]);
		}

		int i = (not_first++) ? 0 : prev->shm_idx + 1;
		for(;i<=routes->last;i++){
			dbg (2, "b=%i i=%i", b, i);
			route_shared* next = ayyi_song_translate_address(route_table[i]);
			if(!next) continue;
			if(!is_song_shm_local(next)){ gerr ("bad shm pointer. route_table[%u]=%p\n", i, route_table[i]); return NULL; }
			if(!next->armed) continue;
			return next;
		}
	}
	return NULL;
}


#if 0
static void*
ayyi_song_container_get_block(AyyiContainer* container, int block_num)
{
    return ayyi_song_translate_address(container->block[block_num]);
}
#endif


gboolean
ayyi_file_get_other_channel(filesource_shared* filesource, char* other)
{
	//return the filename of the other half of a stereo pair.

	strncpy(other, filesource->name, 127);

	gchar* p = g_strrstr(other, "%L.");
	if(p){
		*(p+1) = 'R';
		dbg (3, "pair=%s", other);
		return TRUE;
	}

	p = g_strrstr(other, "%R.");
	if(p){
		*(p+1) = 'L';
		return TRUE;
	}

	p = g_strrstr(other, "-L.");
	if(p){
		*(p+1) = 'R';
		return TRUE;
	}

	p = g_strrstr(other, "-R.");
	if(p){
		*(p+1) = 'L';
		return TRUE;
	}

    other[0] = '\0';
	return FALSE;
}


void
ayyi_song__pool_print()
{
	//list all the pool items in shm. See also pool_print() for the gui data.

	UNDERLINE;
	filesource_shared* file = NULL;
	while((file = ayyi_song__filesource_next(file))){
		printf("%4i %12Lu %6i %s\n", file->shm_idx, file->id, file->length, file->name);
	}
	UNDERLINE;
}


