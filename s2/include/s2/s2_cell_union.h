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
#ifndef __CHECKING_SERVICE_S2_CELL_UNION_H__
#define __CHECKING_SERVICE_S2_CELL_UNION_H__

#include <s2/types.h>

struct s2_cell_union *s2_cell_union_alloc(jobject obj);

void s2_cell_union_free(struct s2_cell_union *p);

int s2_cell_union_contain(struct s2_cell_union *p, double lat, double lng);

#endif
