#ifndef __db_db_h__
#define __db_db_h__
#include "gdk/gdk.h"

struct _db_config
{
};

struct _backend
{
	gboolean         pending;

	void             (*init)             (SamplecatModel*, void* config);

	gboolean         (*search_iter_new)  (char* dir, const char* category, int* n_results);
	Sample*          (*search_iter_next) (unsigned long**);
	void             (*search_iter_free) ();

	void             (*dir_iter_new)     ();
	char*            (*dir_iter_next)    ();
	void             (*dir_iter_free)    ();

	int              (*insert)           (Sample*);
	gboolean         (*remove)           (int);
	gboolean         (*file_exists)      (const char*, int*);
	GList*           (*filter_by_audio)  (Sample*);

	gboolean         (*update_string)    (int, const char*, const char*);
	gboolean         (*update_int)       (int, const char*, const long int);
	gboolean         (*update_float)     (int, const char*, const float);
	gboolean         (*update_blob)      (int, const char*, const guint8*, const guint);

	void             (*disconnect)       ();

	int              n_results;
};

#define BACKEND_IS_NULL (backend.search_iter_new == NULL)

gboolean   db_connect             ();
gboolean   samplecat_set_backend  (BackendType);

GdkPixbuf* blob_to_pixbuf         (const unsigned char* blob, guint len);

#ifdef USE_MYSQL
#include "db/mysql.h"
#endif
#endif //__db_db_h__
