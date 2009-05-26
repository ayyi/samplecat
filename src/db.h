
gboolean    db__connect();
gboolean    db__is_connected();
int         db__insert(char *qry);
gboolean    db_delete_row(int id);
int         mysql_exec_sql(MYSQL*, const char *create_definition);
int         db_update_path(const char* old_path, const char* new_path);

void        db__dir_iter_new();
char*       db__dir_iter_next();
void        db__dir_iter_free();

