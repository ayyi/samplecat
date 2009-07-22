
int         sqlite__connect();
void        sqlite__disconnect();

int         sqlite__insert(char*);
int         sqlite__add_row (int argc, char *argv[]);
gboolean    sqlite__search_iter_new(char* search, char* dir);
//MYSQL_ROW   db__search_iter_next(unsigned long** lengths);
void        sqlite__search_iter_free();
