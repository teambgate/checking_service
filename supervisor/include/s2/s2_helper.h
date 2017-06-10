/*
 * Copyright (C) 2017 Manh Tran
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __CHECKING_SERVICE_S2_HELPER_H__
#define __CHECKING_SERVICE_S2_HELPER_H__

#include <s2/types.h>

struct s2_helper *s2_helper_alloc(int min_level, int max_level, int max_cells);

struct s2_cell_union *s2_helper_get_cell_union(struct s2_helper *p, double lat, double lng, double range);

void s2_helper_free(struct s2_helper *p);

struct array *s2_helper_get_cellids(struct s2_helper *p, struct s2_cell_union *u);

struct s2_cell_id *s2_helper_get_cellid(struct s2_helper *p, double lat, double lng, int level);

#endif
