/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2018-2018 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include <glib.h>
#include "observable.h"

#define _NEW(T, ...) ({T* obj = g_new0(T, 1); *obj = (T){__VA_ARGS__}; obj;})

typedef struct {
   ObservableFn fn;
   gpointer     user;
} Subscription;


Observable*
observable_new ()
{
	return g_new0(Observable, 1);
}


void
observable_free(Observable* observable)
{
	g_list_free_full(observable->subscriptions, g_free);
	g_free(observable);
}


void
observable_set (Observable* observable, int value)
{
	observable->value = value;

	GList* l = observable->subscriptions;
	for(;l;l=l->next){
		Subscription* subscription = l->data;
		subscription->fn(observable, value, subscription->user);
	}
}


void
observable_subscribe (Observable* observable, ObservableFn fn, gpointer user)
{
	observable->subscriptions = g_list_append(observable->subscriptions, _NEW(Subscription,
		.fn = fn,
		.user = user
	));
}
