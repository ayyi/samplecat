#ifndef __SAMPLECAT_TYPES_H_
#define __SAMPLECAT_TYPES_H_
#include <gtk/gtk.h>
#include "typedefs.h"

struct _menu_def
{
	char*                label;
	GCallback            callback;
	char*                stock_id;
	gboolean             sensitive;
};

struct _inspector
{
	unsigned       row_id;
	GtkTreeRowReference* row_ref;
	GtkWidget*     widget;     //scrollwin
	GtkWidget*     vbox;
	GtkWidget*     filename;
	GtkWidget*     tags;
	GtkWidget*     tags_ev;    //event box for mouse clicks
	GtkWidget*     length;
	GtkWidget*     samplerate;
	GtkWidget*     channels;
	GtkWidget*     mimetype;
	GtkWidget*     level;
	GtkWidget*     bitrate;
	GtkWidget*     bitdepth;
	GtkWidget*     metadata;
	GtkWidget*     ebur;
	GtkWidget*     text;
	GtkTextBuffer* notes;
	GtkWidget*     edit;
	int            min_height;
	int            user_height;
	gboolean       show_waveform;
	InspectorPriv* priv;
};

struct _playctrl
{
	GtkWidget*     widget;

	GtkWidget*     slider1; // player position
	GtkWidget*     slider2; // player pitch
	GtkWidget*     slider3; // player speed
	GtkWidget*     cbfx;    // player enable FX
	GtkWidget*     cblnk;   // link speed/pitch
	GtkWidget*     pbctrl;  // playback control box (pause/stop)
	GtkWidget*     pbpause; // playback pause button
};

struct _auditioner {
	int     (*check)();
	void    (*connect)();
	void    (*disconnect)();
	void    (*play_path)(const char *);
	void    (*play)(Sample *);
	void    (*toggle)(Sample *);
	void    (*play_all)();
	void    (*stop)();
/* extended API */
	void    (*play_selected)();
	int     (*playpause)(int);
	void    (*seek)(double);
	double  (*status)();
};

#endif
