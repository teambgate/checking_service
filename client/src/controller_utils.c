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
#include <checking_client/controller_utils.h>
#include <checking_client/root_view_controller/root_view_controller.h>
#include <checking_client/welcome_controller/welcome_controller.h>
#include <cherry/string.h>

struct native_view_controller *checking_client_native_view_controller_alloc(char *name, size_t len)
{
        if(strcmp(name, "welcome_controller") == 0) {
                return welcome_controller_alloc();
        } else if(strcmp(name, "root_view_controller") == 0) {
                return root_view_controller_alloc();
        }
        return NULL;
}
