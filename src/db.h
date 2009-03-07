
gboolean	db_connect();
gboolean    db_is_connected();
int         db_insert(char *qry);
gboolean    db_delete_row(int id);
int         mysql_exec_sql(MYSQL*, const char *create_definition);
int         db_update_path(const char* old_path, const char* new_path);

