
#define BACKEND_IS_MYSQL (backend.search_iter_new == mysql__search_iter_new)

gboolean    mysql__connect          ();
gboolean    mysql__is_connected     ();
int         mysql__insert           (sample*, MIME_type*);
gboolean    mysql__delete_row       (int id);
int         mysql__update_path      (const char* old_path, const char* new_path);
gboolean    mysql__update_colour    (int id, int colour);

gboolean    mysql__search_iter_new  (char* search, char* dir);
MYSQL_ROW   mysql__search_iter_next (unsigned long** lengths);
SamplecatResult* mysql__search_iter_next_ (unsigned long** lengths);
void        mysql__search_iter_free ();

void        mysql__dir_iter_new     ();
char*       mysql__dir_iter_next    ();
void        mysql__dir_iter_free    ();

void        mysql__iter_to_result   (SamplecatResult*);
void        mysql__add_row_to_model (MYSQL_ROW, unsigned long* lengths);
