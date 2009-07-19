
gboolean         tracker__init();

gboolean         tracker__search_iter_new(char* search, char* dir);
SamplecatResult* tracker__search_iter_next();
void             tracker__search_iter_free();
