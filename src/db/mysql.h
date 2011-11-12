#include "mysql/mysql.h"

#define BACKEND_IS_MYSQL (backend.search_iter_new == mysql__search_iter_new)

gboolean  mysql__connect          ();
gboolean  mysql__is_connected     ();
void      mysql__disconnect       ();
int       mysql__insert           (Sample*);
gboolean  mysql__delete_row       (int id);
//void    mysql__delete_row_tags  ();
int       mysql__update_path      (const char* old_path, const char* new_path);
gboolean  mysql__update_colour    (int id, int colour);
gboolean  mysql__update_keywords  (int id, const char*);
gboolean  mysql__update_notes     (int id, const char*);
gboolean  mysql__update_misc      (int id, const char*);
gboolean  mysql__update_pixbuf    (Sample*);
gboolean  mysql__update_online    (int id, gboolean);
gboolean  mysql__update_peaklevel (int id, float);

gboolean  mysql__search_iter_new   (char* search, char* dir, const char* category, int* n_results);
MYSQL_ROW mysql__search_iter_next  (unsigned long** lengths);
Sample *  mysql__search_iter_next_ (unsigned long** lengths);
void      mysql__search_iter_free  ();

void      mysql__dir_iter_new     ();
char*     mysql__dir_iter_next    ();
void      mysql__dir_iter_free    ();

void      mysql__iter_to_result   (Sample*);
void      mysql__add_row_to_model (MYSQL_ROW, unsigned long* lengths);
