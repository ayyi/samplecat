/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2013-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "agl/actor.h"

/*
 *  Common code for all tests
 */

void  set_log_handlers   ();
char* find_wav           (const char*);
const char* find_data_dir();

#if 0
typedef void (KeyHandler)(gpointer);

typedef struct
{
	int         key;
	KeyHandler* handler;
} Key;

typedef struct
{
	guint          timer;
	KeyHandler*    handler;
} KeyHold;
#endif


#ifdef __common2_c__
char grey     [16] = "\x1b[2;39m"; // 2 = dim
char yellow   [16] = "\x1b[1;33m";
char yellow_r [16] = "\x1b[30;43m";
char white__r [16] = "\x1b[30;47m";
char cyan___r [16] = "\x1b[30;46m";
char magent_r [16] = "\x1b[30;45m";
char blue_r   [16] = "\x1b[30;44m";
char red      [10] = "\x1b[1;31m";
char red_r    [16] = "\x1b[30;41m";
char green    [10] = "\x1b[1;32m";
char green_r  [16] = "\x1b[30;42m";
char go_rhs   [32] = "\x1b[A\x1b[50C"; //go up one line, then goto column 60
char ok       [32] = " [ \x1b[1;32mok\x1b[0;39m ]";
char fail     [32] = " [\x1b[1;31mFAIL\x1b[0;39m]";
#else
extern char grey     [16];
extern char yellow   [16];
extern char yellow_r [16];
extern char white__r [16];
extern char cyan___r [16];
extern char magent_r [16];
extern char blue_r   [16];
extern char red      [10];
extern char red_r    [16];
extern char green    [10];
extern char green_r  [16];
extern char ayyi_warn[32];
extern char ayyi_err [32];
extern char go_rhs   [32];
extern char ok       [];
extern char fail     [];
#endif

void add_key_handlers         (Key keys[]);
#ifdef __GTK_H__
void add_key_handlers_gtk     (GtkWindow*, gpointer, Key keys[]);

#ifdef __GDK_GL_CONFIG_H__
typedef void (*WindowFn)      (GtkWindow*, GdkGLConfig*);

int  gtk_window               (Key keys[], WindowFn);
#endif
#endif

#define g_source_remove0(S) {if(S) g_source_remove(S); S = 0;}
