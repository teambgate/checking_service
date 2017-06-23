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
#ifndef __CHECKING_SERVICE_CHECKING_SERVICE_TYPES_H__
#define __CHECKING_SERVICE_CHECKING_SERVICE_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <cherry/server/types.h>
#include <smartfox/types.h>
#include <common/types.h>
#include <s2/types.h>

extern struct cs_requester *local_requester;

struct checking_service {
        struct list_head        server;

        struct smart_object     *config;

        struct cs_requester     *supervisor_requester;
        struct cs_requester     *es_server_requester;

        struct s2_helper        *s2_helper;

        u8                      command_flag;
        pthread_mutex_t         command_mutex;
        pthread_cond_t          command_cond;
};

extern struct smart_object      *__shared_ram_config__;
extern pthread_mutex_t          __shared_ram_config_mutex__;

struct local_supporter {
        struct list_head        server;
};

#ifdef __cplusplus
}
#endif

#endif
