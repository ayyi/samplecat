#pragma once

#include "mysql/mysql.h"
#include "db.h"

#define BACKEND_IS_MYSQL (backend.search_iter_new == mysql__search_iter_new)

gboolean  mysql__connect          ();
void      mysql__set_as_backend   (SamplecatBackend*, SamplecatDBConfig*);


struct _SamplecatDBConfig
{
	char      host[64];
	char      user[64];
	char      pass[64];
	char      name[64];
};


#if NEVER
gboolean  mysql__is_connected     ();
int       mysql__update_path      (const char* old_path, const char* new_path);
//void      mysql__iter_to_result   (Sample*);
#endif
