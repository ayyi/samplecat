/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2018-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include "math.h"
#include <glib.h>
#include "samplecat/support.h"
#include "observable.h"

#if 0
typedef struct {
   AyyiObservableFn fn;
   gpointer         user;
} Subscription;
#endif


Observable*
named_observable_new (const char* name)
{
	return (Observable*)SC_NEW(NamedObservable,
		.name = name
	);
}


Observable*
observable_float_new (float val, float min, float max)
{
	Observable* observable = ayyi_observable_new();

	observable->value = (AyyiVal){.f = val};
	observable->min = (AyyiVal){.f = min};
	observable->max = (AyyiVal){.f = max};

	return observable;
}


void
observable_set_float (Observable* observable, float value)
{
	if (isnan(value)) return;

	value = CLAMP(value, observable->min.f, observable->max.f);

	if (observable->value.f != value) {
		ayyi_observable_set(observable, (AyyiVal){.f=value});
	}
}
