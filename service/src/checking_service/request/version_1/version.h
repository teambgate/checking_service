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
#ifndef __CHECKING_SERVICE_SUPERVISOR_REQUEST_VERSION_1_VERSION_H__
#define __CHECKING_SERVICE_SUPERVISOR_REQUEST_VERSION_1_VERSION_H__

#include <service/types.h>

void checking_service_process_service_register_username_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_service_validate_username_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);

void checking_service_process_location_register_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_location_update_ip_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_location_update_public_ip_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_location_update_latlng_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);

void checking_service_process_user_reserve_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_user_validate_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);

void checking_service_process_check_in_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_check_out_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_check_search_by_date_by_user_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);

void checking_service_process_device_add_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);

void checking_service_process_work_time_new_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_work_time_new_by_user_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_work_time_search_by_date_by_user_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);

void checking_service_process_permission_add_work_time_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_permission_add_employee_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_permission_clear_checkout_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);

#endif
