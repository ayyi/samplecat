/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <gtk/gtk.h>

GtkWidget*  window_new             (GtkApplication*, gpointer);

#if 0
GtkWidget*  message_panel__add_msg (const gchar* msg, const gchar* stock_id);
#endif

#ifdef HAVE_FFTW3
void        show_spectrogram       (bool enable);
#endif
void        show_filemanager       (bool enable);
void        show_waveform          (bool enable);
void        show_player            (bool enable);
