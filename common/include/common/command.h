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
extern struct string __cmd_service_register__;
extern struct string __cmd_service_get_by_username__;
extern struct string __cmd_service_validate__;
extern struct string __cmd_service_get_location_info__;

extern struct string __cmd_location_register__;
extern struct string __cmd_location_update_latlng__;
extern struct string __cmd_location_update_ip_port__;
extern struct string __cmd_location_search_nearby__;

extern struct string __cmd_user_reserve__;
extern struct string __cmd_user_search__;
extern struct string __cmd_user_validate__;

extern struct string __cmd_check_in__;
extern struct string __cmd_check_out__;
extern struct string __cmd_check_search_by_date_by_user__;

extern struct string __cmd_device_add__;
extern struct string __cmd_device_search__;

extern struct string __cmd_work_time_new__;
extern struct string __cmd_work_time_new_by_user__;
extern struct string __cmd_work_time_search_by_date_by_user__;

extern struct string __cmd_permission_add_work_time__;
extern struct string __cmd_permission_add_employee__;
extern struct string __cmd_permission_clear_checkout__;

extern struct string __cmd_local_supporter_get_code__;

#ifdef __cplusplus
}
#endif

#endif
