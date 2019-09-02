/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2018-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __observable_h__
#define __observable_h__

typedef struct {
   int    value;
   int    min;
   int    max;
   GList* subscriptions;
} Observable;

typedef void (*ObservableFn) (Observable*, int value, gpointer);

Observable* observable_new       ();
void        observable_free      (Observable*);
void        observable_set       (Observable*, int);
void        observable_subscribe (Observable*, ObservableFn, gpointer);

#endif
