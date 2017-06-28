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
#ifndef __CHECKING_SERVICE_CHECKING_CLIENT_REQUEST_TYPES_H__
#define __CHECKING_SERVICE_CHECKING_CLIENT_REQUEST_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <checking_client/types.h>

typedef void(*cl_listener_delegate)(void *, struct smart_object *);

struct cl_listener {
        pthread_mutex_t         *lock;
        struct list_head        head;
        void                    *ctx;
        cl_listener_delegate    delegate;
};

struct cl_response {
        struct list_head        head;
        struct smart_object     *data;
};

/*
 * destination parameters
 */
struct cl_dst {
        /*
         * service destination
         */
        char    *ip;
        int     port;
        /*
         * service version code
         */
        char    *ver;
        int     ver_len;
        /*
         * service password
         */
        char    *pss;
        int     pss_len;
};

extern struct cl_dst __dst_srv;
extern struct cl_dst __dst_sup;

struct cl_talker {
        struct cs_requester     *requester;
        struct smart_object     *config;

        struct list_head        task;
        struct list_head        listeners;
        struct list_head        datas;
        pthread_mutex_t         lock;
};

#ifdef __cplusplus
}
#endif

#endif
