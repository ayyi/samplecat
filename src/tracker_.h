
gboolean    tracker__init();
void        tracker__search();

gboolean    tracker__search_iter_new(char* search, char* dir);
//MYSQL_ROW   tracker__search_iter_next(unsigned long** lengths);
void        tracker__search_iter_free();
