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
#include <common/command.h>

struct string __cmd_get__                       = SET_STRING("get")
struct string __cmd_post__                      = SET_STRING("post")
struct string __cmd_put__                       = SET_STRING("put")
struct string __cmd_delete__                    = SET_STRING("delete")

struct string __cmd_get_service__               = SET_STRING("get_service")
struct string __cmd_service_register__          = SET_STRING("service_register")
struct string __cmd_service_get_by_username__   = SET_STRING("service_get_by_username")
struct string __cmd_service_validate__          = SET_STRING("service_validate")

struct string __cmd_location_register__         = SET_STRING("location_register")
struct string __cmd_location_update_latlng__    = SET_STRING("location_update_latlng")
struct string __cmd_location_update_ip_port__   = SET_STRING("location_update_ip_port")
struct string __cmd_location_search_nearby__    = SET_STRING("location_search_nearby")
