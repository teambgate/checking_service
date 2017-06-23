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
#ifndef __CHECKING_SERVICE_LOCAL_SUPPORTER_REQUEST_VERSION_1_VERSION_H__
#define __CHECKING_SERVICE_LOCAL_SUPPORTER_REQUEST_VERSION_1_VERSION_H__

#include <service/types.h>

void local_supporter_process_get_code_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);

#endif
