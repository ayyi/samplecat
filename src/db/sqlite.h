
#define BACKEND_IS_SQLITE (backend.search_iter_new == sqlite__search_iter_new)

int      sqlite__connect          ();
void     sqlite__disconnect       ();

int      sqlite__insert           (Sample*);
gboolean sqlite__delete_row       (int id);
gboolean sqlite__file_exists      (const char*, int *id);
GList *  sqlite__filter_by_audio  (Sample *s);

gboolean sqlite__update_string    (int id, const char*, const char*);
gboolean sqlite__update_int       (int id, const char*, const long int);
gboolean sqlite__update_float     (int id, const char*, const float);
gboolean sqlite__update_blob      (int id, const char*, const guint8*, const guint);

#if 1 // deprecate soon
gboolean sqlite__update_colour    (int id, int colour);
gboolean sqlite__update_keywords  (int id, const char*);
gboolean sqlite__update_notes     (int id, const char*);
gboolean sqlite__update_ebur      (int id, const char*);
gboolean sqlite__update_pixbuf    (Sample*);
gboolean sqlite__update_online    (int id, gboolean, time_t);
gboolean sqlite__update_peaklevel (int id, float);
#endif

gboolean sqlite__search_iter_new  (char* search, char* dir, const char* category, int* n_results);
Sample * sqlite__search_iter_next (unsigned long** lengths);
void     sqlite__search_iter_free ();

void     sqlite__dir_iter_new     ();
char*    sqlite__dir_iter_next    ();
void     sqlite__dir_iter_free    ();
