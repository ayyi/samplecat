
AyyiAction* ayyi_action_new          (char *format, ...);

int         action_set_length        (AyyiAction*, uint32_t _obj_id, uint64_t _len);
int         ayyi_set_float           (AyyiAction*, uint32_t object_type, uint32_t _obj_id, int _prop, double _val);
int         engine_set_obj           (AyyiAction*, uint32_t object_type, uint32_t _obj_id, int _prop, uint64_t _val);
int         engine_set_string        (AyyiAction*, uint32_t object_type, uint32_t _obj_id, const char* string);


