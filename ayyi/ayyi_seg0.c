#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <glib.h>
#include <jack/jack.h>

//typedef void             action;
#include <ayyi/ayyi_typedefs.h>
#include <ayyi/ayyi_types.h>
#include <ayyi/ayyi_shm.h>
#include <ayyi/ayyi_client.h>

#include <ayyi/interface.h>

#include "ayyi_seg0.h"

extern struct _ayyi_client ayyi;


uint32_t
ayyi_transport__get_frame()
{
	return ayyi.song->transport_frame;
}
