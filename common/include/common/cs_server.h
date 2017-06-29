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
#ifndef __CHECKING_SERVICE_COMMON_CS_SERVER_H__
#define __CHECKING_SERVICE_COMMON_CS_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <common/types.h>

/*
 * cs_server handler
 */
struct cs_server_handler *cs_server_handler_alloc(cs_server_handler_delegate delegate, double time_rate);

void cs_server_handler_free(struct cs_server_handler *p);

struct cs_server_callback_user_data *cs_server_callback_user_data_alloc(struct cs_server *p, int fd, u32 mask, struct sobj *obj);

void cs_server_callback_user_data_free(struct cs_server_callback_user_data *p);



/*
 * cs_server
 */
struct cs_server *cs_server_alloc(u8 local_only);

void cs_server_start(struct cs_server *p, u16 port);

void cs_server_send_to_client(struct cs_server *p, int fd, u32 mask, char *ptr, int len, u8 keep);

void cs_server_free(struct cs_server *p);

#ifdef __cplusplus
}
#endif

#endif
