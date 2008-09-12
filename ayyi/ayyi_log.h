
enum {
  LOG_OK = 1,
  LOG_FAIL,
  LOG_WARN,
};

struct _log
{
  GtkTextBuffer* txtbuf;
  GtkTextTag*    tag_green;
  GtkTextTag*    tag_red;
  GtkTextTag*    tag_orange;
  GtkTextTag*    tag_yellow;
  void           (*print_to_ui)(const char*);
};

void        ayyi_log_init ();
void        log_append    (const char* str, int type);
void        log_print_ok  ();
void        log_print_fail();
void        log_print_warn();
