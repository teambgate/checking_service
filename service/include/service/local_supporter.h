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
#ifndef __CHECKING_SERVICE_CHECKING_SERVICE_LOCAL_SUPPORTER_H__
#define __CHECKING_SERVICE_CHECKING_SERVICE_LOCAL_SUPPORTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <service/types.h>

struct local_supporter *local_supporter_alloc();

void local_supporter_start(struct local_supporter *p);

void local_supporter_free(struct local_supporter *p);

#ifdef __cplusplus
}
#endif

#endif
