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
#ifndef __CHECKING_SERVICE_SUPERVISOR_SUPERVISOR_H__
#define __CHECKING_SERVICE_SUPERVISOR_SUPERVISOR_H__

#include <supervisor/types.h>

struct supervisor *supervisor_alloc();

void supervisor_start(struct supervisor *p);

void supervisor_free(struct supervisor *p);

void supervisor_send_to_client(struct supervisor *p, int fd, char *ptr, int len);

/*
 * request delegate
 */
void supervisor_process_get_service(struct supervisor *p, int fd, struct sfs_object *obj);
void supervisor_process_register_service(struct supervisor *p, int fd, struct sfs_object *obj);

#endif
