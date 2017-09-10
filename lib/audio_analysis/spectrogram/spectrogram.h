/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

#ifndef __analysis_spectrogram_h__
#define __analysis_spectrogram_h__
#include <glib.h>

void get_spectrogram             (const char* path, gpointer callback, gpointer);
void get_spectrogram_with_target (const char* path, gpointer callback, void* target, gpointer);
void render_spectrogram          (const char* path, gpointer callback, gpointer);
void cancel_spectrogram          (const char* path);

#endif
