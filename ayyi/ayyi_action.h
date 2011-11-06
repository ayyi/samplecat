#ifndef __ayyi_action_h__
#define __ayyi_action_h__

struct _ayyi_action
{
	unsigned           id;
	struct timeval     time_stamp;
	char               label[64];     // a description of the transaction. For printing only.

	AyyiObjType        obj_type;
	AyyiObjIdx         obj_idx;
	AyyiOp             op;
	int                prop;
	int                i_val;
	double             d_val;
	union {
		int            i;
		double         d;
	}                  val2;

	AyyiActionCallback callback;  // the func to call upon reception of succesful completion of the transaction
	void*              app_data;  // data to forward with the callback

	GList*             pending;   // list of actions that must be completed before this one is started.

	//return values:
	struct _r {
		int            gid;       // the object id number returned by the engine following the request.
		AyyiIdx        idx;       // the object idx returned by the engine.
		GError*        error;     // 
		void*          ptr_1;
	} ret;
};

AyyiAction* ayyi_action_new          (char* format, ...);
AyyiAction* ayyi_action_new_pending  (GList*);
void        ayyi_action_free         (AyyiAction*);
void        ayyi_action_execute      (AyyiAction*);
void        ayyi_action_complete     (AyyiAction*);
AyyiAction* ayyi_action_lookup_by_id (unsigned id);

void        ayyi_print_action_status ();

int         ayyi_set_length          (AyyiAction*, uint64_t _len);
int         ayyi_set_float           (AyyiAction*, uint32_t object_type, AyyiIdx, int _prop, double _val);
void        ayyi_set_obj             (AyyiAction*, AyyiObjType, uint32_t _obj_id, int _prop, uint64_t _val);
int         ayyi_set_string          (AyyiAction*, uint32_t object_type, uint32_t _obj_id, const char* string);

HandlerData*    ayyi_handler_data_new           (AyyiHandler, gpointer);
void            ayyi_handler_simple             (AyyiAction*);
void            ayyi_handler_simple2            (AyyiIdent, GError**, gpointer);

#endif //__ayyi_action_h__
