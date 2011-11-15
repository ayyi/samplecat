#ifndef __SAMPLECAT_OVERVIEW_H
#define __SAMPLECAT_OVERVIEW_H

#include <gtk/gtk.h>
#include "typedefs.h"

enum MsgType
{
	MSG_TYPE_OVERVIEW,
	MSG_TYPE_PEAKLEVEL,
	MSG_TYPE_EBUR128,
} MsgType;

struct _message
{
	enum MsgType type;
	Sample* sample;
};

gpointer    overview_thread        (gpointer data);
void        request_overview       (Sample*);
void        request_peaklevel      (Sample*);
void        request_ebur128        (Sample*);
#endif
