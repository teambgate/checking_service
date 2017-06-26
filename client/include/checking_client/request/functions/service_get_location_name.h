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
#ifndef __CHECKING_SERVICE_CHECKING_CLIENT_REQUEST_FUNCTIONS_SERVICE_GET_LOCATION_NAME_H__
#define __CHECKING_SERVICE_CHECKING_CLIENT_REQUEST_FUNCTIONS_SERVICE_GET_LOCATION_NAME_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <checking_client/request/types.h>

struct checking_client_request_service_get_location_name {
        double lat;
        double lon;
};

void checking_client_requester_service_get_location_name(struct checking_client_requester *p,
        struct checking_client_request_search_around_param param);

#ifdef __cplusplus
}
#endif

#endif
