
#define BACKEND_IS_TRACKER (backend.search_iter_new == tracker__search_iter_new)

gboolean         tracker__init             ();
gboolean         tracker__update_pixbuf    (sample*);

int              tracker__insert           (sample*, MIME_type*);
gboolean         tracker__delete_row       (int id);

gboolean         tracker__search_iter_new  (char* search, char* dir, int* n_results);
SamplecatResult* tracker__search_iter_next ();
void             tracker__search_iter_free ();

gboolean         tracker__update_colour    (int id, int colour);
gboolean         tracker__update_online    (int id, gboolean);
