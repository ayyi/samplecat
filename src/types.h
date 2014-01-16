/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2014 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __samplecat_types_h__
#define __samplecat_types_h__
#include <gtk/gtk.h>
#include "typedefs.h"
#ifdef USE_MYSQL
#include "db/mysql.h"
#endif

#define PALETTE_SIZE 17

struct _config
{
	char      database_backend[64];
#ifdef USE_MYSQL
	struct _mysql_config mysql;
#endif
	char      auditioner[16];
	char      show_dir[PATH_MAX];
	char      window_width[8];
	char      window_height[8];
	char      colour[PALETTE_SIZE][8];
	//gboolean  add_recursive; ///< TODO save w/ config ?
	//gboolean  loop_playback; ///< TODO save w/ config ?
	char      column_widths[4][8];
	char      browse_dir[PATH_MAX];
	char      show_player[8];
	char      show_waveform[8];
	char      show_spectrogram[8];
	char      jack_autoconnect[1024];
	char      jack_midiconnect[1024];
};

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

struct _palette {
	guint red[8];
	guint grn[8];
	guint blu[8];
};

#define HOMOGENOUS 1
#define NON_HOMOGENOUS 0

#endif
