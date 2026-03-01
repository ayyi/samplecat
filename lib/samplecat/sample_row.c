/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2026-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <glib-object.h>
#include <samplecat/sample.h>
#include "sample_row.h"

G_DEFINE_TYPE (SampleRow, sample_row, G_TYPE_OBJECT)

static void
sample_row_dispose (GObject* object)
{
	SampleRow* self = SAMPLECAT_SAMPLE_ROW (object);

	g_clear_pointer(&self->sample, sample_unref);

	G_OBJECT_CLASS (sample_row_parent_class)->dispose (object);
}

static void
sample_row_class_init (SampleRowClass* klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	object_class->dispose = sample_row_dispose;
}

static void
sample_row_init (SampleRow* self)
{
	self->sample = NULL;
}

SampleRow*
sample_row_new (Sample* sample)
{
	g_return_val_if_fail (sample != NULL, NULL);

	SampleRow* self = g_object_new (SAMPLECAT_TYPE_SAMPLE_ROW, NULL);
	self->sample = sample_ref (sample);
	return self;
}

Sample*
sample_row_get_sample (SampleRow* row)
{
	g_return_val_if_fail (SAMPLECAT_IS_SAMPLE_ROW (row), NULL);
	if (!row->sample) return NULL;
	return sample_ref (row->sample);
}
