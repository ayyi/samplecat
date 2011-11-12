
GtkWidget* inspector_new();
void       inspector_free(Inspector*);
void       inspector_update_from_result(Sample*);
void       inspector_set_labels(Sample* sample);
void       inspector_update_from_fileview(GtkTreeView*);

