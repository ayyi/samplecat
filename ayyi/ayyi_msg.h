#ifndef __ayyi_msg_h__
#define __ayyi_msg_h__

#include <sys/time.h>

struct _ayyi_action
{
	unsigned       trid;
	struct timeval time_stamp;
	char           label[64];     // a description of the transaction. For printing only.

	AyyiObjType    obj_type;
	AyyiObjIdx     obj_idx;
	AyyiOp         op;
	int            prop;
	int            i_val;
	double         d_val;
	double         d_val2;

	AyyiActionCallback callback;  // the func to call upon reception of succesful completion of the transaction
	void*              app_data;  // data to forward with the callback

	GList*             pending;   // list of actions that must be completed before this one is started.

	//return values:
	struct _r {
		int         gid;          // the object id number returned by the engine following the request.
		AyyiIdx     idx;          // the object idx returned by the engine.
		char        err;          // zero if no errors reported.
		void*       ptr_1;
	} ret;
};

#endif //__ayyi_msg_h__
