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
#ifndef __CHECKING_SERVICE_CHECKING_SERVICE_REQUEST_VERSION_1_WORKTIME_SHARE_H__
#define __CHECKING_SERVICE_CHECKING_SERVICE_REQUEST_VERSION_1_WORKTIME_SHARE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <service/types.h>

void checking_service_process_work_time_new_start_v1(struct cs_server_callback_user_data *cud);

#ifdef __cplusplus
}
#endif

#endif
