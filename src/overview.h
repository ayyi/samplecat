typedef enum
{
	MSG_TYPE_OVERVIEW,
	MSG_TYPE_PEAKLEVEL,
} MsgType;

struct _message
{
	MsgType type;
	Sample* sample;
};

gpointer    overview_thread        (gpointer data);
void        request_overview       (Sample*);
void        request_peaklevel      (Sample*);

