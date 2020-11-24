#ifndef __ayyi_utils_h__
#define __ayyi_utils_h__

#include "glib.h"

#ifndef true
#define true TRUE
#define false FALSE
#endif

#define UNDERLINE printf("-----------------------------------------------------\n")
#define call(FN, A, ...) if(FN) (FN)(A, ##__VA_ARGS__)
#define BITS_PER_CHAR_8 8

void        warn_gerror              (const char* msg, GError**);
void        info_gerror              (const char* msg, GError**);

gchar*      path_from_utf8           (const gchar* utf8);

void        string_increment_suffix  (char* newstr, const char* orig, int new_max);

int         get_terminal_width       ();

gchar*      audio_path_get_base      (const char*);
gboolean    audio_path_get_wav_leaf  (char* leaf, const char* path, int len);
char*       audio_path_truncate      (char*, char);

#ifdef __ayyi_utils_c__
#define RED "\x1b[1;31m"
#define WHITE "\x1b[0;39m"
#define YELLOW "\x1b[1;33m"

char err      [32] = RED"error"WHITE;
char warn     [32] = YELLOW"warning:"WHITE;

char white    [10] = WHITE;        // 0 = normal
char bold     [10] = "\x1b[1;39m"; // 1 = bright
char grey     [10] = "\x1b[2;39m"; // 2 = dim
char yellow   [10] = YELLOW;
char yellow_r [10] = "\x1b[30;43m";
char white__r [10] = "\x1b[30;47m";
char cyan___r [10] = "\x1b[30;46m";
char magent_r [10] = "\x1b[30;45m";
char blue_r   [10] = "\x1b[30;44m";
char red      [10] = RED;
char red_r    [10] = "\x1b[30;41m";
char green    [10] = "\x1b[1;32m";
char green_r  [10] = "\x1b[30;42m";
char go_rhs   [32] = "\x1b[A\x1b[50C"; //go up one line, then goto column 60
char ok       [32] = " [ \x1b[1;32mok\x1b[0;39m ]";
char fail     [32] = " [\x1b[1;31mFAIL\x1b[0;39m]";
#else
extern char err      [32];
extern char warn     [32];
extern char white    [10];
extern char bold     [10];
extern char grey     [10];
extern char yellow   [10];
extern char yellow_r [10];
extern char white__r [10];
extern char cyan___r [10];
extern char magent_r [10];
extern char blue_r   [10];
extern char red      [10];
extern char red_r    [10];
extern char green    [10];
extern char green_r  [10];
extern char go_rhs   [32];
extern char ok       [];
extern char fail     [];
#endif

#endif //__ayyi_utils_h__
