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
#ifndef __CHECKING_SERVICE_CHECKING_SERVICE_H__
#define __CHECKING_SERVICE_CHECKING_SERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <service/types.h>

struct checking_service *checking_service_alloc(u8 local_only);

void checking_service_start(struct checking_service *p);

void checking_service_free(struct checking_service *p);

/*
 * setup inner functions
 */
void checking_service_register_username(struct checking_service *p, struct smart_object *in);
void checking_service_register_username_callback(struct checking_service *p, struct smart_object *recv);

void checking_service_validate_username(struct checking_service *p, struct smart_object *in);
void checking_service_validate_username_callback(struct checking_service *p, struct smart_object *recv);

void checking_service_register_location(struct checking_service *p, struct smart_object *in);
void checking_service_register_location_callback(struct checking_service *p, struct smart_object *recv);

void checking_service_location_update_ip(struct checking_service *p, struct smart_object *in);
void checking_service_location_update_ip_callback(struct checking_service *p, struct smart_object *recv);

void checking_service_location_update_public_ip(struct checking_service *p, struct smart_object *in);
void checking_service_location_update_public_ip_callback(struct checking_service *p, struct smart_object *recv);

void checking_service_location_update_latlng(struct checking_service *p, struct smart_object *in);
void checking_service_location_update_latlng_callback(struct checking_service *p, struct smart_object *recv);

void checking_service_user_reserve(struct checking_service *p, struct smart_object *in);
void checking_service_user_reserve_callback(struct checking_service *p, struct smart_object *recv);

void checking_service_user_validate(struct checking_service *p, struct smart_object *in);
void checking_service_user_validate_callback(struct checking_service *p, struct smart_object *recv);

void checking_service_device_add(struct checking_service *p, struct smart_object *in);
void checking_service_device_add_callback(struct checking_service *p, struct smart_object *recv);

/*
 * request delegate
 */
void checking_service_process_service_register_username(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_service_validate_username(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_location_register(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_location_update_ip(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_location_update_latlng(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);

void checking_service_process_user_reserve(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);
void checking_service_process_user_validate(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);

void checking_service_process_device_add(struct cs_server *p, int fd, u32 mask, struct smart_object *obj);

#ifdef __cplusplus
}
#endif

#endif