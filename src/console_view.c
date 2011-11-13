#include <stdio.h>
#include <string.h>

#include "main.h"
#include "support.h"
#include "sample.h"
#include "console_view.h"

//extern struct _app app; // defined in main.h

void
console__show_result_header()
{
	printf("filters: text='%s' dir=%s\n", app.search_phrase, strlen(app.search_dir) ? app.search_dir : "<all directories>");

	printf("  name                 directory                            length ch rate mimetype\n");
}


void
console__show_result(Sample* result)
{
	#define DIR_MAX (35)
	char dir[DIR_MAX];
	strncpy(dir, dir_format(result->dir), DIR_MAX-1);
	dir[DIR_MAX-1] = '\0';

	#define SNAME_MAX (20)
	char name[SNAME_MAX];
	strncpy(name, result->sample_name, SNAME_MAX-1);
	name[SNAME_MAX-1] = '\0';

	printf("  %-20s %-35s %7"PRIi64" %d %5d %s\n", name, dir, result->length, result->channels, result->sample_rate, result->mimetype);
}


void
console__show_result_footer(int row_count)
{
	printf("total %i samples found.\n", row_count);
}

#ifdef NEVER
#include <sys/ioctl.h>
static int
get_terminal_width()
{
	struct winsize ws;

	if(ioctl(1, TIOCGWINSZ, (void *)&ws) != 0) printf("ioctl failed\n");

	dbg(1, "terminal width = %d\n", ws.ws_col);

	return ws.ws_col;
}
#endif
