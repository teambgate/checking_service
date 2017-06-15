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
#include <common/util.h>

#include <supervisor/callback_user_data.h>

/*
 * response invalid data
 */
static void __response_invalid_data(struct supervisor *p, int fd, u32 mask, struct smart_object *obj, char *msg, size_t msg_len)
{
        struct smart_object *res = smart_object_alloc();
        smart_object_set_long(res, qskey(&__key_request_id__), smart_object_get_long(obj, qskey(&__key_request_id__), 0));
        smart_object_set_bool(res, qskey(&__key_result__), 0);
        smart_object_set_string(res, qskey(&__key_message__), msg, msg_len);
        smart_object_set_long(res, qskey(&__key_error__), ERROR_DATA_INVALID);

        struct string *d        = smart_object_to_json(res);
        supervisor_send_to_client(p, fd, mask, d->ptr, d->len, 0);
        string_free(d);
        smart_object_free(res);
}

/*
 * response success
 */
static void __response_success(struct supervisor *p, int fd, u32 mask, struct smart_object *obj, char *msg, size_t msg_len)
{
        struct smart_object *res = smart_object_alloc();
        smart_object_set_long(res, qskey(&__key_request_id__), smart_object_get_long(obj, qskey(&__key_request_id__), 0));
        smart_object_set_bool(res, qskey(&__key_result__), 1);
        smart_object_set_string(res, qskey(&__key_message__), msg, msg_len);

        struct string *d        = smart_object_to_json(res);
        supervisor_send_to_client(p, fd, mask, d->ptr, d->len, 0);
        string_free(d);
        smart_object_free(res);
}

/*
 * validate input
 */
static int __validate_input(struct supervisor *p, int fd, u32 mask, struct smart_object *obj)
{
        struct string *service_pass = smart_object_get_string(p->config, qlkey("service_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *pass = smart_object_get_string(obj, qskey(&__key_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(strcmp(service_pass->ptr, pass->ptr) != 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("User unauthorized!\n"));
                return 0;
        }

        struct string *user_name = smart_object_get_string(obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(user_name);
        if(user_name->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide user name!\n"));
                return 0;
        }

        struct string *user_pass = smart_object_get_string(obj, qskey(&__key_user_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(user_pass);
        if(user_pass->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide user pass!\n"));
                return 0;
        }

        struct string *device_id = smart_object_get_string(obj, qskey(&__key_device_id__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(device_id);
        if(device_id->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide device id!\n"));
                return 0;
        }

        return 1;
}

static void __update_latlng_callback(struct callback_user_data *cud, struct smart_object *recv)
{
        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int _version = smart_object_get_int(data, qlkey("_version"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int _old_version = smart_object_get_int(cud->obj, qlkey("_version"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(_version > 0 && _version - 1 == _old_version) {
                __response_success(cud->p, cud->fd, cud->mask,  cud->obj,
                        qlkey("location latlng is updated successfully!\n"));
        } else {
                if(_version == 0) {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                                qlkey("error!\n"));
                } else if(_old_version != _version) {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                                qlkey("please try again!\n"));
                } else if(_old_version == _version) {
                        __response_success(cud->p, cud->fd, cud->mask,  cud->obj,
                                qlkey("nothing to change!\n"));
                } else {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                                qlkey("error!\n"));
                }
        }

        callback_user_data_free(cud);
}

static void __get_location_callback(struct callback_user_data *cud, struct smart_object *recv)
{
        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct smart_object *hits = smart_object_get_object(data, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        int total = smart_object_get_int(hits, qlkey("total"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(total == 1) {
                struct string *user_name        = smart_object_get_string(cud->obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct smart_object *latlng     = smart_object_get_object(cud->obj, qskey(&__key_latlng__), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct smart_array *hits_hits     = smart_object_get_array(hits, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct smart_object *location     = smart_array_get_object(hits_hits, 0, SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *_id              = smart_object_get_string(location, qlkey("_id"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                int version                     = smart_object_get_int(location, qlkey("_version"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct smart_object *_source      = smart_object_get_object(location, qlkey("_source"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                /*
                 * store current _version to check collision
                 */
                smart_object_set_int(cud->obj, qlkey("_version"), version);

                struct string *es_version_code = smart_object_get_string(cud->p->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct string *es_pass = smart_object_get_string(cud->p->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *content = cs_request_string_from_file("res/supervisor/location/update/update_latlng.json", FILE_INNER);

                string_replace(content, "{SERVICE_ID}", user_name->ptr);
                string_replace(content, "{ID}", _id->ptr);
                string_replace_double(content, "{LAT}", smart_object_get_double(latlng, qskey(&__key_lat__), SMART_GET_REPLACE_IF_WRONG_TYPE));
                string_replace_double(content, "{LON}", smart_object_get_double(latlng, qskey(&__key_lon__), SMART_GET_REPLACE_IF_WRONG_TYPE));
                string_replace_int(content, "{VERSION_NUMBER}", version);

                struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                        qskey(es_version_code), qskey(es_pass));

                string_free(content);

                cs_request_alloc(cud->p->es_server_requester, request_data,
                        (cs_request_callback)__update_latlng_callback, cud);
        } else {
                __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                        qlkey("User not found or wrong password!\n"));
                callback_user_data_free(cud);
        }
}

static void __update_location(struct supervisor *p, int fd, u32 mask, struct smart_object *obj)
{
        struct string *user_name = smart_object_get_string(obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *device_id = smart_object_get_string(obj, qskey(&__key_device_id__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *user_pass = smart_object_get_string(obj, qskey(&__key_user_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *es_version_code = smart_object_get_string(p->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = smart_object_get_string(p->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *content = cs_request_string_from_file("res/supervisor/location/update/search.json", FILE_INNER);

        string_replace(content, "{DEVICE_ID}", device_id->ptr);
        string_replace(content, "{SERVICE_ID}", user_name->ptr);
        string_replace(content, "{USER_PASS}", user_pass->ptr);

        struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                qskey(es_version_code), qskey(es_pass));

        string_free(content);

        struct callback_user_data *cud = callback_user_data_alloc(p, fd, mask, obj);

        cs_request_alloc(p->es_server_requester, request_data,
                (cs_request_callback)__get_location_callback, cud);
}

void supervisor_process_location_update_latlng_v1(struct supervisor *p, int fd, u32 mask, struct smart_object *obj)
{
        if(!__validate_input(p, fd, mask, obj)) {
                return;
        }

        __update_location(p, fd, mask, obj);
}
