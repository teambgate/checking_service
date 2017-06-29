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
#ifndef __CHECKING_SERVICE_S2_LAT_LNG_H__
#define __CHECKING_SERVICE_S2_LAT_LNG_H__

#include <s2/types.h>

struct s2_lat_lng *s2_lat_lng_alloc(double lat, double lng);

void s2_lat_lng_free(struct s2_lat_lng *p);

struct s2_point *s2_lat_lng_to_point(struct s2_lat_lng *p);

#endif
