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
#ifndef __CHECKING_SERVICE_CHECKING_CLIENT_REQUEST_REQUEST_H__
#define __CHECKING_SERVICE_CHECKING_CLIENT_REQUEST_REQUEST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <checking_client/request/types.h>

/*
 * context
 */
struct cl_listener *cl_listener_alloc();

void cl_listener_init(struct cl_listener *p);

void cl_listener_clear(struct cl_listener *p);

void cl_listener_free(struct cl_listener *p);

/*
 * data
 */
struct cl_response *cl_response_alloc();

void cl_response_free(struct cl_response *p);

/*
 * requester
 */
struct cl_talker *cl_talker_shared();

struct cl_talker *cl_talker_alloc(struct sobj *config);

void cl_talker_free(struct cl_talker *p);

void cl_talker_add_context(struct cl_talker *p,
        struct cl_listener *context);

void cl_talker_process_datas(struct cl_talker *p);

void cl_talker_callback(struct cl_talker *p, struct sobj *obj);

void cl_talker_send(struct cl_talker *p, struct sobj *obj, struct cl_dst dst);

#ifdef __cplusplus
}
#endif

#endif
