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
#ifndef __CHECKING_SERVICE_SUPERVISOR_TYPES_H__
#define __CHECKING_SERVICE_SUPERVISOR_TYPES_H__

#include <cherry/server/types.h>
#include <smartfox/types.h>
#include <common/types.h>

struct supervisor;

typedef void(*supervisor_delegate)(struct supervisor *, int fd, u32 mask, struct smart_object *);

struct client_buffer {
        struct string   *buff;
        u32             requested_len;
};

typedef void(*supervisor_handler_delegate)(struct supervisor *);

struct supervisor_handler {
        struct list_head                head;

        supervisor_handler_delegate     delegate;
        double                          time_rate;
        struct timeval                  last_executed_time;
};

struct callback_user_data {
        struct supervisor *p;
        int fd;
        u32 mask;
        struct smart_object *obj;
};

struct client_step {
        int fd;
        u64 step;
};

struct supervisor {
        struct file_descriptor_set      *master;
        struct file_descriptor_set      *incomming;
        int                             fdmax;
        int                             listener;
        struct string                   *root;

        struct array                    *fd_mask;

        struct array                    *fd_invalids;

        struct list_head                handlers;

        struct cs_requester             *es_server_requester;

        struct map                      *delegates;

        pthread_mutex_t                 client_data_mutex;

        struct map                      *clients_datas;

        u64                             step;

        struct smart_object               *config;
};

#endif
