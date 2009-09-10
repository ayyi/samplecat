#ifndef __file_manager_h__
#define __file_manager_h__
#include "rox/rox_global.h"
#include "rox/dir.h"
#include "file_manager/typedefs.h"
#include "file_manager/support.h"
#include "file_manager/vala/filemanager.h"
#include "filer.h"

Filer*     file_manager__init           ();
GtkWidget* file_manager__new_window     (const char* path);
void       file_manager__update_all     ();
void       file_manager__set_icon_theme (const char*);
AyyiFilemanager* file_manager__get_signaller();

#endif //__file_manager_h__
