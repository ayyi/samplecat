/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2020-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#undef USE_GTK
#include "agl/ext.h"
#include "agl/utils.h"
#include "shader.h"
#include "views/overlay.h"

static void overlay_free  (AGlActor*);
static bool overlay_paint (AGlActor*);

static AGl* agl = NULL;
static AGlActorClass actor_class = {0, "Overlay", (AGlActorNew*)overlay_view, overlay_free};


AGlActorClass*
overlay_view_get_class ()
{
	return &actor_class;
}


AGlActor*
overlay_view (gpointer root)
{
	agl = agl_get_instance();

	return agl_actor__add_child(
		(AGlActor*)root,
		(AGlActor*)agl_actor__new(OverlayView,
			.actor = {
				.class = &actor_class,
				.name = actor_class.name,
				.paint = overlay_paint,
				.region = {0, 0, 20, 20}
			}
		)
	);
}


static bool
overlay_paint (AGlActor* actor)
{
	OverlayView* overlay = (OverlayView*)actor;

	agl->shaders.plain->uniform.colour = 0xff6600aa;
	agl_use_program((AGlShader*)agl->shaders.plain);
	agl_rect(overlay->insert_pt.x1, overlay->insert_pt.y1, overlay->insert_pt.x2 - overlay->insert_pt.x1, 1);

	return true;
}


static void
overlay_free (AGlActor* actor)
{
	g_free(actor);
}


void
overlay_set_insert_pos (OverlayView* overlay, AGliRegion pt)
{
	overlay->insert_pt = pt;
}
