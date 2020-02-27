/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://ayyi.org               |
* | copyright (C) 2011-2018 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | ROX-Filer, filer for the ROX desktop project, v2.3                   |
* | Copyright (C) 2005, the ROX-Filer team.                              |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __mimetype_h__
#define __mimetype_h__

#include <glib.h>
#include "file_manager/typedefs.h"

extern char theme_name[];
#ifdef __GTK_H__
extern GtkIconTheme* icon_theme;
#endif

extern MIME_type* text_plain;		// often used as a default type
extern MIME_type* application_octet_stream;
extern MIME_type* application_x_shellscript;
extern MIME_type* application_executable;
extern MIME_type* application_x_desktop;
extern MIME_type* inode_directory;
/*
extern MIME_type *inode_mountpoint;
extern MIME_type *inode_pipe;
extern MIME_type *inode_socket;
extern MIME_type *inode_block_dev;
extern MIME_type *inode_char_dev;
extern MIME_type *inode_unknown;
extern MIME_type *inode_door;
*/


struct _MIME_type
{
    char*          media_type;
    char*          subtype;
    MaskedPixmap*  image;      // NULL => not loaded yet
    time_t         image_time; // When we loaded the image

    gboolean       executable; // Subclass of application/x-executable
};


void               type_init                (void);
MIME_type*         type_get_type            (const guchar* path);
MIME_type*         mime_type_lookup         (const char* type);
MIME_type*         type_from_path           (const char* path);

#ifdef GDK_PIXBUF_H
GdkPixbuf*         mime_type_get_pixbuf     (MIME_type*);
#endif
gboolean           type_open                (const char* path, MIME_type*);
MaskedPixmap*      type_to_icon             (MIME_type*);
#ifdef __GDK_TYPES_H__
GdkAtom            type_to_atom             (MIME_type*);
#endif
MIME_type*         mime_type_from_base_type (int base_type);
int                mode_to_base_type        (int st_mode);
void               type_set_handler_dialog  (MIME_type*);
gchar*             describe_current_command (MIME_type*);
void               reread_mime_files        (void);
void               mime_type_clear          ();
GList*             mime_type_name_list      (void);

#define EXECUTABLE_FILE(item) ((item)->mime_type && (item)->mime_type->executable && \
				((item)->flags & ITEM_FLAG_EXEC_FILE))

#endif


