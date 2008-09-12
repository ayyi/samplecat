#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <glib.h>
#include <jack/jack.h>

typedef struct _shm_seg  shm_seg;
typedef void             action;
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_utils.h>
#include <ayyi/interface.h>
#include <ayyi/ayyi_msg.h>
#include <ayyi/ayyi_client.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_list.h>

void*    ayyi_song_translate_address(void* address);


int
ayyi_list__length(AyyiList* list)
{
	guint length = 0;
	list = ayyi_song_translate_address(list);
	while(list){
		length++;
		list = ayyi_song_translate_address(list->next);
	}

	return length;
}


AyyiList*
ayyi_list__first(AyyiList* list)
{
	return ayyi_song_translate_address(list);
}


AyyiList*
ayyi_list__next(AyyiList* list)
{
	if(!list) return NULL;

	//dbg(0, "%p --> %p", list, ayyi_song_translate_address(list->next));
	return ayyi_song_translate_address(list->next);
}


void*
ayyi_list__first_data(AyyiList** list_p)
{
	AyyiList* list = *list_p;
	if(!list) return NULL;

	list = *list_p = ayyi_song_translate_address(list);

	dbg(0, "  list_p=%p list=%p container=%p", list_p, *list_p, list ? ayyi_song_translate_address(list->data) : NULL);
	return list ? ayyi_song_translate_address(list->data) : NULL;
}


void*
ayyi_list__next_data(AyyiList** list_p)
{
	dbg(3, "  list_p=%p list=%p", list_p, *list_p);
	AyyiList* list = *list_p;
	if(!list) return NULL;

	list = *list_p = ayyi_song_translate_address(list->next);

	return list ? ayyi_song_translate_address(list->data) : NULL;
}


