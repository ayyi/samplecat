#ifndef __ayyi_msg_h__
#define __ayyi_msg_h__

#include <sys/time.h>

struct _ayyi_action
{
	unsigned       trid;
	struct timeval time_stamp;
	char           label[64];     // a description of the transaction. For printing only.

	int            obj_type;
	AyyiObjIdx     obj_idx;
	AyyiOp         op;
	int            prop;
	int            i_val;
	double         d_val;
	double         d_val2;

	void           (*callback)(); // the func to call upon reception of succesful completion of the transaction
	//data to forward to callback:
	int            val1;
	int            val2;
	void*          app_data;

	GList*         pending;       // list of actions that must be completed before this one is started.

	//return values:
	int            gid;           // the object id number returned by the engine following the request.
	int            idx;           // the object idx returned by the engine.
	char           err;           // zero if no errors reported.
	void*          return_val_1;
};

#endif //__ayyi_msg_h__
