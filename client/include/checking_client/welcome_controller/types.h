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
#ifndef __CHECKING_SERVICE_CHECKING_CLIENT_clwc_exec_TYPES_H__
#define __CHECKING_SERVICE_CHECKING_CLIENT_clwc_exec_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <checking_client/types.h>

enum {
        WELCOME_SHOW_SEARCH_IP,
        WELCOME_SHOW_SEARCH_AROUND,
        WELCOME_SEARCHING_IP,
        WELCOME_SEARCHING_AROUND
};

struct clwc_exec_data {
        u8                                                      state;

        struct cl_listener                                      *response_context;

        struct map                                              *cmd_delegate;

        u8                                                      searching_around;
};

#ifdef __cplusplus
}
#endif

#endif
