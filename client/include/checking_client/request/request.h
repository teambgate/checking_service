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
struct checking_client_requester_response_context *checking_client_requester_response_context_alloc();

void checking_client_requester_response_context_init(struct checking_client_requester_response_context *p);

void checking_client_requester_response_context_clear(struct checking_client_requester_response_context *p);

void checking_client_requester_response_context_free(struct checking_client_requester_response_context *p);

/*
 * data
 */
struct checking_client_requester_response_data *checking_client_requester_response_data_alloc();

void checking_client_requester_response_data_free(struct checking_client_requester_response_data *p);

/*
 * requester
 */
struct checking_client_requester *checking_client_requester_get_instance();

struct checking_client_requester *checking_client_requester_alloc(struct smart_object *config);

void checking_client_requester_free(struct checking_client_requester *p);

void checking_client_requester_add_context(struct checking_client_requester *p,
        struct checking_client_requester_response_context *context);

void checking_client_requester_process_datas(struct checking_client_requester *p);

void checking_client_requester_callback(struct checking_client_requester *p, struct smart_object *obj);


#ifdef __cplusplus
}
#endif

#endif
