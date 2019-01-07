/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2015-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __glx_utils_h__
#define __glx_utils_h__

typedef struct {
    Window    window;
    AGlScene* scene;
} AGlWindow;

AGlWindow* agl_make_window        (Display*, const char* name, int x, int y, int width, int height, AGlScene*);
void       agl_window_destroy     (Display*, AGlWindow**);

void       on_window_resize       (Display*, AGlWindow*, int width, int height);
GLboolean  is_extension_supported (const char*);
void       show_refresh_rate      (Display*);

#undef USE_GLIB_LOOP
#ifdef USE_GLIB_LOOP
GMainLoop* main_loop_new          (Display*, Window);
#else
void       event_loop             (Display*);
#endif

#endif
