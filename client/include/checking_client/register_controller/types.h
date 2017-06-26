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
#ifndef __CHECKING_SERVICE_CHECKING_CLIENT_REGISTER_CONTROLLER_TYPES_H__
#define __CHECKING_SERVICE_CHECKING_CLIENT_REGISTER_CONTROLLER_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <checking_client/types.h>

struct register_controller_data {
        struct checking_client_requester_response_context       *response_context;

        struct map                                              *cmd_delegate;
};

#ifdef __cplusplus
}
#endif

#endif
