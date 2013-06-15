/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://samplecat.orford.org          |
* | copyright (C) 2007-2013 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "typedefs.h"
#include "support.h"
#include "main.h"

void file_manager__update_all();

extern struct _app app;
extern int debug;


void
observer__files_moved(GList* file_list, const char* dest)
{
	PF;
	GList* l = file_list;
	for(;l;l=l->next){
		dbg(0, "%s", l->data);
	}

	char msg[256];
	if(g_list_length(file_list) > 1){
		snprintf(msg, 255, "%i files moved.", g_list_length(file_list));
	}else{
		snprintf(msg, 255, "file: '%s' moved.", (char*)file_list->data);
	}
	statusbar_print(1, msg);

	//FIXME
	//db_update_path(const char* old_path, const char* new_path);
}


