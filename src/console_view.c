#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <gtk/gtk.h>
#include "src/typedefs.h"
#include "src/types.h"
#include "support.h"
#include "main.h"
#include "console_view.h"

extern struct _app app;

static int get_terminal_width();


void
console__show_result_header()
{
	int terminal_width; terminal_width = get_terminal_width();

	printf("filters: text='%s' dir=%s\n", app.search_phrase, strlen(app.search_dir) ? app.search_dir : "<all directories>");

	printf("  name                 directory                            length ch rate mimetype\n");
}


void
console__show_result(SamplecatResult* result)
{
	#define DIR_MAX 35
	char dir[DIR_MAX];
	strncpy(dir, dir_format(result->dir), DIR_MAX-1);
	dir[DIR_MAX-1] = '\0';

	#define SNAME_MAX 20
	char name[SNAME_MAX];
	snprintf(name, SNAME_MAX-1, result->sample_name);
	name[SNAME_MAX-1] = '\0';

	printf("  %-20s %-35s %7i %i %5i %s\n", name, dir, result->length, result->channels, result->sample_rate, result->mimetype);
}


void
console__show_result_footer(int row_count)
{
	printf("total %i samples found.\n", row_count);
}


static int
get_terminal_width()
{
	struct winsize ws;

	if(ioctl(1, TIOCGWINSZ, (void *)&ws) != 0) printf("ioctl failed\n");

	dbg(1, "terminal width = %d\n", ws.ws_col);

	return ws.ws_col;
}


