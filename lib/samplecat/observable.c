/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2018-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include "math.h"
#include <glib.h>
#include "samplecat/support.h"
#include "observable.h"

typedef struct {
   ObservableFn fn;
   gpointer     user;
} Subscription;


Observable*
observable_new ()
{
	return g_new0(Observable, 1);
}


Observable*
named_observable_new (const char* name)
{
	return (Observable*)SC_NEW(NamedObservable,
		.name = name
	);
}


/*
 *  This will not free string values, this needs to be done by the user.
 */
void
observable_free (Observable* observable)
{
	g_list_free_full(observable->subscriptions, g_free);
	g_free(observable);
}


void
observable_set (Observable* observable, AMVal value)
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


void
observable_subscribe (Observable* observable, ObservableFn fn, gpointer user)
{
	observable->subscriptions = g_list_append(observable->subscriptions, SC_NEW(Subscription,
		.fn = fn,
		.user = user
	));
}


/*
 *  Calls back imediately with the current value
 */
void
observable_subscribe_with_state (Observable* observable, ObservableFn fn, gpointer user)
{
	observable_subscribe(observable, fn, user);
	fn(observable, observable->value, user);
}


/*
 *  Disconnect by either fn or user_data if they are set
 */
void
observable_unsubscribe (Observable* observable, ObservableFn fn, gpointer user)
{
	GList* l = observable->subscriptions;
	for(;l;l=l->next){
		Subscription* subscription = l->data;
		if((fn && fn == subscription->fn) || (user && user == subscription->user)){
			observable->subscriptions = g_list_delete_link(observable->subscriptions, l);
			break;
		}
	}
}


Observable*
observable_float_new (float val, float min, float max)
{
	Observable* observable = observable_new();

	observable->value = (AMVal){.f = val};
	observable->min = (AMVal){.f = min};
	observable->max = (AMVal){.f = max};

	return observable;
}


void
observable_float_set (Observable* observable, float value)
{
	if(isnan(value)) return;

	value = CLAMP(value, observable->min.f, observable->max.f);

	if(observable->value.f != value){
		observable_set(observable, (AMVal){.f=value});
	}
}


/*
 *  Takes ownership of arg str
 */
void
observable_string_set (Observable* observable, const char* str)
{
	bool changed = true;

	if(observable->value.c){
		changed = (!str) || strcmp(str, observable->value.c);
		g_free(observable->value.c);
	}

	if(changed)
		observable_set(observable, (AMVal){.c = (char*)str});
}
