#ifndef __db_mysql_h__
#define __db_mysql_h__
#include "mysql/mysql.h"

typedef struct _mysql_config SamplecatMysqlConfig;

#define BACKEND_IS_MYSQL (backend.search_iter_new == mysql__search_iter_new)

void      mysql__init             (SamplecatModel*, void* config);
gboolean  mysql__connect          ();
void      mysql__set_as_backend   (SamplecatBackend*);


struct _mysql_config
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
#endif //__db_mysql_h__
