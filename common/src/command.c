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

struct string __cmd_get__               = SET_STRING("get")
struct string __cmd_post__              = SET_STRING("post")
struct string __cmd_put__               = SET_STRING("put")

struct string __cmd_get_service__       = SET_STRING("get_service")
struct string __cmd_register_service__  = SET_STRING("register_service")
