/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2018-2021 Tim Orford <tim@orford.org>                  |
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

typedef struct {
   AGlObservableFn fn;
   gpointer        user;
} Subscription;


Observable*
named_observable_new (const char* name)
{
	return (Observable*)SC_NEW(NamedObservable,
		.name = name
	);
}


void
observable_set (Observable* observable, AGlVal value)
{
	// because that because of the possibility of uninitialized padding
	// there is no way to check equality of 2 unions so
	// it is not possible to check here if the value has changed

	observable->value = value;

	GList* l = observable->subscriptions;
	for(;l;l=l->next){
		Subscription* subscription = l->data;
		subscription->fn(observable, value, subscription->user);
	}
}


Observable*
observable_float_new (float val, float min, float max)
{
	Observable* observable = agl_observable_new();

	observable->value = (AGlVal){.f = val};
	observable->min = (AGlVal){.f = min};
	observable->max = (AGlVal){.f = max};

	return observable;
}


void
observable_set_float (Observable* observable, float value)
{
	if (isnan(value)) return;

	value = CLAMP(value, observable->min.f, observable->max.f);

	if (observable->value.f != value) {
		observable_set(observable, (AGlVal){.f=value});
	}
}


/*
 *  Takes ownership of arg str
 */
void
observable_string_set (Observable* observable, const char* str)
{
	bool changed = true;

	if (observable->value.c) {
		changed = (!str) || strcmp(str, observable->value.c);
		g_free(changed ? observable->value.c : (char*)str);
	}

	if (changed)
		observable_set(observable, (AGlVal){.c = (char*)str});
}
