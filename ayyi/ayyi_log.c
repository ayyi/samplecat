/*
  This file is part of the Ayyi Project. http://ayyi.org
  copyright (C) 2004-2010 Tim Orford <tim@orford.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 3
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include "config.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include "ayyi/ayyi_typedefs.h"
#include "ayyi/ayyi_types.h"
#include "ayyi/ayyi_utils.h"
#include "ayyi/ayyi_client.h"

#include <ayyi/ayyi_log.h>

static void   log_append    (const char* str, int type);


void
ayyi_log_init()
{
	ayyi.log.txtbuf      = gtk_text_buffer_new(NULL);
	ayyi.log.tag_green   = gtk_text_buffer_create_tag(ayyi.log.txtbuf, "green_foreground", "foreground", "green", NULL);
	ayyi.log.tag_red     = gtk_text_buffer_create_tag(ayyi.log.txtbuf, "red_foreground",   "foreground", "red",   NULL);
	ayyi.log.tag_orange  = gtk_text_buffer_create_tag(ayyi.log.txtbuf, "orange_foreground","foreground", "orange",NULL);
	ayyi.log.tag_yellow  = gtk_text_buffer_create_tag(ayyi.log.txtbuf, "yellow_foreground","foreground", "yellow",NULL);
	ayyi.log.print_to_ui = NULL;
}


static void
log_append(const char* str, int type)
{
	//text is appended to the text buffer of each open log window.
	//text is also output to stdout and (if print_to_ui is set) the gui (statusbar or other notification).

	GtkTextIter iter;

	GtkTextBuffer* buffer = ayyi.log.txtbuf;

	if(type==LOG_WARN) log_print_warn();
	gtk_text_buffer_get_end_iter(buffer, &iter);    //find the end of the buffer.
	gtk_text_buffer_insert(buffer, &iter, str, -1);
	if(type==LOG_OK)   log_print_ok();
	if(type==LOG_FAIL) log_print_fail();
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);

	//output to gui statusbar:
	if(ayyi.log.print_to_ui){
		char* status = g_strdup_printf("%s%s", str, type==LOG_OK ? " ok" : /*type==LOG_FAIL ? " fail!" :*/ "");
		ayyi.log.print_to_ui(status, type);
		g_free(status);
	}

	//output to stdout:
	if(ayyi.log.to_stdout){
		char goto_rhs[32];
		sprintf(goto_rhs, "\x1b[A\x1b[%iC", MIN(get_terminal_width() - 12, 80)); //go up one line, then goto rhs

		if(type==LOG_WARN) printf("%s ", ayyi_warn);
		if(!type) printf("%s", bold);
		printf("%s", str);
		if(!type) printf("%s", white);
		if(type==LOG_OK)   printf("\n%s%s", goto_rhs, ok);
		if(type==LOG_FAIL) printf("\n%s%s", goto_rhs, fail);
		printf("\n");
	}
}


void 
log_print(int type, const char* format, ...)
{
	char str[256];

	va_list argp;           //points to each unnamed arg in turn
	va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
	vsprintf(str, format, argp);
	va_end(argp);           //clean up

	log_append(str, type);
}


void
log_print_ok()
{
	//append coloured "[ ok ]" to the log textbuffer.

	GtkTextBuffer* buffer = ayyi.log.txtbuf;
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(buffer, &iter);

	gtk_text_buffer_insert(buffer, &iter, " [ ", -1);
	gtk_text_buffer_insert_with_tags(buffer, &iter, " ok ", 4, ayyi.log.tag_green, NULL);
	gtk_text_buffer_insert(buffer, &iter, " ] ", -1);
}


void
log_print_fail()
{
	GtkTextBuffer* buffer = ayyi.log.txtbuf;
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(buffer, &iter);

	gtk_text_buffer_insert(buffer, &iter, " [ ", -1);
	gtk_text_buffer_insert_with_tags(buffer, &iter, " fail ", 6, ayyi.log.tag_red, NULL);
	gtk_text_buffer_insert(buffer, &iter, " ] ", -1);
}


void
log_print_warn()
{
	GtkTextBuffer* buffer = ayyi.log.txtbuf;
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(buffer, &iter);

	gtk_text_buffer_insert_with_tags(buffer, &iter, "warning: ", 9, ayyi.log.tag_orange, NULL);
}


