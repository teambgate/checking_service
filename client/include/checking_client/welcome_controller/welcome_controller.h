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
#ifndef __CHECKING_SERVICE_CHECKING_CLIENT_WELCOME_CONTROLLER_WELCOME_CONTROLLER_H__
#define __CHECKING_SERVICE_CHECKING_CLIENT_WELCOME_CONTROLLER_WELCOME_CONTROLLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <checking_client/welcome_controller/types.h>

struct native_view_controller *welcome_controller_alloc();

void welcome_controller_on_linked(struct native_view_controller *p);

void welcome_controller_on_removed(struct native_view_controller *p);

void welcome_controller_on_touch_set_ip(struct native_view_controller *p, struct native_view *sender, u8 type);

void welcome_controller_on_touch_search_around(struct native_view_controller *p, struct native_view *sender, u8 type);

void welcome_controller_on_touch_search_ip(struct native_view_controller *p, struct native_view *sender, u8 type);


#ifdef __cplusplus
}
#endif

#endif
