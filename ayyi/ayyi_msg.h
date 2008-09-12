#ifndef __ayyi_msg_h__
#define __ayyi_msg_h__

struct _ayyi_action
{
	unsigned       trid;

	int            obj_type;
	AyyiOp         op;

	void*          app_data;

	//return values:
	int            gid;           // the object id number returned by the engine following the request.
	int            idx;           // the object idx returned by the engine.
	char           err;           // zero if no errors reported.
	void*          return_val_1;
};

#endif //__ayyi_msg_h__
