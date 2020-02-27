/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://www.ayyi.org           |
* | copyright (C) 2018-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __observable_h__
#define __observable_h__

typedef union
{
    int         i;
    float       f;
    double      d;
    char*       c;

} AMVal;

typedef struct {
   AMVal  value;
   AMVal  min;
   AMVal  max;
   GList* subscriptions;
} Observable;

typedef struct {
   Observable  observable;
   const char* name;
} NamedObservable;

typedef enum {
   CHANGE_X = 1,
   CHANGE_Y = 2,
   CHANGE_BOTH = 3,
} PtChangeType;

typedef struct {
   Observable   observable;
   PtChangeType change;
} PtObservable;

typedef void (*ObservableFn) (Observable*, AMVal, gpointer);

Observable* observable_new         ();
Observable* named_observable_new   (const char*);
void        observable_free        (Observable*);
void        observable_set         (Observable*, AMVal);
void        observable_subscribe   (Observable*, ObservableFn, gpointer);
void        observable_subscribe_with_state
                                   (Observable*, ObservableFn, gpointer);
void        observable_unsubscribe (Observable*, ObservableFn, gpointer);

Observable* observable_float_new   (float val, float min, float max);
void        observable_float_set   (Observable*, float);

void        observable_string_set  (Observable*, const char*);

#define     observable_free0(A)    (observable_free(A), A = NULL)

#endif
