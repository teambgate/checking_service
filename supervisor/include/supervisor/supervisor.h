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
#include <common/types.h>

struct supervisor *supervisor_alloc();

void supervisor_start(struct supervisor *p);

void supervisor_free(struct supervisor *p);

/*
 * request delegate
 */
void supervisor_process_get_service(struct cs_server *p, int fd, u32 mask, struct sobj *obj);
void supervisor_process_service_register(struct cs_server *p, int fd, u32 mask, struct sobj *obj);
void supervisor_process_service_get_by_username(struct cs_server *p, int fd, u32 mask, struct sobj *obj);
void supervisor_process_service_validate(struct cs_server *p, int fd, u32 mask, struct sobj *obj);

void supervisor_process_location_register(struct cs_server *p, int fd, u32 mask, struct sobj *obj);
void supervisor_process_location_update_latlng(struct cs_server *p, int fd, u32 mask, struct sobj *obj);
void supervisor_process_location_update_ip_port(struct cs_server *p, int fd, u32 mask, struct sobj *obj);
void supervisor_process_location_search_nearby(struct cs_server *p, int fd, u32 mask, struct sobj *obj);

/*
 * handle delegate
 */
void supervisor_process_clear_invalidated_service(struct cs_server *p);

#endif
