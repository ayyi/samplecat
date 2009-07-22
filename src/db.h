
gboolean    db__connect();
gboolean    db__is_connected();
int         db__insert(char* qry);
gboolean    db__delete_row(int id);
int         mysql_exec_sql(MYSQL*, const char* create_definition);
int         db__update_path(const char* old_path, const char* new_path);

gboolean    db__search_iter_new(char* search, char* dir);
MYSQL_ROW   db__search_iter_next(unsigned long** lengths);
void        db__search_iter_free();

void        db__dir_iter_new();
char*       db__dir_iter_next();
void        db__dir_iter_free();

void        db__iter_to_result(SamplecatResult*);
void        db__add_row_to_model(MYSQL_ROW, unsigned long* lengths);
