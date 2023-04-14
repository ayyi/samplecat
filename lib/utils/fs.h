/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2020-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <stdbool.h>
#include <stdio.h>

void with_dir (const char* path, GError**, bool (*fn)(GDir*, const char*, gpointer), gpointer);
bool with_fp  (const char* path, const char* mode, bool (*fn)(FILE*, gpointer), gpointer);
