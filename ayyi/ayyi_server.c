
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <glib.h>
#include "ayyi_typedefs.h"
#include "ayyi_types.h"
#include "ayyi_utils.h"
//#include "ayyi_shm_seg.h"
#include "ayyi/ayyi_shm.h"
#include "ayyi_server.h"
#include "ayyi_msg.h"
#include "ayyi_dbus.h"
#include "ayyi_log.h"
//#include "dbus.h" //FIXME

AyyiServer* server = NULL; //tmp - for dbus.


AyyiServer*
ayyi_server__new(GList* segs)
{
	PF;

	AyyiServer* ayyi = g_new0(AyyiServer, 1);

	ayyi->session_num = 1;

	//shm_debug        = 0;
	ayyi->got_messaging    = 0;
	ayyi->got_shm          = 0;
	ayyi->shm_connect_sent = FALSE;
	ayyi->shm_segs         = NULL;

	GList* l = segs;
	for(;l;l=l->next){
		AyyiShmSeg* seg = l->data;
		ayyi_server__add_shm_segment(ayyi, seg);
	}

	if(0){
		GError* error = NULL;
		Service* ardourd = &known_services[0];
		if((/*app.dbus = */ayyi_client_connect(ardourd, &error))){
			log_print(LOG_OK, "dbus connection");
			gboolean dbus_server_get_shm__server(AyyiServer*);
			dbus_server_get_shm__server(ayyi);
		}
		else { P_GERR; log_print(LOG_FAIL, "dbus connection"); exit(1); }
	}

	return ayyi;
}


void
ayyi_server__add_shm_segment(AyyiServer* ayyi, AyyiShmSeg* seg)
{
	//create a generic local shm segment.

	//its up to the application to deal with type-specific details of the segment.

	ayyi->shm_segs = g_list_append(ayyi->shm_segs, seg);

	shm_seg__attach(seg);
}


//TODO merge fn with client shm_seg_new() once AyyiShmSeg struct sorted.
AyyiShmSeg*
ayyi_server__shm_seg__new(SegType seg_type, int size)
{
	AyyiShmSeg* seg = g_new0(AyyiShmSeg, 1);

	int pagesize = sysconf(_SC_PAGESIZE);
	int num_pages = size / pagesize + ((size % pagesize) ? 1 : 0);

	seg->size = pagesize * num_pages;
	seg->_shmkey = 0;
	seg->_shmfd = 0;
	strcpy(seg->name, seg_strs[seg_type]);

	return seg;
}


void
ayyi_server__delete_shm_segment(AyyiServer* ayyi, AyyiShmSeg* seg)
{
	g_free(g_list_find(ayyi->shm_segs, seg));
	ayyi->shm_segs = g_list_remove(ayyi->shm_segs, seg);
}


void
ayyi_server__shm_segment_reset(AyyiServer* ayyi, char* name)
{
	//clean up following a shm create error.

	ayyi->got_shm = 0;

	GList* l = ayyi->shm_segs;
	for(;l;l=l->next){
		AyyiShmSeg* seg = l->data;
		if(!seg->size) continue;
		if(strcmp(seg->name, name)) continue;
		seg->address = NULL;
		return;
	}
	gerr("couldnt find segment. name=%s\n", name);
}


void
ayyi_server__add_foreign_shm_segment(AyyiServer* ayyi, void* shmptr)
{
	AyyiShmSeg* seg = g_new0(AyyiShmSeg, 1);
	seg->address = shmptr;
	ayyi->foreign_shm_segs = g_list_append(ayyi->foreign_shm_segs, seg);
}


static void
ayyi_server__shm_connect(AyyiServer* ayyi)
{
	//request shm segments be made.

	//FIXME is this fn for local or foreign segments?

	if(ayyi->shm_connect_sent) return;

	GList* l = ayyi->shm_segs;
	for(;l;l=l->next){
		//if(!shm_segs[i].size) continue;
		//dae_shm_connect_export(shm_segs[i].name, shm_segs[i].size, DAE_SHM_R);
	}
	ayyi->shm_connect_sent = TRUE;
}


static void
ayyi_server__shm_disconnect()
{
	//disconnect all shm segments. They will be destroyed if no longer used.

	/*
	for(int i=0;i<10;i++){
		if(!shm_segs[i].size) continue;
		printf("%s(): %s...\n", __func__, shm_segs[i].name);
		if(dae_shm_disconnect(shm_segs[i].name, DAE_SHM_EXPORT)) printf("%s(): *** failed to disconnect shm segment: %s\n", __func__, shm_segs[i].name);
		sleep(2); //does this help prevent the machine from momentarily freezing?
	}
	*/
}


gboolean
shm_import(AyyiServer* ayyi)
{
	//attach all available shm segments.

	GList* list = ayyi->shm_segs;
	for(;list;list=list->next){
		AyyiShmSeg* shm_seg = list->data;
		if(shm_seg->id){
			if(shm_seg->attached){ /*printf("%s(): seg already attached.\n", __func__);*/ continue; }

			void* shmptr;
			if((shmptr = shmat(shm_seg->id, 0, 0)) == (void *)-1){
				gerr ("shmat() %s", fail);
				return FALSE;
			}
			if(shm_seg__validate()){
				printf("ayyi data import: %s \n%s %s\n", seg_strs[shm_seg->type], go_rhs, ok);
				shm_seg->attached = TRUE;

				switch(shm_seg->type){
					case 1:
#if 0
						//currently we can probably get away without engine-specific callback.
						//In fact we're not supporting engine-specific features at all.
#endif
						ayyi_server__add_foreign_shm_segment(ayyi, shmptr);
						break;
					case 2:
						//shm.amixer = shmptr;
						break;
					default:
						gerr ("unexpected shm segment type %i", shm_seg->type);
						break;
				}
			} else {
				printf("%s(): %s \n%s %s\n", __func__, seg_strs[shm_seg->type], go_rhs, fail);
				return FALSE;
			}
		}// else printf("%s(): segment has no id!\n", __func__);
	}

	return TRUE;
}


/*
shm_seg_defn*
Ayyi_server::shm_segment_find_by_name(char* name)
{
	shm_seg_defn* ret = NULL;

	for(int i=0;i<10;i++){
		if(!shm_segs[i].size) continue;
		//shm_segs[i].size = size;
		if(!strcmp(shm_segs[i].name, name)) return &shm_segs[i];
	}
	printf("%s(): **** not found: %s\n", __func__, name);
	return ret;
}
#endif


bool
Ayyi_server::shm_is_complete()
{
	//checks if all segments have been created, and updates got_shm.

	//this is only used for imported segments. Locally created segments are assumed to be complete.

	for(int i=0;i<10;i++){
		if(!shm_segs[i].size) continue;
		if(!shm_segs[i].seg_id){
			return (got_shm = false);
		}
	}

	//(debug) check the address is set:
	for(int i=0;i<10;i++) if(shm_segs[i].size) if(!shm_segs[i].address) printf("%s(): *** address not set!\n", __func__);

	return (got_shm = true);
}


int
Ayyi_server::shm_count_active_segments()
{
	int count = 0;
	for(int i=0;i<10;i++){
		if(!shm_segs[i].size || !shm_segs[i].address) continue;
		count++;
	}
	return count;
}


void
Ayyi_server::shm_cleanup()
{
	//remove orphaned shm segments.

	struct shmid_ds shmseg;
	struct shm_info shm_info;

	int maxid = shmctl (0, SHM_INFO, (struct shmid_ds *) (void *) &shm_info);
	if (maxid < 0) {
		printf ("kernel not configured for shared memory?\n");
		return;
	}

	int count = 0;
	for (int id = 0; id <= maxid; id++) {
		int shmid = shmctl (id, SHM_STAT, &shmseg);
		if (shmid < 0) continue;
		if (shmseg.shm_nattch) continue; //segment is still being used.

		if(debug) printf("Ayyi_server::%s(): %12i %12lu %li\n", __func__, shmid, (unsigned long)shmseg.shm_segsz, (long)shmseg.shm_nattch);

		int r;
		if(!(r = shmctl(shmid, IPC_RMID, &shmseg))){
			count++;
		} else {
			err("shmctl error! cannot delete dead shm segment.\n");
		}
	}

	if(debug && count) printf("Ayyi_server::%s(): %i dead shm segments removed.\n", __func__, count);
}


void
Ayyi_server::set_shm_callback(int (Ayi::Ayyi_server::*recv_shm_notify)(short int, int, char*, int, int, int))
{
	//cant compile - is there a missing 'this'?

 	//note: a given member function pointer can only point to functions that are members of the class it was declared with. It cannot be applied to an object of a different class even if it has member functions with the same signature

	if(debug) cout << "Ayyi_server::set_shm_callback(): setting callback..." << endl;
	//dae_shm.shm_notify_callback = (int (*)(short int, int, char*, int, int, int))recv_shm_notify;
}


int
Ayyi_server::recv_shm_notify(short int _notify_level, int _seg_id, char *_name, int _pages, int _rw, int _status)
{
	return 0;
}


#ifndef USE_DBUS
void
Ayyi_server::shm_set_debug(int level)
{
	dae_shm_set_debug(level);
}
#endif


struct block*
Ayyi_server::block_init(container* container)
{
	//allocate and initialise shm for a new block and make it current

	if(!container){ err("bad arg! no container!\n"); return NULL; }

	//cout << "Ayyi_server::block_init()..." << endl;
	//cout << "Ayyi_server::block_init(): " << " container=" << container << endl;

	//find next free block:
	int b;
	bool found = false;
	for(b=0;b<CONTAINER_SIZE;b++){
		if(!container->block[b]){ found = true; break; }
	}
	if(!found){ err("cant find empty block. out of memory?\n"); return NULL; }

	//int b = i;
	container->last = max(container->last, b);
	if(shm_debug) printf("Ayyi_server::%s(): new_block=%i\n", __func__, b);


	struct block* block = container->block[b] = (struct block*)shm_malloc(sizeof(*block), NULL); //hmmm, recursive. Pass null to stop feedback...?
	//cout << "Ayyi_server::block_init(): block=" << block << endl;
	if(!block){ err("block NULL!!\n"); return NULL; }

	for(int i=0; i<BLOCK_SIZE; i++) block->slot[i] = NULL;
	block->last = -1;
	block->full = false;

	container->last = b;
	return container->block[b];
}


struct block*
Ayyi_server::msg_idx_get_block(unsigned idx, int* block_num, int* s2)
{
	//convert a serialised index into two separate array indexes for the Container and Block.

	*block_num = idx & 0xffff0000;
	*s2        = idx & 0x0000ffff;
	if(*block_num >= CONTAINER_SIZE){ err("bad index: %i\n", idx); return NULL; }
	return shm_index->regions.block[*block_num];
}


void*
Ayyi_server::shm_get_address_next()
{
	//return (unsigned)shm_address_next ? shm_address_next : NULL;
	return shm_index->next[0];
}


std::string*
Ayyi_server::get_object_string(int object_type)
{
	static std::string str[AYYI_OBJECT_ALL];
	str[AYYI_OBJECT_TRACK] = "AYYI_OBJECT_TRACK";
	str[AYYI_OBJECT_PART]  = "AYYI_OBJECT_PART";
	str[AYYI_OBJECT_CHAN]  = "AYYI_OBJECT_CHAN";
	str[AYYI_OBJECT_STRING]= "AYYI_OBJECT_STRING";
	return &str[0];
}
*/
