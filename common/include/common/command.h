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
#ifndef __CHECKING_SERVICE_COMMON_COMMAND_H__
#define __CHECKING_SERVICE_COMMON_COMMAND_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <common/types.h>

extern struct string __cmd_get__;
extern struct string __cmd_post__;
extern struct string __cmd_put__;
extern struct string __cmd_delete__;

extern struct string __cmd_get_service__;
extern struct string __cmd_register_service__;
extern struct string __cmd_get_service_by_username__;
extern struct string __cmd_validate_service__;

extern struct string __cmd_register_location__;
extern struct string __cmd_update_location_latlng__;
extern struct string __cmd_update_location_ip_port__;

#ifdef __cplusplus
}
#endif

#endif
