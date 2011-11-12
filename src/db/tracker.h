
#define BACKEND_IS_TRACKER (backend.search_iter_new == tracker__search_iter_new)

gboolean tracker__init             ();
void     tracker__disconnect       ();

gboolean tracker__update_ignore    (Sample*);

int      tracker__insert           (Sample*);
gboolean tracker__delete_row       (int id);

gboolean tracker__search_iter_new  (char* search, char* dir, const char* category, int* n_results);
Sample * tracker__search_iter_next ();
void     tracker__search_iter_free ();

void     tracker__dir_iter_new     ();
char*    tracker__dir_iter_next    ();
void     tracker__dir_iter_free    ();

gboolean tracker__update_colour    (int id, int colour);
gboolean tracker__update_keywords  (int id, const char*);
gboolean tracker__update_online    (int id, gboolean);
gboolean tracker__update_peaklevel (int id, float);
