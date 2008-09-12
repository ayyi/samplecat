#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
//#include <jack/jack.h>
//#include <fst.h>
#include <gtk/gtk.h>
#include <ayyi/ayyi_utils.h>

#include <ayyi/ayyi_log.h>

struct _log app_log;


void
ayyi_log_init()
{
	app_log.txtbuf      = gtk_text_buffer_new(NULL);
	app_log.tag_green   = gtk_text_buffer_create_tag(app_log.txtbuf, "green_foreground", "foreground", "green", NULL);
	app_log.tag_red     = gtk_text_buffer_create_tag(app_log.txtbuf, "red_foreground",   "foreground", "red",   NULL);
	app_log.tag_orange  = gtk_text_buffer_create_tag(app_log.txtbuf, "orange_foreground","foreground", "orange",NULL);
	app_log.tag_yellow  = gtk_text_buffer_create_tag(app_log.txtbuf, "yellow_foreground","foreground", "yellow",NULL);
	app_log.print_to_ui = NULL;
}


void
log_append(const char* str, int type)
{
  //text is appended to the text buffer of each open log window.
  //text is also output to the arrange statusbar and to stdout

  GtkTextIter iter;

  GtkTextBuffer* buffer = app_log.txtbuf;

  if(type==LOG_WARN) log_print_warn();
  gtk_text_buffer_get_end_iter(buffer, &iter);    //find the end of the buffer.
  gtk_text_buffer_insert(buffer, &iter, str, -1);
  if(type==LOG_OK)   log_print_ok();
  if(type==LOG_FAIL) log_print_fail();
  gtk_text_buffer_get_end_iter(buffer, &iter);
  gtk_text_buffer_insert(buffer, &iter, "\n", -1);

  //output to gui statusbar:
  if(app_log.print_to_ui){
    char status[128];
    snprintf(status, 127, "%s%s", str, type==LOG_OK ? " ok" : /*type==LOG_FAIL ? " fail!" :*/ "");
    app_log.print_to_ui(status);
  }

  //output to stdout:
  if(type==LOG_WARN) printf("%s ", smwarn);
  printf(str);
  if(type==LOG_OK)   printf("\n%s %s", go_rhs, ok);
  if(type==LOG_FAIL) printf("\n%s%s", go_rhs, fail);
  printf("\n");
}


void
log_print_ok()
{
  //append coloured "[ ok ]" to the log textbuffer.

  GtkTextBuffer* buffer = app_log.txtbuf;
  GtkTextIter iter;
  gtk_text_buffer_get_end_iter(buffer, &iter);

  gtk_text_buffer_insert(buffer, &iter, " [ ", -1);
  gtk_text_buffer_insert_with_tags(buffer, &iter, " ok ", 4, app_log.tag_green, NULL);
  gtk_text_buffer_insert(buffer, &iter, " ] ", -1);
}


void
log_print_fail()
{
  GtkTextBuffer* buffer = app_log.txtbuf;
  GtkTextIter iter;
  gtk_text_buffer_get_end_iter(buffer, &iter);

  gtk_text_buffer_insert(buffer, &iter, " [ ", -1);
  gtk_text_buffer_insert_with_tags(buffer, &iter, " fail ", 6, app_log.tag_red, NULL);
  gtk_text_buffer_insert(buffer, &iter, " ] ", -1);
}


void
log_print_warn()
{
  GtkTextBuffer* buffer = app_log.txtbuf;
  GtkTextIter iter;
  gtk_text_buffer_get_end_iter(buffer, &iter);

  gtk_text_buffer_insert_with_tags(buffer, &iter, "warning: ", 9, app_log.tag_orange, NULL);
}


