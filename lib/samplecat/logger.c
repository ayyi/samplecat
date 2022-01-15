/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2015-2022 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <logger.h>

static gpointer logger_parent_class = NULL;

enum  {
	LOGGER_DUMMY_PROPERTY
};

static GObject* logger_constructor (GType, guint n_construct_properties, GObjectConstructParam*);
static void     logger_finalize    (GObject*);


Logger*
logger_construct (GType object_type)
{
	Logger * self = NULL;
	self = (Logger*) g_object_new (object_type, NULL);
	self->state = 1;
	return self;
}


Logger*
logger_new (void)
{
	return logger_construct (TYPE_LOGGER);
}


void
logger_log (Logger* self, const char* format, ...)
{
	g_return_if_fail(self);
	g_return_if_fail(format);

	char out[256];

	va_list argp;
	va_start(argp, format);
	vsnprintf(out, 255, format, argp);
	out[255] = '\0';
	va_end(argp);

	g_signal_emit_by_name (self, "message", out);
}


static GObject*
logger_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (logger_parent_class);
	GObject* obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	return obj;
}


static void
logger_class_init (LoggerClass* klass)
{
	logger_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->constructor = logger_constructor;
	G_OBJECT_CLASS (klass)->finalize = logger_finalize;
	g_signal_new ("message", TYPE_LOGGER, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}


static void
logger_instance_init (Logger* self)
{
	self->state = 0;
}


static void
logger_finalize (GObject* obj)
{
	G_OBJECT_CLASS (logger_parent_class)->finalize (obj);
}


GType
logger_get_type (void)
{
	static volatile gsize logger_type_id__volatile = 0;
	if (g_once_init_enter ((gsize*)&logger_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (LoggerClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) logger_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (Logger), 0, (GInstanceInitFunc) logger_instance_init, NULL };
		GType logger_type_id;
		logger_type_id = g_type_register_static (G_TYPE_OBJECT, "Logger", &g_define_type_info, 0);
		g_once_init_leave (&logger_type_id__volatile, logger_type_id);
	}
	return logger_type_id__volatile;
}
