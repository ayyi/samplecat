#pragma once

#ifdef HAVE_IL8N_H
#include <glib/gi18n-lib.h>
#else
#define _(A) A
#endif
