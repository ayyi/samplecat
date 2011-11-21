
#define BACKEND_IS_TRACKER (backend.search_iter_new == tracker__search_iter_new)

gboolean tracker__init             ();
void     tracker__disconnect       ();

int      tracker__insert           (Sample*);
gboolean tracker__delete_row       (int id);
gboolean tracker__file_exists      (const char*);

gboolean tracker__search_iter_new  (char* search, char* dir, const char* category, int* n_results);
Sample * tracker__search_iter_next (unsigned long** lengths);
void     tracker__search_iter_free ();

void     tracker__dir_iter_new     ();
char*    tracker__dir_iter_next    ();
void     tracker__dir_iter_free    ();


gboolean tracker__update_string    (int id, const char*, const char*);
gboolean tracker__update_int       (int id, const char*, const long int);
gboolean tracker__update_float     (int id, const char*, const float);
gboolean tracker__update_blob      (int id, const char*, const guint8*, const guint);
