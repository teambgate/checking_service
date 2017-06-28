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
#ifndef __CHECKING_SERVICE_CHECKING_CLIENT_clwc_exec_clwc_exec_H__
#define __CHECKING_SERVICE_CHECKING_CLIENT_clwc_exec_clwc_exec_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <checking_client/welcome_controller/types.h>

DEFINE_CONTROLLER(clwc_exec);

void clwc_exec_touch_set_ip(struct nexec *p, struct nview *sender, u8 type);

void clwc_exec_touch_scan(struct nexec *p, struct nview *sender, u8 type);

void clwc_exec_touch_search_ip(struct nexec *p, struct nview *sender, u8 type);

void clwc_exec_touch_item(struct nexec *p, struct nview *sender, u8 type);

void clwc_exec_listen_scan(struct nexec *p, struct smart_object *obj);

void clwc_exec_listen_loc_info(struct nexec *p, struct smart_object *obj);

#ifdef __cplusplus
}
#endif

#endif
