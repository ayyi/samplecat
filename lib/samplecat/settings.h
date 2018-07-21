/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2018 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __settings_h__
#define __settings_h__
#ifdef USE_MYSQL
#include "db/mysql.h"
#endif

typedef struct _Config       Config;
typedef struct _ConfigOption ConfigOption;

struct _ConfigOption {
   char*   name;
   GValue  val;
   GValue  min;
   GValue  max;
   void    (*save)(ConfigOption*);
};

typedef struct {
   const char*       dir;
   char*             filename;
   GKeyFile*         key_file;  // loaded data.
   ConfigOption**    options;   // null terminated.
} ConfigContext;

typedef enum {
   CONFIG_WINDOW_WIDTH = 0,
   CONFIG_WINDOW_HEIGHT,
   CONFIG_ICON_THEME,
   CONFIG_COL1_WIDTH,
   CONFIG_MAX = 9
} ConfigOptionType;

struct _Config
{
	char      database_backend[64];
#ifdef USE_MYSQL
	SamplecatDBConfig mysql;
#endif
	char      auditioner[16];
	char      show_dir[PATH_MAX];
	char      window_width[8];
	char      window_height[8];
	char      colour[/*PALETTE_SIZE*/17][8];
	bool      add_recursive;
	bool      loop_playback;
	char      column_widths[4][8];
	char      browse_dir[PATH_MAX];
	char      show_player[8];
	char      show_waveform[8];
	char      show_spectrogram[8];
	char      jack_autoconnect[1024];
	char      jack_midiconnect[1024];
};

bool          config_load              (ConfigContext*, Config*);
bool          config_save              (ConfigContext*);

ConfigOption* config_option_new_int    (char* name, void (*save)(ConfigOption*), int min, int max);
ConfigOption* config_option_new_string (char* name, void (*save)(ConfigOption*));
ConfigOption* config_option_new_bool   (char* name, void (*save)(ConfigOption*));
ConfigOption* config_option_new_manual (void (*save)(ConfigOption*));


#endif
