#ifndef __ayyi_log_h__
#define __ayyi_log_h__
#include <gtk/gtk.h>

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
  void           (*print_to_ui)(const char*, int);
};

void        ayyi_log_init ();
void        log_print     (int type, const char* format, ...);
void        log_print_ok  ();
void        log_print_fail();
void        log_print_warn();
#endif
