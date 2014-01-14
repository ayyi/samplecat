#ifndef __ayyi_utils_h__
#define __ayyi_utils_h__

#include "glib.h"

#ifndef true
#define true TRUE
#define false FALSE
#endif

#define UNDERLINE printf("-----------------------------------------------------\n")
#define call(FN, A, ...) if(FN) (FN)(A, ##__VA_ARGS__)
#define IDLE_STOP FALSE
#define BITS_PER_CHAR_8 8

void        warn_gerror              (const char* msg, GError**);
void        info_gerror              (const char* msg, GError**);

gchar*      path_from_utf8           (const gchar* utf8);

GList*      get_dirlist              (const char*);

void        string_increment_suffix  (char* newstr, const char* orig, int new_max);

int         get_terminal_width       ();

gchar*      audio_path_get_base      (const char*);
gboolean    audio_path_get_wav_leaf  (char* leaf, const char* path, int len);
char*       audio_path_truncate      (char*, char);

char err [32];
char warn[32];

#ifdef __ayyi_utils_c__
char white    [16] = "\x1b[0;39m"; // 0 = normal
char bold     [16] = "\x1b[1;39m"; // 1 = bright
char grey     [16] = "\x1b[2;39m"; // 2 = dim
char yellow   [16] = "\x1b[1;33m";
char yellow_r [16] = "\x1b[30;43m";
char white__r [16] = "\x1b[30;47m";
char cyan___r [16] = "\x1b[30;46m";
char magent_r [16] = "\x1b[30;45m";
char blue_r   [16] = "\x1b[30;44m";
char red      [16] = "\x1b[1;31m";
char red_r    [16] = "\x1b[30;41m";
char green    [16] = "\x1b[1;32m";
char green_r  [16] = "\x1b[30;42m";
char go_rhs   [32] = "\x1b[A\x1b[50C"; //go up one line, then goto column 60
char ok       [32] = " [ \x1b[1;32mok\x1b[0;39m ]";
char fail     [32] = " [\x1b[1;31mFAIL\x1b[0;39m]";
#else
extern char white    [16];
extern char bold     [16];
extern char grey     [16];
extern char yellow   [16];
extern char yellow_r [16];
extern char white__r [16];
extern char cyan___r [16];
extern char magent_r [16];
extern char blue_r   [16];
extern char red      [16];
extern char red_r    [16];
extern char green    [16];
extern char green_r  [16];
extern char go_rhs   [32];
extern char ok       [];
extern char fail     [];
#endif

#endif //__ayyi_utils_h__
