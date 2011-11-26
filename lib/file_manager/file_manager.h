#ifndef __file_manager_h__
#define __file_manager_h__
#include <gtk/gtk.h>
#include "rox_global.h"
#include "dir.h"
#include "file_manager/typedefs.h"
#include "file_manager/support.h"
#include "filer.h"
#include "file_manager/vala/filemanager.h"
#include "file_manager/filetypes/filetype_plugin.h"
#include "file_view.h"

Filer*              file_manager__get            ();
GtkWidget*          file_manager__new_window     (const char* path);
void                file_manager__update_all     ();
void                file_manager__on_dir_changed ();
AyyiLibfilemanager* file_manager__get_signaller  ();

#endif //__file_manager_h__
