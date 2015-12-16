
/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2015-2016 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __glx_utils_h__
#define __glx_utils_h__

void      glx_init               (Display*);
void      make_window            (Display*, const char* name, int x, int y, int width, int height, GLboolean fullscreen, Window* winRet, GLXContext* ctxRet);
void      make_extension_table   (const char*);
GLboolean is_extension_supported (const char*);
void      show_refresh_rate      (Display*);
void      on_window_resize       (int width, int height);
void      event_loop             (Display*, Window);

#endif
