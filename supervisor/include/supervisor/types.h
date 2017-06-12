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

struct supervisor;

typedef void(*supervisor_delegate)(struct supervisor *, int fd, struct sfs_object *);

struct client_buffer {
        struct string   *buff;
        u32             requested_len;
};

struct supervisor {
        struct file_descriptor_set      *master;
        struct file_descriptor_set      *incomming;
        int                             fdmax;
        int                             listener;
        struct string                   *root;

        struct map                      *delegates;
        struct map                      *clients_datas;

        struct sfs_object               *config;
};

#endif
