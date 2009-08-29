#warning move mysql out of app struct
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <gtk/gtk.h>
#include "mysql/mysql.h" //FIXME
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

	printf("filters: %s\n", app.search_dir);

	printf("  name                 directory                            length  ch rate mimetype\n");
}


void
console__show_result(SamplecatResult* result)
{
	printf("  %-20s %-35s %7i %i %5i %s\n", result->sample_name, dir_format(result->dir), result->length, result->channels, result->sample_rate, result->mimetype);
}


static int
get_terminal_width()
{
	struct winsize ws;

	if(ioctl(1, TIOCGWINSZ, (void *)&ws) != 0) printf("ioctl failed\n");

	dbg(1, "terminal width = %d\n", ws.ws_col);

	return ws.ws_col;
}


