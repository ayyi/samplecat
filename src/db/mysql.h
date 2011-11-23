#include "mysql/mysql.h"

#define BACKEND_IS_MYSQL (backend.search_iter_new == mysql__search_iter_new)

gboolean  mysql__connect          ();
void      mysql__disconnect       ();

int       mysql__insert           (Sample*);
gboolean  mysql__delete_row       (int id);
gboolean  mysql__file_exists      (const char*, int *id);
GList *   mysql__filter_by_audio  (Sample *s);

gboolean  mysql__update_string    (int id, const char*, const char*);
gboolean  mysql__update_int       (int id, const char*, const long int);
gboolean  mysql__update_float     (int id, const char*, float);
gboolean  mysql__update_blob      (int id, const char*, const guint8*, const guint);


gboolean  mysql__search_iter_new   (char* search, char* dir, const char* category, int* n_results);
Sample *  mysql__search_iter_next_ (unsigned long** lengths);
void      mysql__search_iter_free  ();

void      mysql__dir_iter_new     ();
char*     mysql__dir_iter_next    ();
void      mysql__dir_iter_free    ();

#if NEVER
gboolean  mysql__is_connected     ();
int       mysql__update_path      (const char* old_path, const char* new_path);
//void      mysql__iter_to_result   (Sample*);
#endif
