/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2015-2016 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __logger_h__
#define __logger_h__

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <samplecat/typedefs.h>

G_BEGIN_DECLS

#define TYPE_LOGGER (logger_get_type ())
#define LOGGER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_LOGGER, Logger))
#define LOGGER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_LOGGER, LoggerClass))
#define IS_LOGGER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_LOGGER))
#define IS_LOGGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_LOGGER))
#define LOGGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_LOGGER, LoggerClass))

typedef struct _LoggerClass LoggerClass;
typedef struct _LoggerPrivate LoggerPrivate;

struct _Logger {
   GObject parent_instance;
   LoggerPrivate* priv;
   gint state;
};

struct _LoggerClass {
   GObjectClass parent_class;
};

GType   logger_get_type  () G_GNUC_CONST;
Logger* logger_new       ();
Logger* logger_construct (GType);
void    logger_log       (Logger*, const char* format, ...);

G_END_DECLS

#endif
