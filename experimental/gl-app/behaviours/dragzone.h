/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2013-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |                                                                      |
 |  DragZone                                                            |
 |                                                                      |
 |  An AGlActor can be assigned a number of interaction zones           |
 |  represented by DragZones                                            |
 |                                                                      |
 |  Each DragZone handles a drag operation                              |
 |                                                                      |
 |  DragZone provides a generic way to:                                 |
 |   - handle mouse events. First the actor needs to Pick the DragZone, |
 |     then it can update the object as it is moved.                    |
 |   - update the model (AMObject) during and after user changes.       |
 |   - reset the model if change cancelled (not in this version).       |
 |   - scroll the actor if user scrolls outside (not in this version).  |
 |                                                                      |
 |  It is designed to simplify the event handler by making behaviour    |
 |  uniform for each type of object.                                    |
 |                                                                      | 
 |  Usage of DragZone hides some low level details but it is quite      |
 |  complex and is not a complete winner.                               |
 |                                                                      |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include "agl/actor.h"
#include "agl/observable.h"

// -----------------------------------------------------------

typedef struct _AMObject AMObject;
typedef void  (*ErrorHandler) (const GError* error, void*);

struct _AMObject
{
	struct TypedVal {
		AGlVal   val;
		int      type;
	}*           vals;
	void         (*set) (AGlActor*, AMObject*, int, AGlVal, ErrorHandler, gpointer);
	gpointer     user_data;
};

void am_object_init   (AMObject*, int n_properties);
void am_object_deinit (AMObject*);

#define am_object_val(A) ((A)->vals[0].val)

// -----------------------------------------------------------

typedef struct _DragZone      DragZone;
typedef struct _DragZoneClass DragZoneClass;

typedef DragZone* (*ActorPick)   (AGlActor*, AGliPt);

struct _DragZoneClass
{
    Cursor* cursor;       // for hover

    AGlVal (*xy_to_val)   (DragZone*, int, AGlActor*, AGliPt, AGliPt offset);
#if 0
    void   (*change_done) (DragZone*, void*, void*, DragHandler, gpointer);
#endif
    void   (*abort)       (DragZone*);
};

struct _DragZone
{
    DragZoneClass* class;
    AGlVal*        val;
    AGlVal         orig_val;
    AGliRegion     region;    // the region to initiate interaction.
    struct {
       const char* text;
    }              hover;
    gpointer       user_data;
};

void dragzone_init            (DragZoneClass*, DragZone*, DragZone prototype);
void dragzone_box             (DragZone*, int);

#define abort_change(A) A->class->abort(A);
#define dragzone_width(A) (A->region.x2 - A->region.x1)

typedef struct {
   AGlBehaviour behaviour;
   AMObject*    model;
   int          n_zones;
   DragZone     zones[];
} DragZoneBehaviour;

typedef struct {
   AGlBehaviour behaviour;
   AMObject*    model;
   int          n_zones;
} DragZoneBehaviourPrototype;

AGlBehaviourClass* dragzone_get_class ();

AGlBehaviour* dragzone_behaviour_init (DragZoneBehaviour*, DragZoneBehaviourPrototype);
