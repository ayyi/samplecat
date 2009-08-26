#ifndef __file_manager_h__
#define __file_manager_h__
#include "rox/rox_global.h"
#include "rox/dir.h"
#include "file_manager/typedefs.h"
#include "filer.h"

Filer*     file_manager__init();
GtkWidget* file_manager__new_window(const char* path);
void       file_manager__update_all();

#endif //__file_manager_h__
