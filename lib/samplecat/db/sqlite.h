
#define BACKEND_IS_SQLITE (backend.search_iter_new == sqlite__search_iter_new)

void     sqlite__set_as_backend   (SamplecatBackend*);
int      sqlite__connect          ();
GList*   sqlite__filter_by_audio  (Sample *s);

#if 1 // deprecate soon
gboolean sqlite__update_colour    (int id, int colour);
gboolean sqlite__update_keywords  (int id, const char*);
gboolean sqlite__update_notes     (int id, const char*);
gboolean sqlite__update_ebur      (int id, const char*);
gboolean sqlite__update_pixbuf    (Sample*);
gboolean sqlite__update_online    (int id, gboolean, time_t);
gboolean sqlite__update_peaklevel (int id, float);
#endif

