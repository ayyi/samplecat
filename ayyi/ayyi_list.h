#ifdef __cplusplus
extern "C" {
#endif

int       ayyi_list__length     (AyyiList*);
AyyiList* ayyi_list__first      (AyyiList*);
AyyiList* ayyi_list__next       (AyyiList*);
void*     ayyi_list__first_data (AyyiList**);
void*     ayyi_list__next_data  (AyyiList**);
AyyiList* ayyi_list__find       (AyyiList*, uint32_t);

#ifdef __cplusplus
}
#endif
