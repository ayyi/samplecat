
#define BACKEND_IS_SQLITE (backend.search_iter_new == sqlite__search_iter_new)

int              sqlite__connect          ();
void             sqlite__disconnect       ();

int              sqlite__insert           (sample*, MIME_type*);
gboolean         sqlite__update_colour    (int id, int colour);

gboolean         sqlite__search_iter_new  (char* search, char* dir);
SamplecatResult* sqlite__search_iter_next (unsigned long** lengths);
void             sqlite__search_iter_free ();
