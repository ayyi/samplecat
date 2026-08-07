/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2025-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |                                                                      |
 | Unit tests for the DockH widget                                      |
 |                                                                      |
 +----------------------------------------------------------------------+
 |
 */

#include "test/dock-h.h"
#include "views/dock_h.h"


void
test_one_child ()
{
	START_TEST;

	AGlActor* actor = dock_h_view(NULL);
	actor->region = (AGlfRegion){ 0, 0, 100, 200 };

	AGlActor* child1 = panel_view(NULL);
	dock_h_add_panel((DockHView*)actor, (PanelView*)child1);

	actor->set_size(actor);

	assert(child1->region.x1 == 0., "child1 x1 %f", child1->region.x1);
	assert(child1->region.x2 == 100., "child1 x2 %f", child1->region.x2);

	FINISH_TEST;
}


void
test_two_children ()
{
	START_TEST;

	AGlActor* actor = dock_h_view(NULL);
	actor->region = (AGlfRegion){ 0, 0, 100, 200 };

	AGlActor* child1 = panel_view(NULL);
	AGlActor* child2 = panel_view(NULL);
	dock_h_add_panel((DockHView*)actor, (PanelView*)child1);
	dock_h_add_panel((DockHView*)actor, (PanelView*)child2);

	actor->set_size(actor);

	extern void agl_actor__print_tree (AGlActor*);
	agl_actor__print_tree(actor);

	assert(child1->region.x1 == 0., "child1 x1 %f", child1->region.x1);
	assert(child1->region.x2 == 43., "child1 x2 %f", child1->region.x2);
	assert(child2->region.x1 == 58., "child2 x1 %f", child2->region.x1);
	assert(child2->region.x2 == 100., "child2 x2 %f", child2->region.x2);

	FINISH_TEST;
}


void
test_three_children ()
{
	START_TEST;

	AGlActor* actor = dock_h_view(NULL);
	actor->region = (AGlfRegion){ 0, 0, 100, 200 };

	AGlActor* child1 = panel_view(NULL);
	AGlActor* child2 = panel_view(NULL);
	AGlActor* child3 = panel_view(NULL);
	dock_h_add_panel((DockHView*)actor, (PanelView*)child1);
	dock_h_add_panel((DockHView*)actor, (PanelView*)child2);
	dock_h_add_panel((DockHView*)actor, (PanelView*)child3);

	actor->set_size(actor);

	assert(child1->region.x1 == 0., "child1 x1 %f", child1->region.x1);
	assert(child1->region.x2 == 24., "child1 x2 %f", child1->region.x2);
	assert(child2->region.x1 == 39., "child2 x1 %f", child2->region.x1);
	assert(child2->region.x2 == 62., "child2 x2 %f", child2->region.x2);
	assert(child3->region.x1 == 77., "child2 x1 %f", child2->region.x1);
	assert(child3->region.x2 == 100., "child2 x2 %f", child2->region.x2);

	FINISH_TEST;
}


void
test_preferred_size ()
{
	START_TEST;

	AGlActor* actor = dock_h_view(NULL);
	actor->region = (AGlfRegion){ 0, 0, 200, 100 };

	AGlActor* child1 = panel_view(NULL);
	AGlActor* child2 = panel_view(NULL);
	((PanelView*)child1)->size_req = (SizeRequest){ .min = { 0, }, .preferred = { 60 }, .max = {-1}};
	dock_h_add_panel((DockHView*)actor, (PanelView*)child1);
	dock_h_add_panel((DockHView*)actor, (PanelView*)child2);

	actor->set_size(actor);

	// the preferred size for child1 is respected and most of the space is allocated to child2
	assert(child1->region.x1 == 0., "child1 x1 %f", child1->region.x1);
	assert(child1->region.x2 == 82., "child1 x2 %f", child1->region.x2);
	assert(child2->region.x1 >= 97., "child2 x1 %f", child2->region.x1);
	assert(child2->region.x2 == 200., "child2 x2 %f", child2->region.x2);

	// if child2 has a max size, most of the space is allocated to child1
	((PanelView*)child2)->size_req.max.x = 70;
	child2->region.x2 = 0;
	actor->set_size(actor);

	assert(child1->region.x1 == 0., "child1 x1 %f", child1->region.x1);
	assert(child1->region.x2 == 115., "child1 x2 %f", child1->region.x2);
	assert(child2->region.x1 == 130., "child2 x1 %f", child2->region.x1);
	assert(child2->region.x2 == 200., "child2 x2 %f", child2->region.x2);

	FINISH_TEST;
}
