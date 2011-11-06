#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <glib.h>
#include <jack/jack.h>

#include <ayyi/ayyi_typedefs.h>
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_time.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/ayyi_list.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_client.h>
#include <ayyi/interface.h>
#include <ayyi/ayyi_msg.h>
#include <ayyi/ayyi_action.h>
#include <ayyi/ayyi_log.h>
#include <ayyi/ayyi_song.h>
#include <ayyi/ayyi_mixer.h>

#define translate(A) ayyi_song__translate_address(A)

extern struct _ayyi_client ayyi;

gboolean is_song_shm_local_quiet(void*);

static block* route_get_block(int block_num);
static block* ayyi_filesource_get_block(int block_num);
static gboolean ayyi_song__container_index_ok(AyyiContainer*, AyyiIdx);


void*
ayyi_song__translate_address(void* address)
{
	//pointers in shm have to be translated to the address range of the imported segment.

	if(!address) return NULL;

	void* translated = (void*)((unsigned)address + (unsigned)ayyi.service->song - (unsigned)ayyi.service->song->owner_shm);
	//void* translated = (void*)((ptrdiff_t)address + (void*)ayyi.service->song - (void*)ayyi.service->song->owner_shm);
	void* translated2 = (void*)((uintptr_t)address + (uintptr_t)ayyi.service->song - (uintptr_t)ayyi.service->song->owner_shm);
	if(translated2 != translated) gwarn("pointer arithmetic error");

	//dbg(0, "owner %p -> local %p", address, translated);
	return translated2;
}


void
ayyi_song__print_automation_list(AyyiChannel* track)
{
	struct _ayyi_list* automation_list = track->automation_list;
	printf("-------------------------------------------\n");
	dbg(0, "track=%s", track->name);
	//dbg(0, "list=%p %p", automation_list, ayyi_song__translate_address(automation_list));
	dbg(0, "n_curves=%i", ayyi_list__length(automation_list));
	struct _ayyi_list* l = translate(automation_list);
	for(;l;l=translate(l->next)){
		//AyyiContainer* param = l->data;
		dbg(0, "param='%s' id=%i next=%p", l->name, l->id, translate(l->next));
	}
	printf("-------------------------------------------\n");
}


AyyiTrack*
ayyi_song__track_lookup_by_id(uint64_t id)
{
	//fn is silent. caller must check for NULL return on fail.

	AyyiItem* track = NULL;
	while((track = (AyyiItem*)ayyi_song__audio_track_next(track))){
		if(track->id == id) return (AyyiTrack*)track;
	}
	while((track = ayyi_song_container_next_item(&ayyi.service->song->audio_tracks, track))){
		if(track->id == id) return (AyyiTrack*)track;
	}
	dbg (2, "not found. id=%Lu", id);
	return NULL;
}


shm_seg*
ayyi_song__get_info()
{
	GList* l = ayyi.service->segs;
	for(;l;l=l->next){
		shm_seg* seg = l->data;
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
	unsigned segment_end = (unsigned)ayyi.service->song + seg_info->size;
	if(((uintptr_t)a > (uintptr_t)ayyi.service->song) && ((uintptr_t)a<segment_end)) return TRUE;

	gerr ("address is not in song shm: %p (orig=%p) (expected %p - %p)\n", a, p, ayyi.service->song, (void*)segment_end);
	if((uintptr_t)p > (uintptr_t)ayyi.service->song) gerr ("shm offset=%u translated_offset=%u\n", (uintptr_t)p - (uintptr_t)ayyi.service->song, (uintptr_t)a - (uintptr_t)ayyi.service->song);
	return FALSE;
}


gboolean
is_song_shm_local(void* p)
{
	//returns true if the pointer is in the local shm segment.
	//returns false if the pointer is in a foreign shm segment.

	if(is_song_shm_local_quiet(p)) return TRUE;

	if(p){
		shm_seg* seg_info = ayyi_song__get_info();
		unsigned segment_end = (unsigned)ayyi.service->song + seg_info->size;
		gerr ("address is not in song shm: %p (expected %p - %p)", p, ayyi.service->song, (void*)segment_end);
		if((unsigned)p > (unsigned)ayyi.service->song) gerr ("shm offset=%u", (unsigned)p - (unsigned)ayyi.service->song);
	}
	else gerr("address null.");
	return FALSE;
}


gboolean
is_song_shm_local_quiet(void* p)
{
	//returns true if the pointer is in the local shm segment.
	//returns false if the pointer is in a foreign shm segment.

	shm_seg* seg_info = ayyi_song__get_info();
	unsigned segment_end = (unsigned)ayyi.service->song + seg_info->size;
	if(((unsigned int)p>(unsigned int)ayyi.service->song) && ((unsigned int)p<segment_end)) return TRUE;

	return FALSE;
}


GList*
ayyi_song__get_audio_part_list()
{
	// returns a list of audio regions from the shared_regions struct in shm.
	//
	// parts that are 'not being used' (are not in any playlists), are ignored.
	//
	// the returned list must be free'd by the caller.

	GList* part_list = NULL;
	AyyiAudioRegion* region = NULL;
	while((region = ayyi_song__audio_region_next(region))){
		/*
		if(region->playlist < 0) continue;
		AyyiPlaylist* playlist = ayyi_song__playlist_at(region->playlist);
		if(!playlist) continue; //ignore non-playlist regions. Deleted parts also dont have a playlist.
		*/
		AyyiPlaylist* playlist;
		if((region->playlist < 0) ||
			!(playlist = ayyi_song__playlist_at(region->playlist))) continue; //ignore non-playlist regions. Deleted parts also dont have a playlist.

		part_list = g_list_append(part_list, region);
	}

	dbg (2, "num audio parts found: %i", g_list_length(part_list));
	return part_list;
}


GList*
ayyi_song__get_midi_part_list()
{
	GList* part_list = NULL;
	AyyiMidiRegion* part = NULL;
	int i = 0;
	while((part = ayyi_song__midi_region_next(part))){
		part_list = g_list_append(part_list, part);
		if(i++ > 100) { gerr("bailing..."); break; } 
	}
	dbg (2, "num parts found: %i", g_list_length(part_list));
	return part_list;
}


AyyiItem*
ayyi_song_container_get_item(AyyiContainer* container, int idx)
{
	return ayyi_container_get_item(container, idx, ayyi_song__translate_address);
}


AyyiItem*
ayyi_song_container_next_item(AyyiContainer* container, void* prev)
{
	return ayyi_container_next_item(container, prev, ayyi_song__translate_address);
}


void*
ayyi_song_container_prev_item(AyyiContainer* container, void* old)
{
	//used to iterate backwards through the shm data. Returns the item before the one given, or NULL.
	//-if arg is NULL, we return the last region.

	if(container->last < 0) return NULL;

	if(old && (ayyi_song_container_find(container, old) >= AYYI_BLOCK_SIZE)) gwarn ("FIXME");

	int not_first_block = 0;

	int b; for(b=container->last;b>=0;b--){
		block* blk = ayyi_song__translate_address(container->block[b]);
		void** table = blk->slot;

		if(!old){
			//return end item
			dbg (2, "first! region=%p", ayyi_song__translate_address(table[blk->last]));
			return ayyi_song__translate_address(table[blk->last]);
		}

		int i = (not_first_block++) ? blk->last : ayyi_song_container_find(container, old) - 1;
		if(i == -2) { gwarn("cannot find orig item in container."); return NULL; }
		if(i < 0) return NULL; //no more items.
		for(;i>=0;i--){
			dbg (0, "b=%i i=%i", b, i);
			void* next = ayyi_song__translate_address(table[i]);
			if(!next) continue; //unused slot.
			if(!is_song_shm_local(next)){ gerr ("bad shm pointer. table[%u]=%p", i, table[i]); return NULL; }
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

	void* (*translator)(void* address) = ayyi_song__translate_address;

	int b; for(b=0;b<=container->last;b++){
		block* block = translator(container->block[b]);
		if(block->full){
			count += AYYI_BLOCK_SIZE;
		}else{
			if(block->last < 0) break;

			void** table = block->slot;
			int i; for(i=0;i<=block->last;i++){
				if(!table[i]) continue;
				void* item = translator(table[i]);
				if(item) count++;
			}
		}
	}
	dbg (2, "count=%i (%s)", count, container->signature);
	return count;
}


int
ayyi_song_container_find(AyyiContainer* container, void* content)
{
	return ayyi_container_find(container, content, ayyi_song__translate_address);
}


gboolean
ayyi_song_container_verify(AyyiContainer* container)
{
	return ayyi_client_container_verify(container, ayyi_song__translate_address);
}


char*
ayyi_song_container_next_name(AyyiContainer* container, const char* basename)
{
	// find the next unused name for the given basename
	// eg, given "New Track", will return something like "New Track 01".
	//
	// -the returned string must be freed by the caller.

	char base[AYYI_NAME_MAX];
	strncpy(base, basename, AYYI_NAME_MAX);
	base[AYYI_NAME_MAX] = '\0';

	gboolean found = false;
	char name[AYYI_NAME_MAX];
	int i = 1; for(;i<256;i++){
		snprintf(name, AYYI_NAME_MAX - 1, "%s %02i", base, i);
		dbg(2, "trying: %s ...", name);

		found = (gboolean)ayyi_song_container_lookup_by_name(container, name);

		if(!found) break;
	}
	dbg(2, "found=%i", found);
	return found ? NULL : g_strdup(name);
}


AyyiItem*
ayyi_song_container_lookup_by_name(AyyiContainer* container, const char* name)
{
	//can be used to check that a name is unused, so not finding a region is not considered an error.

	AyyiItem* test = NULL;
	while((test = ayyi_song_container_next_item(container, test))){
		if(!strcmp(test->name, name)) return test;
	}

	return NULL;
}


void
ayyi_song_container_make_name_unique(AyyiContainer* container, char* source_name_unique, const char* source_name)
{
	char new_name[64], old_name[64];
	if(strlen(source_name)) strncpy(old_name, source_name, 63); else strcpy(old_name, "unnamed");
	strcpy(new_name, old_name); //default, in case lookup fails.
	while(ayyi_song_container_lookup_by_name(container, old_name)){
		string_increment_suffix(new_name, old_name, 64);
		dbg (2, "new_name=%s", new_name);
		if(strlen(new_name) > AYYI_NAME_MAX){ log_print(LOG_FAIL, __func__); return; }
		strcpy(old_name, new_name);
	}

	strcpy(source_name_unique, new_name);
	dbg (2, "name=%s --> %s", source_name, source_name_unique);
}


AyyiAction*
ayyi_song_container_delete_item(AyyiContainer* container, AyyiItem* item, AyyiHandler handler, gpointer user_data)
{
	g_return_val_if_fail(item, NULL);
	g_return_val_if_fail(container->obj_type, NULL);

	void ayyi_item__delete_done(AyyiAction* a)
	{
		AyyiIdent id = {a->obj_type, a->obj_idx.idx1};
		HandlerData* d = a->app_data;
		if(d && d->callback) d->callback(id, NULL, d->user_data);
	}

	AyyiAction* a   = ayyi_action_new("item delete %i \"%s\"", item->shm_idx, item->name);
	a->op           = AYYI_DEL;
	a->obj_type     = container->obj_type;
	a->obj_idx.idx1 = item->shm_idx;
	a->callback     = ayyi_item__delete_done;
	a->app_data     = ayyi_handler_data_new(handler, user_data);
	ayyi_action_execute(a);

	return a;
}


static gboolean
ayyi_song__container_index_ok(AyyiContainer* container, AyyiIdx shm_idx)
{
	//if you need this externally, use x__get_item.

	if(shm_idx < 0) return FALSE;
	int b = shm_idx / AYYI_BLOCK_SIZE;
	int i = shm_idx % AYYI_BLOCK_SIZE;
	block* block = translate(container->block[b]);
	if (b >= CONTAINER_SIZE) return FALSE;
	AyyiItem* item = ayyi_song_container_get_item(container, shm_idx);
	return ((item->flags | deleted) || (i <= block->last));
}


AyyiItem*
ayyi_song__get_item_by_ident(AyyiIdent id)
{
	switch(id.type){
		case AYYI_OBJECT_AUDIO_PART:
			return (AyyiItem*)ayyi_song__audio_region_at(id.idx);
			break;
		default:
			break;
	}
	return NULL;
}


gboolean
ayyi_region__index_ok(AyyiIdx pod_index)
{
	//check that the given region slot number is valid.

	return ayyi_song__container_index_ok(&ayyi.service->song->audio_regions, pod_index);
}


#if 0
AyyiAction*
ayyi_region__delete_async(AyyiAudioRegion* region, AyyiHandler handler, gpointer user_data)
{
	g_return_val_if_fail(region, NULL);
	g_return_val_if_fail(ayyi_region__index_ok(region->shm_idx), NULL);

	void ayyi_region__delete_done(AyyiAction* a)
	{
		AyyiIdent id = {a->obj_type, a->obj_idx.idx1};
		HandlerData* d = a->app_data;
		if(d && d->callback) d->callback(id, &a->ret.error, d->user_data);
	}

	AyyiAction* a   = ayyi_action_new("region delete %i \"%s\"", region->shm_idx, region->name);
	a->op           = AYYI_DEL;
	a->obj_type     = AYYI_OBJECT_AUDIO_PART;
	a->obj_idx.idx1 = region->shm_idx;
	a->callback     = ayyi_region__delete_done;
	a->app_data     = ayyi_handler_data_new(handler, user_data);
	ayyi_action_execute(a);

	return a;
}
#endif


void
ayyi_region__make_name_unique(char* source_name_unique, const char* source_name)
{
	ayyi_song_container_make_name_unique(&ayyi.service->song->audio_regions, source_name_unique, source_name);
#if 0
	char new_name[64], old_name[64];
	if(strlen(source_name)) strncpy(old_name, source_name, 63); else strcpy(old_name, "unnamed");
	strcpy(new_name, old_name); //default, in case lookup fails.
	while(ayyi_region_lookup_by_name(old_name)){
		string_increment_suffix(new_name, old_name, 64);
		dbg (2, "new_name=%s", new_name);
		if(strlen(new_name) > AYYI_NAME_MAX){ log_print(LOG_FAIL, __func__); return; }
		strcpy(old_name, new_name);
	}

	strcpy(source_name_unique, new_name);
	dbg (2, "name=%s --> %s", source_name, source_name_unique);
#endif
}


AyyiIdx
ayyi_region__get_pod_index(AyyiRegionBase* region)
{
	g_return_val_if_fail(region, 0);
	return region->shm_idx;
}


uint32_t
ayyi_region__get_start_offset(AyyiAudioRegion* region)
{
	if (!region) return 0;

	return region->start;
}


static block*
route_get_block(int block_num)
{
	return translate(ayyi.service->song->audio_tracks.block[block_num]);
}


AyyiAudioRegion*
ayyi_region__get_from_id(uint64_t id)
{
	AyyiAudioRegion* region = NULL;
	while((region = ayyi_song__audio_region_next(region))){
		if(region->id == id) return region;
	}
	gwarn("id not found (%Lu).", id);
	return NULL;
}


AyyiTrack*
ayyi_track__next_armed(AyyiTrack* prev)
{
	AyyiTrack* track = NULL;
	while((track = ayyi_song__audio_track_next(track))){
		if(track->flags & armed) return track;
	}

	gwarn("no armed tracks found.");
	return NULL;
}


AyyiConnection*
ayyi_track__get_output(AyyiTrack* ayyi_trk)
{
	AyyiList* routing = ayyi_list__first(ayyi_trk->output_routing);
	int idx = routing ? routing->id : 0;
	if(routing) dbg (2, "output_idx=%i name=%s", routing->id, ayyi_song__connection_at(idx) ? ((AyyiConnection*)ayyi_song__connection_at(idx))->name : "");
	return (AyyiConnection*)ayyi_song__connection_at(idx);
}


const char*
ayyi_track__get_output_name(AyyiTrack* ayyi_trk)
{
	AyyiConnection* c = ayyi_track__get_output(ayyi_trk);
	return c ? c->name : "";
}


gboolean
ayyi_track__index_ok(AyyiIdx pod_index)
{
	block* routes = route_get_block(0);

	return (pod_index > -1 && pod_index <= routes->last);
}


AyyiChannel*
ayyi_track__get_channel(AyyiTrack* at)
{
	return ayyi_mixer__channel_at(at->shm_idx);
}


void
ayyi_track__make_name_unique(char* unique, const char* name)
{
	ayyi_song_container_make_name_unique(&ayyi.service->song->audio_tracks, unique, name);
}


int
ayyi_song__get_track_count()
{
	int track_count = ayyi_song_container_count_items(&ayyi.service->song->audio_tracks)
	                + ayyi_song_container_count_items(&ayyi.service->song->midi_tracks);

	//engine_cache.track_count = track_count;

	return track_count;
}


AyyiTrack* ayyi_song__get_track_by_playlist(AyyiPlaylist* playlist)
{
	const char* playlist_name = playlist->name;
	g_return_val_if_fail(playlist_name, NULL);
	g_return_val_if_fail(strlen(playlist_name), NULL);

	gchar* dot = g_strrstr(playlist_name, ".");
	if(!dot){
		//dbg (1, "playlist_name has no dot. Assuming this is legal...");
		dot = (char*)&playlist_name[strlen(playlist_name)];
	}
	int str_len = (unsigned)dot - (unsigned)playlist_name;
	char track_name[AYYI_NAME_MAX];
	strncpy(track_name, playlist_name, str_len);
	track_name[str_len] = '\0';

	AyyiTrack* track = NULL;
	while((track = ayyi_song__audio_track_next(track))){
		dbg (3, "testing trk='%s'.", track);
		if(!strcmp(track_name, track->name)){
			return track;
		}
	}
	track = NULL;
	while((track = (AyyiTrack*)ayyi_song__midi_track_next(track))){
		dbg (3, "testing trk='%s'.", track);
		if(!strcmp(track_name, track->name)) return track;
	}

	gwarn ("track not found. playlist='%s'. looking for track: %s", playlist_name, track_name);
	extern void ayyi_song__print_playlists();
	extern void ayyi_song__print_tracks();
	ayyi_song__print_playlists();
	ayyi_song__print_tracks();
	return NULL;
}

#if 0
static void*
ayyi_song_container_get_block(AyyiContainer* container, int block_num)
{
    return ayyi_song__translate_address(container->block[block_num]);
}
#endif


gboolean
ayyi_file_get_other_channel(AyyiFilesource* filesource, char* other, int n_chars)
{
	//return the filename of the other half of a stereo pair.

	g_strlcpy(other, filesource->name, n_chars);

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


AyyiFilesource*
ayyi_filesource_get_from_id(uint64_t id)
{
	AyyiFilesource* test = 0;
	while((test = ayyi_song__filesource_next(test))){
		if(test->id == id) return test;
	}
	if(ayyi.debug > 1) gwarn ("filesource shm data not found (id=%Lu)", id);
	return NULL;
}


AyyiFilesource*
ayyi_filesource_get_by_name(const char* name)
{
	AyyiFilesource* test = 0;
	while((test = ayyi_song__filesource_next(test))){
		//dbg(0, "  %s - %s", test->name, name);
		if(!strcmp(test->name, name)) return test;
	}
	return NULL;
}


int
ayyi_file_is_stereo(AyyiFilesource* filesource)
{
	//@return: 0=mono, 1=left, 2=right

	gchar* lr = g_strrstr(filesource->name, "%L.");
	if(lr) return 1;

	lr = g_strrstr(filesource->name, "%R.");
	if(lr) return 2;

	lr = g_strrstr(filesource->name, "-L.");
	if(lr) return 1;

	lr = g_strrstr(filesource->name, "-R.");
	if(lr) return 2;

	return 0;
}


AyyiFilesource*
ayyi_file_get_linked(AyyiFilesource* f1)
{
	//given the left channel, return the right.

	if(ayyi_file_is_stereo(f1)){
		char other[256];
		if(ayyi_file_get_other_channel(f1, other, 256)){
			AyyiFilesource* f2 = ayyi_filesource_get_by_name(other);
			if(f2){
				return f2;
			}
			else dbg(0, "f2=%p mono?", f2);
		}
	}
	return NULL;
}


gboolean
ayyi_connection_is_input(AyyiConnection* connection)
{
	return (connection->io == 1);
}


AyyiConnection*
ayyi_connection__next_input(AyyiConnection* c, int n_chans)
{
	//return the next input connection that matches the channel count.
	//-set n_chans=0 to ignore channel count

	while((c = ayyi_song__audio_connection_next(c)) && ((c->io != 1) || (n_chans ? (c->nports != n_chans) : FALSE))){
	}
	if(c) dbg(2, "n_chans=%i --> %i", n_chans, c->nports);
	return c;
}


AyyiConnection*
ayyi_connection__next_output(AyyiConnection* c, int n_chans)
{
	while((c = ayyi_song__audio_connection_next(c)) && ((c->io == 1) || (n_chans ? (c->nports != n_chans) : FALSE))){
	}
	if(c) dbg(2, "n_chans=%i --> %i", n_chans, c->nports);
	return c;
}


gboolean
ayyi_connection__parse_string(AyyiConnection* connection, char (*label)[64])
{
	//split the shm string into 3 parts - short name, port1, port2
	//format is subject to change!
	//format is: "short{port1}{port2}"

	g_return_val_if_fail(connection, false);

	label[1][0] = '\0';
	label[2][0] = '\0';

	//short name:
	char* a = strstr(connection->name, "{");
	if(a){
		strncpy(label[0], connection->name, a - connection->name);
		label[0][a - connection->name] = '\0';
	}
	else{ strcpy(label[0], connection->name); return TRUE; }

	//first port name
	char* b = a + 1;
	char* c = strstr(b, "}");
	if(!c) return FALSE;
	int len  = c - b;
	strncpy(label[1], b, len);
	label[1][len] = '\0';

	//maybe a 2nd port?
	if(c[1] != '{'){ label[2][0] = '\0'; return TRUE; }
	char* d = c + 2;
	char* e = strstr(d, "}");
	if(!e) return FALSE;
	len  = e - d;
	strncpy(label[2], d, len);
	label[2][len] = '\0';

	return TRUE;
}


GList*
ayyi_song__get_files_list()
{
	//list must be free'd after use.

	//note: use pool_model_update() to "sync" the gui audio-filelist.

	GList* file_list = NULL;
	block* block = ayyi_filesource_get_block(0);
	void** table = block->slot;
	if(block->last >= AYYI_BLOCK_SIZE){ log_print(LOG_FAIL, "song is corrupted"); gerr ("last=%i", block->last); return NULL; }
	int i = 0;
	for(i=0;i<=block->last;i++){
		if(!table[i]) continue;
		AyyiFilesource* shared = ayyi_song__filesource_at(i);
		if(!is_song_shm_local(shared)){ gerr ("failed to get shared filesource struct. table[%u]=%p", i, table[i]); continue; }

#ifdef ARDOURD_WORKS_PROPERLY
		char msg[256]; snprintf(msg, 255, "file has zero length! %s", shared->name);
		if(!shared->length) log_append(msg, LOG_WARN);
#endif

		if(shared->id == 0){ log_print(LOG_FAIL, "bad song data found. File has illegal source_id 0."); continue; }

		file_list = g_list_append(file_list, GINT_TO_POINTER(i));
	}

	dbg (2, "num files found: %i", g_list_length(file_list));
	return file_list;
}


static block*
ayyi_filesource_get_block(int block_num)
{
	return ayyi_song__translate_address(ayyi.service->song->filesources.block[block_num]);
}


gboolean
ayyi_song__have_file(const char* fname)
{
	// @param fname - must be either absolute path, or relative to song root.

	// if the engine is 'importing' files, the imported path will always be
	// different than the external one, so this fn will always return false.
	// **** hence we now only match against the leafname ****
	// (despite the fact that this means we cannot distinguish between similar files in different directories)

	// also, if the engine has seen multiple versions of the file, it may change the name
	// -in the case of Ardour, a '_' will be appended.

	// because we compare leaf names, we cannot reliably have files with the same name in different directories.

	dbg (1, "looking for '%s'...", fname);
	gboolean found = false;

	/*
	char leafname[256];
	audio_path_get_wav_leaf(leafname, fname, 256); //this means we cant have different files with the same name. Fix later...

	//check for stereo files:
	char base[256];
	strcpy(base, leafname);
	char* pos = g_strrstr(base, ".");
	if(pos){
		*pos = '\0';
	}
	char* left = g_strdup_printf("%s-L.wav", base);
	*/

	gchar* target = audio_path_get_base(fname);
	audio_path_truncate(target, '_');
	//TODO tidy. ardour now seems to append '-N' to file names that are already used
	audio_path_truncate(target, '1');
	audio_path_truncate(target, '2');
	audio_path_truncate(target, '3');
	audio_path_truncate(target, '4');
	audio_path_truncate(target, '5');
	audio_path_truncate(target, '6');
	audio_path_truncate(target, '-');

	//char test_leaf[256];
	AyyiFilesource* f = NULL;
	while((f = ayyi_song__filesource_next(f))){
		dbg (1, "%i: %s orig=%s", f->shm_idx, f->name, f->original_name);

		gchar* orig_base = audio_path_get_base(f->original_name);
		if(!strcmp(orig_base, target)){
			dbg(1, "found from original");
			found = true;
			break;
		}
		g_free(orig_base);

		gchar* test_base = audio_path_get_base(f->name);
		audio_path_truncate(test_base, '_');
		audio_path_truncate(test_base, '1');
		audio_path_truncate(test_base, '2');
		audio_path_truncate(test_base, '3');
		audio_path_truncate(test_base, '4');
		audio_path_truncate(test_base, '5');
		audio_path_truncate(test_base, '6');
		audio_path_truncate(test_base, '7');
		audio_path_truncate(test_base, '-');

		dbg(1, "        %s %s", target, test_base);
		if(!strcmp(test_base, target)){
			found = true;
		}else{
			/*
			dbg(0, "        %s %s", base, test_base);
			if(!strcmp(left, test_base)){
				found = true;
				break;
			}
			*/
		}
		g_free(test_base);
		if(found) break;
	}
	if(!found) dbg (1, "file not found in pool (%s).", fname);

	//if(left) g_free(left);
	if(target) g_free(target);

	return found;
}


const char*
ayyi_song__get_file_path()
{
	//session path?
	return ayyi.service->song->path;
}


char*
ayyi_song__get_audiodir()
{
	//strictly speaking, i think Ardour files can be anywhere. The path is in the XML file.
	//But we dont currently share the full path, so we have to assume that they are in the Interchange folder.

	char d[256]; snprintf(d, 255, "%s%s%s", ayyi.service->song->path, ARDOUR_SOUND_DIR1, ayyi.service->song->snapshot);
	if(!g_file_test(d, G_FILE_TEST_EXISTS)){
		snprintf(d, 255, "%s%s", ayyi.service->song->path, ARDOUR_SOUND_DIR1);
		GList* dir_list = get_dirlist(d);
		if(dir_list){
			strncpy(d, (char*)dir_list->data, 255);

			//free the dir list
			GList* l = dir_list; for(;l;l=l->next) g_free(l->data);
			g_list_free(dir_list);
		}
	}

	return g_build_filename(d, ARDOUR_SOUND_DIR2, NULL);
}


void
ayyi_song__print_pool()
{
	//list all the pool items in shm. See also pool_print() for the gui data.

	UNDERLINE;
	AyyiFilesource* file = NULL;
	while((file = ayyi_song__filesource_next(file))){
		printf("%4i %12Lu %6i %s\n", file->shm_idx, file->id, file->length, file->name);
	}
	UNDERLINE;
}


static gboolean
ayyi_region__validate_source(AyyiAudioRegion* part)
{
	if(!part->source0){ gwarn ("part has no source. '%s'", part->name); return FALSE; }
	if(!ayyi_filesource_get_from_id(part->source0)){ gwarn("part has invalid source id (source doesnt exist). sourceid=%Lu", part->source0); return FALSE; }
	return TRUE;
}


#define fail_if(TEST, MSG, ...) if(TEST){ gwarn(MSG, ##__VA_ARGS__); ok = FALSE; }
gboolean
ayyi_song__verify(shm_seg* seg)
{
	g_return_val_if_fail(seg, FALSE);
	g_return_val_if_fail(seg->type == SEG_TYPE_SONG, FALSE);
	gboolean ok = TRUE;
	shm_virtual* shmv = seg->address;
	g_return_val_if_fail(shmv, FALSE);

	AyyiShmSong* song = (AyyiShmSong*)shmv;
	if(!song->path[0]){ gerr("bad shm. session path not set."); ok = FALSE; }

	if(song->bpm < 10 || song->bpm > 200) gwarn("bpm=%.2f", song->bpm);

	//check tracks...
	AyyiContainer* container = &((AyyiShmSong*)shmv)->audio_tracks;
	fail_if(container->obj_type != AYYI_OBJECT_AUDIO_TRACK, "obj_type not set.");
	if(strcmp(container->signature, "Ayyi Routes")){
		gwarn("bad shm tracks signature: %s", container->signature);
		ok = FALSE;
	}
//ayyi_container_print(&ayyi.service->song->audio_tracks);
//ayyi_song__print_tracks();
	AyyiTrack* track = NULL;
	while((track = ayyi_song__audio_track_next(track))){
		fail_if(!track->id, "track id not set: t=%i", track->shm_idx);
	}

	container = &ayyi.service->song->midi_tracks;
	/*
	if(strcmp(container->signature, "Ayyi Midi Trks")){
		gwarn("bad shm tracks signature: %s", container->signature);
		ok = FALSE;
	}
	*/
	fail_if(strcmp(container->signature, "Ayyi Midi Trks"), "bad shm tracks signature: %s", container->signature);

	//check audio files:
	container = &ayyi.service->song->filesources;
	if(!ayyi_song_container_verify(&ayyi.service->song->filesources)) ok = FALSE;
	fail_if(container->obj_type != AYYI_OBJECT_FILE, "files: obj_type not set.");

	//check audio parts:
	container = &ayyi.service->song->audio_regions;
	if(!ayyi_song_container_verify(&ayyi.service->song->audio_regions)) ok = FALSE;
	fail_if(container->obj_type != AYYI_OBJECT_AUDIO_PART, "audio parts: obj_type not set.");
	AyyiAudioRegion* part = NULL;
	while((part = ayyi_song__audio_region_next(part))){
		fail_if(!ayyi_region__validate_source(part), "invalid source");
		if(!ok) break; //dangerous to iterate over broken container. can loop forever.
	}

	container = &ayyi.service->song->midi_regions;
	if(!ayyi_song_container_verify(container)) ok = FALSE;
	fail_if(container->obj_type != AYYI_OBJECT_MIDI_PART, "midi parts: obj_type not set.");
	AyyiMidiRegion* mpart = NULL;
	while((mpart = ayyi_song__midi_region_next(mpart))){
		if(!ayyi_song_container_verify(&mpart->events)){
			gwarn("midi part content verification failed");
			ok = FALSE;
		}
	}
	//block* midi_routes = ardour_midi_track_get_block(0); //TODO do something with this

	if(!ok) dbg(0, "address=%p sig=%s%s%s type=%s%s%s owner_shm=%p", shmv, bold, shmv->service_name, white, bold, shmv->service_type, white, shmv->owner_shm);
	seg->invalid = !ok;
	return ok;
}


void
ayyi_song__print_part_list()
{
	printf("ayyi regions:\n");
	if(ayyi_song__get_region_count()) printf("%s (%10s)         %s      pos    start               %1s   sourceid                      playlist_name\n", "slot", "", "id", "F");

	char info[256];
	int total = 0;
	AyyiRegion* part = NULL;
	while((part = ayyi_song__audio_region_next(part))){
		info[0] = '\0';
		AyyiPlaylist* playlist = ayyi_song__playlist_at(part->playlist);
		if(!playlist){ snprintf(info, 255, "non playlist region."); }
		else total++;

		char flags[8];
		snprintf(flags, 7, "%s", (part->flags & deleted) ? "D" : "-");

//dbg(0, "position=%u start=%u tot=%u", part->position, part->start, part->position + part->start);
		uint64_t a = part->position + part->start;
		char bbst[64]; samples2bbst(a, bbst);

		char playlist_name[AYYI_NAME_MAX] = "";
		if(playlist) strcpy(playlist_name, playlist->name);

		printf("  %2u (%p) %10Lu %8u %8u %s %1s %Lu %18s '%12s' %s\n", part->shm_idx, part, part->id, part->position, part->start, bbst, flags, part->source0, part->name, playlist_name, info);
	}
	printf("  num parts found: %i. last=%i\n", total, 0/*block->last*/);
}


void
ayyi_song__print_playlists()
{
	int count = 0;
	AyyiPlaylist* shared = NULL;
	UNDERLINE;
	printf("ayyi playlists:\n");
	while((shared = ayyi_song__playlist_next(shared))){
		printf("  %2i %s\n", shared->shm_idx, shared->name);
		count++;
	}

	printf("num playlists found: %i\n", count);
	UNDERLINE;
}


void
ayyi_song__print_tracks()
{
	UNDERLINE;

	block*
	ayyi_route_get_block(int block_num)
	{
		return ayyi_song__translate_address(ayyi.service->song->audio_tracks.block[block_num]);
	}

	block*
	midi_track_get_block(int block_num)
	{
		return ayyi_song__translate_address(ayyi.service->song->midi_tracks.block[block_num]);
	}

	printf("ayyi tracks: audio_last=%i midi_last=%i\n", ayyi_route_get_block(0)->last, midi_track_get_block(0)->last);
	printf("   shm               id %2s\n", "fl");

	AyyiTrack* route = NULL;
	while((route = ayyi_song__audio_track_next(route))){
		int t = route->shm_idx;
		printf("  [%2i] %16Lu %2i '%s'\n", t, route->id, route->flags, route->name);
	}

	AyyiMidiTrack* track = NULL;
	while((track = ayyi_song__midi_track_next(track))){
		printf("  [%2i] %16Lu %2i '%s'\n", track->shm_idx, track->id, track->flags, track->name);
	}
	UNDERLINE;
}


void
ayyi_song__print_midi_tracks()
{
	PF;
	int total=0;
	midi_track_shared* track = NULL;
	while((track = ayyi_song__midi_track_next(track))){
		printf("  %2u: %10s %p\n", total, track->name, track);
		total++;
	}

	printf("  total midi tracks: %i\n", total);
}


MidiNote*
ayyi_midi_note_new()
{
	return g_new0(MidiNote, 1);
}


