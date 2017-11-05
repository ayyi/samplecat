/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://www.ayyi.org           |
* | copyright (C) 2004-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

#define FILETYPE_PLUGIN_API_VERSION 1

typedef struct filetypePlugin {
    guint       api_version;
    gchar*      name;
    guint       type;

    void        (*init)    ();
    void        (*deinit)  ();

    //void*       client_data;
} *FileTypePluginPtr;

#include <gmodule.h>
#define DECLARE_FILETYPE_PLUGIN(plugin) \
	G_MODULE_EXPORT FileTypePluginPtr plugin_get_info() { \
		return &plugin; \
	}

