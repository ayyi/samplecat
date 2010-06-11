#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <glib.h>
#include <jack/jack.h>
#ifdef USE_VST
#include "fst.h"
#endif

#include <ayyi/ayyi_typedefs.h>
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/ayyi_msg.h>
#include <ayyi/ayyi_client.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/interface.h>
#include <ayyi/ayyi_mixer.h>
#include <ayyi/ayyi_dbus.h>
#include <ayyi/engine_proxy.h>

extern struct _ayyi_client ayyi;

static int           ayyi_mixer__container_find(AyyiContainer*, void* content);
static struct block* ayyi_plugin_get_block(int block_num);
static struct block* ayyi_automation_get_block(AyyiChannel*, int block_num, int auto_type);
/*static*/ gboolean is_mixer_shm_local(void*);


static int
ayyi_mixer__container_find(AyyiContainer* container, void* content)
{
	//return the index number for the given struct.

	g_return_val_if_fail(container, FALSE);
	g_return_val_if_fail(content, FALSE);

	if(container->last < 0) return -1;

	int b; for(b=0;b<=container->last;b++){
		struct block* block = translate_mixer_address(container->block[b]);
		//if(block->full){
		//	dbg (0, "block %i full.", b);
		//	count += BLOCK_SIZE;
		//}else{
			if(block->last < 0) break;

			void** table = block->slot;
			int i; for(i=0;i<=block->last;i++){
				if(!table[i]) continue;
				void* item = translate_mixer_address(table[i]);
				if(item == content) return b * BLOCK_SIZE + i;
			}
		//}
	}
	dbg (0, "not found.");
	return -1;
}


gboolean
ayyi_mixer__verify()
{
	//g_return_val_if_fail(seg->type == SEG_TYPE_MIXER, false);

	//check channels:
	if(!ayyi_mixer_container_verify(&ayyi.service->amixer->tracks)){
		return FALSE;
	}

	if(!ayyi_client_container_verify(&ayyi.service->amixer->plugins, translate_mixer_address)){
		return FALSE;
	}

	return TRUE;
}


AyyiChannel*
ayyi_mixer_track_get(AyyiIdx slot)
{
	return ayyi_mixer_container_get_quiet(&ayyi.service->amixer->tracks, slot);
}


int
ayyi_mixer_track_count_plugins(AyyiChannel* ch)
{
	int count = 0;
	int i; for(i=0;i<AYYI_PLUGINS_PER_CHANNEL;i++){
		if(ch->insert_active[i]) count ++;
	}
	return count;
}


AyyiPlugin*
ayyi_plugin_get(int slot)
{
	//returns the shm data for a plugin with the given slot number.

	if(!ayyi_plugin_pod_index_ok(slot)){ gerr ("bad plugin index (%i)", slot); return NULL; }
	struct block* block = ayyi_plugin_get_block(0);
	if(block->last<0) dbg (0, "no plugins. last=%i", block->last);

	//printf("%s(): block=%p 0=%p\n", __func__, shm_index->regions.block, shm_index->regions.block[0]);

	void** plugin_index = block->slot;
	if(!is_mixer_shm_local(plugin_index)){ gerr ("bad song data. block_index=%p", plugin_index); return NULL; }
	//printf("%s(): slot=%i %p last=%i shm_index=%p local?=%1i\n", __func__, pod_index, region_index, shm_index->regions.last, shm_index, is_song_shm_local(region_index));
	AyyiPlugin* plugin = translate_mixer_address(plugin_index[slot]);
	if(!plugin) return NULL;
	if(!is_mixer_shm_local(plugin)){ gerr ("failed to get shared plugin struct. plugin_index[%u]=%p\n", slot, plugin_index[slot]); return NULL; }
	return plugin;
}


gboolean
ayyi_plugin_pod_index_ok(int pod_index)
{
	//check that the given plugin slot number is valid.
	//-we use this to ensure pointer dereferencing is ok, so dont return TRUE for any magic index numbers.

	if(pod_index >= BLOCK_SIZE) return FALSE;

	int block_num = pod_index / BLOCK_SIZE;
	int slot_num = pod_index % BLOCK_SIZE;
	struct block* block = ayyi_plugin_get_block(block_num);

	return (pod_index > -1 && slot_num <= block->last);
}


AyyiContainer*
ayyi_plugin_get_controls(int plugin_slot)
{
	AyyiPlugin* plugin = ayyi_plugin_get(plugin_slot);
	if(!plugin) return NULL;

	return &plugin->controls;
}


struct _ayyi_control*
ayyi_plugin_get_control(AyyiPlugin* plugin, int control_idx)
{
	struct _ayyi_control* control = ayyi_mixer_container_get_item(&plugin->controls, control_idx);
	dbg(0, "idx=%i --> control='%s'", control_idx, control ? control->name : "null");
	return control;
}


struct _ayyi_control*
ayyi_mixer__get_plugin_control(AyyiTrack* route, int i)
{
	#warning empty fn.
	return NULL;
}


struct _ayyi_control*
ayyi_mixer__plugin_control_next(AyyiContainer* plugin_container, struct _ayyi_control* control)
{
	return ayyi_mixer__container_next_item(plugin_container, control);
#if 0
	if(container->last < 0) return NULL;

	if(prev && (ayyi_container_find(container, prev) >= BLOCK_SIZE)) P_WARN ("FIXME\n");

	int not_first = 0;

	int b; for(b=0;b<=container->last;b++){
		struct block* blk = ayyi_client_translate_address(container->block[b]);
		void** table = blk->slot;

		if(!prev){
			//return first item
			dbg (2, "first! region=%p", ayyi_client_translate_address(table[0]));
			return ayyi_client_translate_address(table[0]);
		}

		int i = (not_first++) ? 0 : ayyi_container_find(container, prev) + 1;
		if(i < 0) { gwarn("cannot find starting point"); return NULL; }
		for(;i<=blk->last;i++){
			dbg (2, "b=%i i=%i", b, i);
			AyyiRegion* next = ayyi_client_translate_address(table[i]);
			if(!next) continue;
			if(!is_shm_seg_local(next)){ P_ERR ("bad shm pointer. table[%u]=%p\n", i, table[i]); return NULL; }
			return next;
		}
	}
	return NULL;
#endif
}


static struct block*
ayyi_plugin_get_block(int block_num)
{
    return translate_mixer_address(ayyi.service->amixer->plugins.block[block_num]);
}


void*
ayyi_mixer_container_get_item(AyyiContainer* container, int idx)
{
	if(idx >= CONTAINER_SIZE * BLOCK_SIZE){ gerr("shm_idx out of range: %i", idx); return NULL; }

	int b = idx / BLOCK_SIZE;
	int i = idx % BLOCK_SIZE;
	struct block* blk = translate_mixer_address(container->block[b]);
	void** table = blk->slot;
	void* item = translate_mixer_address(table[i]);
	if(!item) gwarn("empty item. idx=0x%x", idx);
	return item;
}


void*
ayyi_mixer_container_get_quiet(AyyiContainer* container, int idx)
{
	if(idx >= CONTAINER_SIZE * BLOCK_SIZE){ gerr("shm_idx out of range: %i", idx); return NULL; }

	int b = idx / BLOCK_SIZE;
	int i = idx % BLOCK_SIZE;
	struct block* blk = translate_mixer_address(container->block[b]);
	void** table = blk->slot;
	if(!table[i]) return NULL;
	void* item = translate_mixer_address(table[i]);
	return item;
}


void*
ayyi_mixer__container_next_item(AyyiContainer* container, void* prev)
{
	//used to iterate through the shm data. Returns the item following the one given, or NULL.
	//-if arg is NULL, the first region is returned.

	if(container->last < 0) return NULL;

	int prev_idx = prev ? ayyi_mixer__container_find(container, prev) : -1;
	if(prev && (prev_idx >= BLOCK_SIZE)) gwarn ("FIXME");

	int not_first = 0;

	int b; for(b=0;b<=container->last;b++){
		dbg(3, "b=%i: =%p", b, container->block[b]);
		struct block* blk = translate_mixer_address(container->block[b]);
		void** table = blk->slot;

		int i = (not_first++) ? 0 : prev_idx + 1;
		if(i < 0) { gwarn("cannot find starting point"); return NULL; }
		for(;i<=blk->last;i++){
			dbg (2, "b=%i i=%i", b, i);
			if(!table[i]) continue;
			AyyiRegion* next = translate_mixer_address(table[i]);
			if(!next) continue;
			if(!is_mixer_shm_local(next)){ gerr ("bad shm pointer. table[%u]=%p\n", i, table[i]); return NULL; }
			return next;
		}
	}
	return NULL;
}


int
ayyi_mixer_container_count_items(AyyiContainer* container)
{
	//return the number of items that are allocated in the given shm container.

	int count = 0;
	if(container->last < 0) return count;

	int b; for(b=0;b<=container->last;b++){
		struct block* block = translate_mixer_address(container->block[b]);
		//struct block* block = translator(container->block[b]);
		if(block->full){
			gwarn ("block %i full.", b);
			count += BLOCK_SIZE;
		}else{
			if(block->last < 0) break;

			void** table = block->slot;
			int i; for(i=0;i<=block->last;i++){
				if(!table[i]) continue;
				void* item = translate_mixer_address(table[i]);
				//void* item = translator(table[i]);
				if(item) count++;
			}
		}
	}
	dbg (2, "count=%i", count);
	return count;
}


gboolean
ayyi_mixer_container_verify(AyyiContainer* container)
{
	return ayyi_client_container_verify(container, translate_mixer_address);
}


struct _ayyi_aux*
ayyi_mixer__aux_get(struct _ayyi_channel* chan, int idx)
{
	ASSERT_POINTER_FALSE(chan, "channel");

	if(idx >= AYYI_AUX_PER_CHANNEL) return NULL;
	if(!chan->aux[idx]) return NULL;
	struct _ayyi_aux* aux = translate_mixer_address(chan->aux[idx]);
	if(!is_mixer_shm_local(aux)){ gerr ("bad shm pointer. chan=%i aux_idx=%i\n", chan->shm_idx, idx); return NULL; }
	return aux;
}


struct _ayyi_aux*
ayyi_mixer__aux_get_(int channel_idx, int idx)
{
	struct _ayyi_channel* chan = ayyi_mixer_track_get(channel_idx);
	if(!chan){ gwarn("no channel for channel_idx=%i", channel_idx); return NULL; }
	return ayyi_mixer__aux_get(chan, idx);
}


int
ayyi_mixer__aux_count(int channel_idx)
{
	struct _ayyi_channel* chan = ayyi_mixer_track_get(channel_idx);
	if(!chan){ gwarn("no channel for channel_idx=%i", channel_idx); return -1; }
	int count = 0;
	int i; for(i=0;i<AYYI_AUX_PER_CHANNEL;i++){
		if(chan->aux[i]) count++;
	}
	return count;
}


void
ayyi_channel__set_float(int ch_prop, int chan, double val)
{
	//sends a msg to the engine for the simple case where a strip has a property with a single float value.
	PF;
#if USE_DBUS
	dbus_set_prop_float (NULL, AYYI_OBJECT_CHAN, ch_prop, chan, val);
#endif
}


void
ayyi_channel__set_bool(int ch_prop, AyyiIdx chan, gboolean val)
{
	//sends a msg to the engine for the simple case where a strip has a property with a single float value.
#if USE_DBUS
	dbus_set_prop_bool (NULL, AYYI_OBJECT_CHAN, ch_prop, chan, val);
#endif
}


void
ayyi_mixer__block_print(AyyiContainer* container, int s1)
{
	ASSERT_POINTER(container, "container");

	struct block* block = translate_mixer_address(container->block[s1]);
	if (!is_mixer_shm_local(block)){ gerr ("bad block! not initialised?"); }

	printf("%s():\n", __func__);
	printf("  last=%i full=%i, next=%p\n", block->last, block->full, block->next);
	int i; for(i=0;i<4;i++) printf("  %i: %p\n", i, block->slot[i]);
}


/*static*/ gboolean
is_mixer_shm_local(void* p)
{
	//returns true if the pointer is in the local shm segment.
	//returns false if the pointer is in a foreign shm segment.

	//printf("%s(): size=%i\n", __func__, ayyi.service->amixer->num_pages);
	unsigned segment_end = (unsigned)ayyi.service->amixer + ayyi.service->amixer->num_pages * AYYI_SHM_BLOCK_SIZE; //TODO should use seg_info->size here?
	if(((unsigned)p>(unsigned)ayyi.service->amixer) && ((unsigned)p<segment_end)) return TRUE;

	gerr ("address is not in mixer shm: %p (expected %p - %p)\n", p, ayyi.service->amixer, (void*)segment_end);
	if((unsigned)p > (unsigned)ayyi.service->amixer) gerr ("shm offset=%u seg_size=%u\n", (unsigned)p - (unsigned)ayyi.service->amixer, 512*AYYI_SHM_BLOCK_SIZE);
	//else errprintf("is_song_shm(): address is below start of shm segment by %u\n", (unsigned int)shm.song - (unsigned int)p);
	return FALSE;
}


void* //temporarily used in ayyi_shm
translate_mixer_address(void* address)
{
	//pointers in shm have to be translated to the address range of the imported segment.

	//unfortunately we cannot make this static as it is used by all mixer shm data types, eg ayyi_list.

	ASSERT_POINTER_FALSE(address, "address");

	//is there any danger of overflow here? both of these 2 versions _appear_ to work:

	//void* translated = (void*)((uint32_t)address + (uint32_t)ayyi.service->amixer - (uint32_t)ayyi.service->amixer->owner_shm);

	void* distance_from_base = (void*)((uint32_t)address - (uint32_t)ayyi.service->amixer->owner_shm);
	void* translated = distance_from_base + (uint32_t)ayyi.service->amixer;

	//printf("%s(): ayyi.service->amixer=%p ayyi.service->amixer->owner_shm=%p diff=%x distance_from_base=%x\n", __func__, ayyi.service->amixer, ayyi.service->amixer->owner_shm, (uint32_t)ayyi.service->amixer - (uint32_t)ayyi.service->amixer->owner_shm, (uint32_t)distance_from_base);
	
	//if(!is_song_shm(translated)){ errprintf("sml_translate_address(): address is not in shm!\n"); return NULL; }
	//printf("%s(): owner %p -> local %p\n", __func__, address, (void*)translated);
	return (void*)translated;
}


static struct block*
ayyi_automation_get_block(AyyiChannel* ch, int block_num, int auto_type)
{
	if(auto_type > 1){ gerr ("auto_type out of range: %i", auto_type); return NULL; }
	if(block_num > ch->automation[auto_type].last) { gerr ("block_num out of range: %i (last=%i)", block_num, ch->automation[auto_type].last); return NULL; }

	static char sigs[2][32] = {"vol automation", "pan automation"};
	ASSERT_POINTER_FALSE(ch, "channel");
	if(strcmp(ch->automation[auto_type].signature, sigs[auto_type])){ gerr ("bad container signature: %s", ch->automation[auto_type].signature); return NULL; }

	dbg (3, "route=%p b=%i block=%p block[0]=%p last=%i", ch, block_num, ch->automation[auto_type].block, ch->automation[auto_type].block[0], ch->automation[auto_type].last);
    return translate_mixer_address(ch->automation[auto_type].block[block_num]);
}


/*
static block*
ardour_automation_get_block(AyyiChannel* ch, int block_num, int auto_type)
{
	if(auto_type > 1){ gerr ("auto_type out of range: %i", auto_type); return NULL; }
	if(block_num > ch->automation[auto_type].last) { P_ERR ("block_num out of range: %i (last=%i)\n", block_num, ch->automation[auto_type].last); return NULL; }

	static char sigs[2][32] = {"vol automation", "pan automation"};
	ASSERT_POINTER_NULL(ch, "channel");
	if(strcmp(ch->automation[auto_type].signature, sigs[auto_type])){ P_ERR ("bad container signature: %s\n", ch->automation[auto_type].signature); return NULL; }

	dbg (3, "route=%p b=%i block=%p block[0]=%p last=%i", ch, block_num, ch->automation[auto_type].block, ch->automation[auto_type].block[0], ch->automation[auto_type].last);
    return translate_mixer_address(ch->automation[auto_type].block[block_num]);
}
*/

shm_event*
ayyi_auto_get_event(AyyiChannel* ch, int i, int auto_type)
{
	struct block* block = ayyi_automation_get_block(ch, 0, auto_type);
	if(block->last < 0) dbg (0, "no events. last=%i", block->last);

	void** event_table = block->slot;
	if(!is_mixer_shm_local(event_table)){ gerr ("bad song data. block_table=%p", event_table); return NULL; }
	//printf("%s(): slot=%i %p last=%i ayyi.song=%p local?=%1i\n", __func__, pod_index, region_index, shm_index->regions.last, shm_index, is_song_shm_local(region_index));
	shm_event* event = translate_mixer_address(event_table[i]);
	if(!event) return NULL;
	if(!is_mixer_shm_local(event)){ gerr ("failed to get shared event struct. event_index[%u]=%p auto_type=%i", i, event_table[i], auto_type); return NULL; }
	return event;
}


void
ayyi_automation_print(AyyiChannel* route)
{
	PF;

	if(!route->automation[VOL].block[0]){ dbg (0, "track has no automation events."); return; }

	int count = 0;
	int b; for(b=0;b<CONTAINER_SIZE;b++){
		struct block* block = ayyi_automation_get_block(route, b, VOL);
		if(!block){ dbg (2, "end found (empty block). stopping..."); break; }

		void** auto_table = block->slot;
		int i;
		for(i=0;i<=block->last;i++){
			if(!auto_table[i]) continue;
			shm_event* shared = ayyi_auto_get_event(route, i, VOL);
			if(shared){
				printf("  %9.2f: %8.2f\n", shared->when, shared->value);
				count++;
			}
		}
	}

	dbg (0, "event_count=%i", count);
}


