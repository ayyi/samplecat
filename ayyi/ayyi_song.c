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

static struct block* route_get_block(int block_num);
static block* ayyi_filesource_get_block(int block_num);
static gboolean ayyi_song__container_index_ok(AyyiContainer*, int shm_idx);


void*
ayyi_song__translate_address(void* address)
{
	//pointers in shm have to be translated to the address range of the imported segment.

	if(!address) return NULL;

	void* translated = (void*)((uint32_t)address + (uint32_t)ayyi.service->song - (uint32_t)ayyi.service->song->owner_shm);

	//dbg(0, "owner %p -> local %p", address, translated);
	return (void*)translated;
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

	AyyiTrack* track = NULL;
	while((track = ayyi_song__audio_track_next(track))){
		dbg (3, "id=%Lu", track->id);
		if(track->id == id) return track;
	}
	while((track = ayyi_song__midi_track_next(track))){
		if(track->id == id) return track;
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
	if(((unsigned int)a>(unsigned int)ayyi.service->song) && ((unsigned int)a<segment_end)) return TRUE;

	gerr ("address is not in song shm: %p (orig=%p) (expected %p - %p)\n", a, p, ayyi.service->song, (void*)segment_end);
	if((unsigned)p > (unsigned)ayyi.service->song) gerr ("shm offset=%u translated_offset=%u\n", (unsigned)p - (unsigned)ayyi.service->song, (unsigned)a - (unsigned)ayyi.service->song);
	return FALSE;
}


gboolean
is_song_shm_local(void* p)
{
	//returns true if the pointer is in the local shm segment.
	//returns false if the pointer is in a foreign shm segment.

	//if(((unsigned int)p>(unsigned int)ayyi.service->song) && ((unsigned int)p<segment_end)) return TRUE;
	if(is_song_shm_local_quiet(p)) return TRUE;

	shm_seg* seg_info = ayyi_song__get_info();
	unsigned segment_end = (unsigned)ayyi.service->song + seg_info->size;
	gerr ("address is not in song shm: %p (expected %p - %p)", p, ayyi.service->song, (void*)segment_end);
	if((unsigned)p > (unsigned)ayyi.service->song) gerr ("shm offset=%u", (unsigned)p - (unsigned)ayyi.service->song);
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
	// returns a list of audio region indexes from the shared_regions struct in shm.
	//
	// parts that are 'not being used' (are not in any playlists), are ignored.
	//
	// the returned list must be free'd.

	GList* part_list = NULL;
	AyyiAudioRegion* region = NULL;
	while((region = ayyi_song__audio_region_next(region))){
		if(!strlen(region->playlist_name)) continue; //ignore non-playlist regions. Deleted parts also dont have a playlist.
		part_list = g_list_append(part_list, GINT_TO_POINTER(region->shm_idx));
	}

	dbg (1, "num audio parts found: %i", g_list_length(part_list));
	return part_list;
}


AyyiItem*
ayyi_song_container_get_item(AyyiContainer* container, int idx)
{
	return ayyi_container_get_item(container, idx, ayyi_song__translate_address);
}


void*
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

	if(old && (ayyi_song_container_find(container, old) >= BLOCK_SIZE)) gwarn ("FIXME");

	int not_first_block = 0;

	int b; for(b=container->last;b>=0;b--){
		struct block* blk = ayyi_song__translate_address(container->block[b]);
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
		struct block* block = translator(container->block[b]);
		if(block->full){
			count += BLOCK_SIZE;
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


gboolean
ayyi_region_pod_index_ok(AyyiIdx pod_index)
{
	//check that the given region slot number is valid.

	return ayyi_song__container_index_ok(&ayyi.service->song->audio_regions, pod_index);
}


static gboolean
ayyi_song__container_index_ok(AyyiContainer* container, int shm_idx)
{
	int b = shm_idx / BLOCK_SIZE;
	int i = shm_idx % BLOCK_SIZE;
	struct block* block = translate(container->block[b]);
	if (i < 0 || i >= BLOCK_SIZE) return FALSE;
	if (b < 0 || b >= CONTAINER_SIZE) return FALSE;
	AyyiItem* item = ayyi_song_container_get_item(container, shm_idx);
	return ((item->flags | deleted) || (i <= block->last));
}


AyyiAction*
ayyi_region__delete_async(AyyiAudioRegion* region)
{
	g_return_val_if_fail(region, NULL);

	AyyiAction* a   = ayyi_action_new("region delete %i \"%s\"", region->shm_idx, region->name);
	a->op           = AYYI_DEL;
	a->obj_type     = AYYI_OBJECT_AUDIO_PART;
	a->obj_idx.idx1 = region->shm_idx;
	ayyi_action_execute(a);

	return a;
}


AyyiAudioRegion*
ayyi_region_lookup_by_name(char* name)
{
	//can be used to check that a name is unused, so not finding a region is not considered an error.

	AyyiAudioRegion* test = NULL;
	while((test = ayyi_song__audio_region_next(test))){
		if(!strcmp(test->name, name)) return test;
	}

	return NULL;
}


void
ayyi_region_name_make_unique(char* source_name_unique, const char* source_name)
{
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
}


AyyiIdx
ayyi_region_get_pod_index(AyyiRegionBase* region)
{
	g_return_val_if_fail(region, 0);
	return region->shm_idx;
}


uint32_t
ayyi_region_get_start_offset(AyyiAudioRegion* region)
{
	if (!region) return 0;

	return region->start;
}


static struct block*
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
		if(track->armed) return track;
	}

	gwarn("no armed tracks found.");
	return NULL;
}


AyyiConnection*
ayyi_track__get_output(AyyiTrack* ayyi_trk)
{
	AyyiList* routing = ayyi_list__first(ayyi_trk->output_routing);
	int idx = routing ? routing->id : 0;
	if(routing) dbg (2, "output_idx=%i name=%s", routing->id, ayyi_song__connection_get(idx) ? ((AyyiConnection*)ayyi_song__connection_get(idx))->name : "");
	return (AyyiConnection*)ayyi_song__connection_get(idx);
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
	return ayyi_mixer__channel_get(at->shm_idx);
}


void
ayyi_track__make_name_unique(char* unique, const char* name)
{
	dbg(0, "FIXME");
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


#if 0
static void*
ayyi_song_container_get_block(AyyiContainer* container, int block_num)
{
    return ayyi_song__translate_address(container->block[block_num]);
}
#endif


gboolean
ayyi_file_get_other_channel(AyyiFilesource* filesource, char* other)
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


gboolean
ayyi_connection_is_input(AyyiConnection* connection)
{
	return (connection->io == 1);
}


AyyiConnection*
ayyi_connection_next_input(AyyiConnection* c, int n_chans)
{
	//return the next input connection that matches the channel count.

	while((c = ayyi_song__audio_connection_next(c)) && ((c->io != 1) || (n_chans ? (c->nports != n_chans) : FALSE))){
	}
	dbg(0, "n_chans=%i --> %i", n_chans, c->nports);
	return c;
}


GList*
ayyi_song__get_files_list()
{
	//list must be free'd after use.

	//note: use pool_model_update() to "sync" the gui audio-filelist.

	GList* file_list = NULL;
	block* block = ayyi_filesource_get_block(0);
	void** table = block->slot;
	if(block->last>=BLOCK_SIZE){ log_print(LOG_FAIL, "song is corrupted"); gerr ("last=%i", block->last); return NULL; }
	int i = 0;
	for(i=0;i<=block->last;i++){
		if(!table[i]) continue;
		AyyiFilesource* shared = ayyi_song__filesource_get(i);
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


static struct block*
ayyi_filesource_get_block(int block_num)
{
	return ayyi_song__translate_address(ayyi.service->song->filesources.block[block_num]);
}


gboolean
ayyi_song__have_file(const char* fname)
{
	// looks through the core fil array looking for the given filename.
	// @param fname - must be either absolute path, or relative to song root.

	// if the engine is 'importing' files, the imported path will always be
	// different than the external one, so this fn will always return false.

	dbg (2, "looking for '%s'...", fname);

	char leafname[256];
	audio_path_get_leaf(fname, leafname); //this means we cant have different files with the same name. Fix later...

	char test_leaf[256];
	AyyiFilesource* f = NULL;
	while((f = ayyi_song__filesource_next(f))){
		dbg (3, "%i: %s", f, f->name);
		if(!strcmp(f->name, leafname)) return true;

		//compare leaf names - this means we cannot have files with the same name in different directories.
		audio_path_get_leaf(f->name, test_leaf);
		if(!strcmp(test_leaf, leafname)) return true;
	}
	dbg (2, "file not found in pool (%s).", fname);
	return false;
}


const char*
ayyi_song__get_file_path()
{
	//session path?
	return ayyi.service->song->path;
}


void
ayyi_song__get_audiodir(char* dir)
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

	snprintf(dir, 255, "%s/%s/", d, ARDOUR_SOUND_DIR2);
	dbg (2, "using dir: %s", dir);
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

	//check tracks...
	AyyiContainer* container = &((AyyiShmSong*)shmv)->audio_tracks;
	if(strcmp(container->signature, "Ayyi Routes")){
		gwarn("bad shm tracks signature: %s", container->signature);
		ok = FALSE;
	}
	container = &ayyi.service->song->midi_tracks;
	/*
	if(strcmp(container->signature, "Ayyi Midi Trks")){
		gwarn("bad shm tracks signature: %s", container->signature);
		ok = FALSE;
	}
	*/
	fail_if(strcmp(container->signature, "Ayyi Midi Trks"), "bad shm tracks signature: %s", container->signature);

	//check audio parts:
	if(!ayyi_song_container_verify(&ayyi.service->song->audio_regions)) ok = FALSE;
	AyyiAudioRegion* part = NULL;
	while((part = ayyi_song__audio_region_next(part))){
		fail_if(!ayyi_region__validate_source(part), "invalid source");
	}

	if(!ayyi_song_container_verify(&ayyi.service->song->midi_regions)) ok = FALSE;
	//block* midi_routes = ardour_midi_track_get_block(0); //TODO do something with this

	if(!ok) dbg(0, "address=%p sig=%s%s%s type=%s%s%s owner_shm=%p", shmv, bold, shmv->service_name, white, bold, shmv->service_type, white, shmv->owner_shm);
	seg->invalid = !ok;
	return ok;
}


void
ayyi_song__print_part_list()
{
	printf("ayyi regions:\n");
	if(ayyi_song__get_region_count()) printf("%s (%10s)         %s      pos    start                 sourceid                      playlist_name\n", "slot", "", "id");

	char info[256];
	int total = 0;
	AyyiRegion* part = NULL;
	while((part = ayyi_song__audio_region_next(part))){
		info[0] = '\0';
		if(!strlen(part->playlist_name)){ snprintf(info, 255, "non playlist region."); }
		else total++;

//dbg(0, "position=%u start=%u tot=%u", part->position, part->start, part->position + part->start);
		uint64_t a = part->position + part->start;
		char bbst[64]; samples2bbst(a, bbst);
		printf("  %2u (%p) %10Lu %8u %8u %s %Lu %18s '%12s' %s\n", part->shm_idx, part, part->id, part->position, part->start, bbst, part->source0, part->name, part->playlist_name, info);
	}
	printf("  num parts found: %i. last=%i\n", total, 0/*block->last*/);
}


