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
 #ifndef __CHECKING_SERVICE_COMMON_REQUEST_H__
 #define __CHECKING_SERVICE_COMMON_REQUEST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <common/types.h>

struct sobj *cs_request_data_from_file(char *file, int file_type,
        char *version, size_t version_len,
        char *pass, size_t pass_len);

struct sobj *cs_request_data_from_string(char *content, size_t len,
        char *version, size_t version_len,
        char *pass, size_t pass_len);

struct string *cs_request_string_from_file(char *file, int file_type);

/*
 * cs request
 */
struct cs_request_param {
        char    *host;
        int     port;
        int     timeout;
};

struct cs_request *cs_request_alloc(struct cs_requester *p, struct sobj *data, cs_request_callback callback, void *ctx);

struct cs_request *cs_request_alloc_with_host(struct cs_requester *p, struct sobj *data,
        cs_request_callback callback, void *ctx, char *host, size_t host_len, u16 port);

struct cs_request *cs_request_alloc_with_param(struct cs_requester *p, struct sobj *data,
        cs_request_callback callback, void *ctx, struct cs_request_param param);

void cs_request_free(struct cs_request *p);

/*
 * cs requester
 */
struct cs_requester *cs_requester_alloc();

int cs_requester_connect(struct cs_requester *p, char *host, u16 port);

int cs_requester_reconnect(struct cs_requester *p, char *host, size_t host_len, u16 port, int timeout);

void cs_requester_free(struct cs_requester *p);

#ifdef __cplusplus
}
#endif

#endif
