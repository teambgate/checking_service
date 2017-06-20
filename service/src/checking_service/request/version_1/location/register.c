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
static void __response_invalid_data(struct cs_server *p, int fd, u32 mask, struct smart_object *obj, char *msg, size_t msg_len)
{
        struct smart_object *res = smart_object_alloc();
        smart_object_set_long(res, qskey(&__key_request_id__), smart_object_get_long(obj, qskey(&__key_request_id__), 0));
        smart_object_set_bool(res, qskey(&__key_result__), 0);
        smart_object_set_string(res, qskey(&__key_message__), msg, msg_len);
        smart_object_set_long(res, qskey(&__key_error__), ERROR_DATA_INVALID);

        struct string *d        = smart_object_to_json(res);
        cs_server_send_to_client(p, fd, mask, d->ptr, d->len, 0);
        string_free(d);
        smart_object_free(res);
}

static int __validate_input(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        struct string *service_pass = smart_object_get_string(p->config, qlkey("service_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *pass = smart_object_get_string(obj, qskey(&__key_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(strcmp(service_pass->ptr, pass->ptr) != 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("User unauthorized!"));
                return 0;
        }

        return 1;
}

static void __register_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct smart_object *res = smart_object_alloc();

        smart_object_set_long(res, qskey(&__key_request_id__), smart_object_get_long(cud->obj, qskey(&__key_request_id__), 0));
        smart_object_set_bool(res, qskey(&__key_result__),
                smart_object_get_bool(recv, qskey(&__key_result__), SMART_GET_REPLACE_IF_WRONG_TYPE));

        struct string* message = smart_object_get_string(recv, qskey(&__key_message__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        smart_object_set_string(res, qskey(&__key_message__), qskey(message));

        struct string *d        = smart_object_to_json(res);
        cs_server_send_to_client(cud->p, cud->fd, cud->mask, d->ptr, d->len, 0);
        string_free(d);
        smart_object_free(res);

        cs_server_callback_user_data_free(cud);
}

static void __register(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)p->user_head.next - offsetof(struct checking_service, server));

        struct string *supervisor_version_code = smart_object_get_string(p->config, qlkey("supervisor_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *supervisor_pass = smart_object_get_string(p->config, qlkey("supervisor_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct smart_object *data = smart_object_alloc();
        smart_object_set_string(data, qskey(&__key_version__), qskey(supervisor_version_code));
        smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_location_register__));
        smart_object_set_string(data, qskey(&__key_pass__),qskey(supervisor_pass));

        struct string *user_name = smart_object_get_string(obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *user_pass = smart_object_get_string(obj, qskey(&__key_user_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct smart_object *latlng = smart_object_get_object(obj, qskey(&__key_latlng__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *ip = smart_object_get_string(obj, qskey(&__key_ip__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int port = smart_object_get_int(obj, qskey(&__key_port__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *location_name = smart_object_get_string(obj, qskey(&__key_location_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *device_id = smart_object_get_string(obj, qskey(&__key_device_id__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        smart_object_set_string(data, qskey(&__key_ip__), qskey(ip));
        smart_object_set_int(data, qskey(&__key_port__), port);
        smart_object_set_string(data, qskey(&__key_location_name__), qskey(location_name));
        smart_object_set_string(data, qskey(&__key_user_name__), qskey(user_name));
        smart_object_set_string(data, qskey(&__key_user_pass__), qskey(user_pass));
        smart_object_set_string(data, qskey(&__key_device_id__), qskey(device_id));
        smart_object_set_object(data, qskey(&__key_latlng__), smart_object_clone(latlng));

        struct cs_server_callback_user_data *cud = cs_server_callback_user_data_alloc(p, fd, mask, obj);

        cs_request_alloc(cs->supervisor_requester, data, (cs_request_callback)__register_callback, cud);
}

void checking_service_process_location_register_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        if( ! __validate_input(p, fd, mask, obj)) {
                return;
        }

        __register(p, fd, mask, obj);
}
