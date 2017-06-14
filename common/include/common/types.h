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
#ifndef __CHECKING_SERVICE_COMMON_TYPES_H__
#define __CHECKING_SERVICE_COMMON_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <cherry/types.h>
#include <smartfox/types.h>
#include <sys/time.h>
#include <pthread.h>

#define SET_STRING(str) {       \
        .len = sizeof(str) - 1, \
        .ptr = str              \
};

typedef void(*cs_request_callback)(void*, struct sfs_object *);

struct cs_request {
        struct list_head        head;
        struct sfs_object       *data;
};

struct cs_response {
        int                     num;
        struct sfs_object       *data;
        void                    *ctx;
        cs_request_callback     callback;
};

struct cs_requester {
        struct list_head        list;

        struct map              *waits;

        int                     listener;
        i64                     total;
        float                   life_time;
        int                     *send_valid;
        int                     *read_valid;

        pthread_mutex_t         lock;
        pthread_mutex_t         wait_lock;

        struct string           *buff;
        u32                     requested_len;

        pthread_mutex_t         run_mutex;
        pthread_cond_t          run_cond;

        pthread_mutex_t         write_mutex;
        pthread_cond_t          write_cond;

        pthread_mutex_t         read_mutex;
        pthread_cond_t          read_cond;

        struct string           *host;
        u16                     port;

        struct timeval          t1, t2;
};

#ifdef __cplusplus
}
#endif

#endif
