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

#pragma once

#include <glib-object.h>
#include <samplecat/sample.h>

G_BEGIN_DECLS

#define SAMPLECAT_TYPE_SAMPLE_ROW            (sample_row_get_type ())
#define SAMPLECAT_SAMPLE_ROW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_SAMPLE_ROW, SampleRow))
#define SAMPLECAT_IS_SAMPLE_ROW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLECAT_TYPE_SAMPLE_ROW))
#define SAMPLECAT_SAMPLE_ROW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_SAMPLE_ROW, SampleRowClass))
#define SAMPLECAT_IS_SAMPLE_ROW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_SAMPLE_ROW))
#define SAMPLECAT_SAMPLE_ROW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_SAMPLE_ROW, SampleRowClass))

typedef struct _SampleRow        SampleRow;
typedef struct _SampleRowClass   SampleRowClass;

/* A GObject wrapper around Sample* facilitating usage in GListModel.
 * The wrapper owns a ref to the underlying Sample and releases it on finalize.
 */
struct _SampleRow {
	GObject parent_instance;
	Sample* sample;
	void* overview;
};

struct _SampleRowClass {
	GObjectClass parent_class;
};

GType      sample_row_get_type    (void) G_GNUC_CONST;
SampleRow* sample_row_new         (Sample* sample);
/* Returns a referenced Sample*. Caller must sample_unref() when done. */
Sample*    sample_row_get_sample  (SampleRow* row);

G_END_DECLS
