typedef enum
{
	MSG_TYPE_OVERVIEW,
	MSG_TYPE_PEAKLEVEL,
	MSG_TYPE_EBUR128,
} MsgType;

struct _message
{
	MsgType type;
	Sample* sample;
};

gpointer    overview_thread        (gpointer data);
void        request_overview       (Sample*);
void        request_peaklevel      (Sample*);
void        request_ebur128        (Sample*);

