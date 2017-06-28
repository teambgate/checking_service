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

#include <cherry/time.h>

/*
 * response invalid data
 */
static void __response_invalid_data(struct cs_server *p, int fd, u32 mask, struct smart_object *obj, char *msg, size_t msg_len, long code)
{
        struct smart_object *res = smart_object_alloc();
        smart_object_set_long(res, qskey(&__key_request_id__), smart_object_get_long(obj, qskey(&__key_request_id__), 0));
        smart_object_set_bool(res, qskey(&__key_result__), 0);
        smart_object_set_string(res, qskey(&__key_message__), msg, msg_len);
        smart_object_set_long(res, qskey(&__key_error__), code);

        struct string *d        = smart_object_to_json(res);
        cs_server_send_to_client(p, fd, mask, d->ptr, d->len, 0);
        string_free(d);
        smart_object_free(res);
}

/*
 * response success
 */
static void __response_success(struct cs_server *p, int fd, u32 mask, struct smart_object *obj, char *msg, size_t msg_len)
{
        struct smart_object *res = smart_object_alloc();
        smart_object_set_long(res, qskey(&__key_request_id__), smart_object_get_long(obj, qskey(&__key_request_id__), 0));
        smart_object_set_bool(res, qskey(&__key_result__), 1);
        smart_object_set_string(res, qskey(&__key_message__), msg, msg_len);
        struct string *cmd = smart_object_get_string(obj, qskey(&__key_cmd__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        smart_object_set_string(res, qskey(&__key_cmd__), qskey(cmd));

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
                __response_invalid_data(p, fd, mask, obj, qlkey("User unauthorized!"), ERROR_DATA_INVALID);
                return 0;
        }

        struct string *name = smart_object_get_string(obj, qskey(&__key_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(name);
        if(name->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide name!"), ERROR_DATA_INVALID);
                return 0;
        }

        struct string *user_name = smart_object_get_string(obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(user_name);
        if(user_name->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide user name!"), ERROR_DATA_INVALID);
                return 0;
        }
        if(user_name->len < 6) {
                __response_invalid_data(p, fd, mask, obj, qlkey("username needs have at least 6 characters!"), ERROR_DATA_INVALID);
                return 0;
        }
        if(!common_username_valid(user_name->ptr, user_name->len)) {
                __response_invalid_data(p, fd, mask, obj, qlkey("username contains invalid character!"), ERROR_DATA_INVALID);
                return 0;
        }

        struct string *user_pass = smart_object_get_string(obj, qskey(&__key_user_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(user_pass);
        if(user_pass->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide password!"), ERROR_DATA_INVALID);
                return 0;
        }
        if(user_pass->len < 8) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Password need have at least 8 characters!"), ERROR_DATA_INVALID);
                return 0;
        }

        return 1;
}

static void __reserve_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *result = smart_object_get_string(data, qlkey("result"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        if(strcmp(result->ptr, "created") == 0) {
                __response_success(cud->p, cud->fd, cud->mask,
                        cud->obj, qlkey("user name is created successfully!"));
        } else {
                int status = smart_object_get_int(data, qlkey("status"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                if(status == 409) {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,
                                cud->obj, qlkey("user name is not avaliable!"), ERROR_DATA_EXIST);
                } else {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,
                                cud->obj, qlkey("server error!"), ERROR_SERVER_INVALID);
                }
        }
        cs_server_callback_user_data_free(cud);
}

static void __reserve(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)p->user_head.next - offsetof(struct checking_service , server));

        struct string *name             = smart_object_get_string(obj, qskey(&__key_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *user_name        = smart_object_get_string(obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *user_pass        = smart_object_get_string(obj, qskey(&__key_user_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *current_time     = current_time_to_string(TIME_FORMAT_YY_MM_DD_HH_MM_SS);

        struct string *content = cs_request_string_from_file("res/checking_service/user/create/create.json", FILE_INNER);
        string_replace(content, "{USER_NAME}", user_name->ptr);
        string_replace(content, "{NAME}", name->ptr);
        string_replace(content, "{USER_PASS}", user_pass->ptr);
        string_replace(content, "{RESERVED}", current_time->ptr);

        struct string *es_version_code = smart_object_get_string(p->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = smart_object_get_string(p->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                qskey(es_version_code), qskey(es_pass));
        string_free(content);

        struct cs_server_callback_user_data *cud = cs_server_callback_user_data_alloc(p, fd, mask, obj);

        cs_request_alloc(cs->es_server_requester, request_data,
                (cs_request_callback)__reserve_callback, cud);

        string_free(current_time);
}

void checking_service_process_user_reserve_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        if( ! __validate_input(p, fd, mask, obj)) {
                return;
        }

        __reserve(p, fd, mask, obj);
}
