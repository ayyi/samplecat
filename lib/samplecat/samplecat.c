/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2016 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#define __samplecat_c__
#include "config.h"
#include "samplecat.h"

void
samplecat_init()
{
	samplecat.logger = logger_new();

	samplecat.model = samplecat_model_new();

	samplecat_model_add_filter (samplecat.model, samplecat.model->filters.search   = samplecat_filter_new("search"));
	samplecat_model_add_filter (samplecat.model, samplecat.model->filters.dir      = samplecat_filter_new("directory"));
	samplecat_model_add_filter (samplecat.model, samplecat.model->filters.category = samplecat_filter_new("category"));

	samplecat.store = (GtkListStore*)samplecat_list_store_new();

#ifdef USE_MYSQL
	samplecat_model_add_backend("mysql");
#endif
#ifdef USE_SQLITE
	samplecat_model_add_backend("sqlite");
#endif
#ifdef USE_TRACKER
	samplecat_model_add_backend("tracker");
#endif
}

