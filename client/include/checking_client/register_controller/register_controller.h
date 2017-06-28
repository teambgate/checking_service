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
#ifndef __CHECKING_SERVICE_CHECKING_CLIENT_REGISTER_CONTROLLER_REGISTER_CONTROLLER_H__
#define __CHECKING_SERVICE_CHECKING_CLIENT_REGISTER_CONTROLLER_REGISTER_CONTROLLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <checking_client/register_controller/types.h>

DEFINE_CONTROLLER(register_controller);

void register_controller_on_touch_back(struct nexec *p, struct nview *sender, u8 type);

void register_controller_on_touch_register(struct nexec *p, struct nview *sender, u8 type);

void register_controller_on_listen_register(struct nexec *p, struct smart_object *obj);

void register_controller_set_location_name(struct nexec *p, char *name, size_t len);

#ifdef __cplusplus
}
#endif

#endif
