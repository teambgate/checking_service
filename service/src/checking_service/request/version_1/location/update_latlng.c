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
#include <service/checking_service.h>
#include "../version.h"

#include <cherry/memory.h>
#include <cherry/string.h>
#include <cherry/server/file_descriptor.h>
#include <cherry/stdio.h>
#include <cherry/array.h>
#include <cherry/map.h>
#include <cherry/bytes.h>
#include <cherry/math/math.h>
#include <cherry/server/socket.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <netinet/in.h>

#include <cherry/unistd.h>
#include <smartfox/data.h>

#include <common/command.h>
#include <common/key.h>
#include <common/error.h>
#include <common/request.h>
#include <common/cs_server.h>
#include <common/util.h>

/*
 * response invalid data
 */
static void __response_invalid_data(struct cs_server *p, int fd, u32 mask, struct sobj *obj, char *msg, size_t msg_len)
{
        struct sobj *res = sobj_alloc();
        sobj_set_i64(res, qskey(&__key_request_id__), sobj_get_i64(obj, qskey(&__key_request_id__), 0));
        sobj_set_u8(res, qskey(&__key_result__), 0);
        sobj_set_str(res, qskey(&__key_message__), msg, msg_len);
        sobj_set_i64(res, qskey(&__key_error__), ERROR_DATA_INVALID);
        struct string *cmd = sobj_get_str(obj, qskey(&__key_cmd__), RPL_TYPE);
        sobj_set_str(res, qskey(&__key_cmd__), qskey(cmd));

        struct string *d        = sobj_to_json(res);
        cs_server_send_to_client(p, fd, mask, d->ptr, d->len, 0);
        string_free(d);
        sobj_free(res);
}

static int __validate_input(struct cs_server *p, int fd, u32 mask, struct sobj *obj)
{
        struct string *service_pass = sobj_get_str(p->config, qlkey("service_pass"), RPL_TYPE);
        struct string *pass = sobj_get_str(obj, qskey(&__key_pass__), RPL_TYPE);

        if(strcmp(service_pass->ptr, pass->ptr) != 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("User unauthorized!"));
                return 0;
        }

        return 1;
}

static void __update_callback(struct cs_server_callback_user_data *cud, struct sobj *recv)
{
        struct sobj *res = sobj_alloc();

        sobj_set_i64(res, qskey(&__key_request_id__), sobj_get_i64(cud->obj, qskey(&__key_request_id__), 0));
        sobj_set_u8(res, qskey(&__key_result__),
                sobj_get_u8(recv, qskey(&__key_result__), RPL_TYPE));
        struct string *cmd = sobj_get_str(cud->obj, qskey(&__key_cmd__), RPL_TYPE);
        sobj_set_str(res, qskey(&__key_cmd__), qskey(cmd));

        struct string* message = sobj_get_str(recv, qskey(&__key_message__), RPL_TYPE);
        sobj_set_str(res, qskey(&__key_message__), qskey(message));

        struct string *d        = sobj_to_json(res);
        cs_server_send_to_client(cud->p, cud->fd, cud->mask, d->ptr, d->len, 0);
        string_free(d);
        sobj_free(res);

        cs_server_callback_user_data_free(cud);
}

static void __update(struct cs_server *p, int fd, u32 mask, struct sobj *obj)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)p->user_head.next - offsetof(struct checking_service, server));

        struct string *supervisor_version_code = sobj_get_str(p->config, qlkey("supervisor_version_code"), RPL_TYPE);
        struct string *supervisor_pass = sobj_get_str(p->config, qlkey("supervisor_pass"), RPL_TYPE);

        struct sobj *data = sobj_alloc();
        sobj_set_str(data, qskey(&__key_version__), qskey(supervisor_version_code));
        sobj_set_str(data, qskey(&__key_cmd__), qskey(&__cmd_location_update_latlng__));
        sobj_set_str(data, qskey(&__key_pass__),qskey(supervisor_pass));

        struct string *user_name = sobj_get_str(obj, qskey(&__key_user_name__), RPL_TYPE);
        struct string *user_pass = sobj_get_str(obj, qskey(&__key_user_pass__), RPL_TYPE);
        struct sobj *latlng = sobj_get_obj(obj, qskey(&__key_latlng__), RPL_TYPE);
        struct string *device_id = sobj_get_str(obj, qskey(&__key_device_id__), RPL_TYPE);

        sobj_set_str(data, qskey(&__key_user_name__), qskey(user_name));
        sobj_set_str(data, qskey(&__key_user_pass__), qskey(user_pass));
        sobj_set_str(data, qskey(&__key_device_id__), qskey(device_id));
        sobj_set_obj(data, qskey(&__key_latlng__), sobj_clone(latlng));

        struct cs_server_callback_user_data *cud = cs_server_callback_user_data_alloc(p, fd, mask, obj);

        cs_request_alloc(cs->supervisor_requester, data, (cs_request_callback)__update_callback, cud);
}

void checking_service_process_location_update_latlng_v1(struct cs_server *p, int fd, u32 mask, struct sobj *obj)
{
        if( ! __validate_input(p, fd, mask, obj)) {
                return;
        }

        __update(p, fd, mask, obj);
}
