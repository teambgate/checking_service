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
#include <supervisor/supervisor.h>
#include "version_1/version.h"

#include <cherry/memory.h>
#include <cherry/string.h>
#include <cherry/array.h>
#include <cherry/map.h>
#include <smartfox/data.h>

#include <common/command.h>
#include <common/key.h>
#include <common/error.h>
#include <common/cs_server.h>

static void __response_invalid_version(struct cs_server *p, int fd, u32 mask, struct sobj *obj)
{
        struct sobj *res = sobj_alloc();
        sobj_set_i64(res, qskey(&__key_request_id__), sobj_get_i64(obj, qskey(&__key_request_id__), 0));
        sobj_set_u8(res, qskey(&__key_result__), 0);
        sobj_set_str(res, qskey(&__key_message__), qlkey(""));
        sobj_set_i64(res, qskey(&__key_error__), ERROR_VERSION_INVALID);
        struct string *cmd = sobj_get_str(obj, qskey(&__key_cmd__), RPL_TYPE);
        sobj_set_str(res, qskey(&__key_cmd__), qskey(cmd));

        struct string *d        = sobj_to_json(res);
        cs_server_send_to_client(p, fd, mask, d->ptr, d->len, 0);
        string_free(d);
        sobj_free(res);
}

#define register_function(func)                                                                 \
void func(struct cs_server *p, int fd, u32 mask, struct sobj *obj)                                 \
{                                                                                               \
        if(!map_has_key(obj->data, qskey(&__key_version__))) {                                  \
                __response_invalid_version(p, fd, mask, obj);                                         \
                return;                                                                         \
        }                                                                                       \
                                                                                                \
        struct string *version = sobj_get_str(obj, qskey(&__key_version__), 0);        \
                                                                                                \
        if(strcmp(version->ptr, "1") == 0) {                                                    \
                func##_v1(p, fd, mask, obj);                                                          \
                return;                                                                         \
        }                                                                                       \
                                                                                                \
        __response_invalid_version(p, fd, mask, obj);                                                 \
}


register_function(supervisor_process_get_service);
register_function(supervisor_process_service_register);
register_function(supervisor_process_service_get_by_username);
register_function(supervisor_process_service_validate);

register_function(supervisor_process_location_register);
register_function(supervisor_process_location_update_latlng);
register_function(supervisor_process_location_update_ip_port);
register_function(supervisor_process_location_search_nearby);
