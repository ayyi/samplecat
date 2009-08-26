#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include "src/typedefs.h"
#include "src/types.h"
#include "support.h"
#include "console_view.h"

void
console__show_result_header()
{
	printf("  name                 directory                            length rate mimetype\n");
}


void
console__show_result(SamplecatResult* result)
{
	printf("  %-20s %-35s %7i %i %s\n", result->sample_name, dir_format(result->dir), result->length, result->sample_rate, result->mimetype);
}

