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
#include <common/cs_server.h>

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
        struct string *cmd = smart_object_get_string(obj, qskey(&__key_cmd__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        smart_object_set_string(res, qskey(&__key_cmd__), qskey(cmd));

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

/*
 * validate input
 */
static int __validate_input(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        struct string *service_pass = smart_object_get_string(p->config, qlkey("service_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *pass = smart_object_get_string(obj, qskey(&__key_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(strcmp(service_pass->ptr, pass->ptr) != 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("User unauthorized!\n"));
                return 0;
        }

        return 1;
}

static void __get_location_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct supervisor *supervisor = (struct supervisor *)
                ((char *)cud->p->user_head.next - offsetof(struct supervisor , server));

        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct smart_object *hits = smart_object_get_object(data, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        int total = smart_object_get_int(hits, qlkey("total"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(total) {
                struct smart_object     *response               = smart_object_alloc();
                struct smart_array      *response_location      = smart_object_get_array(response, qskey(&__key_locations__), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct smart_array *hits_hits                   = smart_object_get_array(hits, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                int i;
                for_i(i, total) {
                        struct smart_object *location           = smart_array_get_object(hits_hits, i, SMART_GET_REPLACE_IF_WRONG_TYPE);
                        struct smart_object *_source            = smart_object_get_object(location, qlkey("_source"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                        smart_array_add_object(response_location, smart_object_clone(_source));
                }

                struct smart_object *res = smart_object_alloc();
                smart_object_set_long(res, qskey(&__key_request_id__), smart_object_get_long(cud->obj, qskey(&__key_request_id__), 0));
                smart_object_set_bool(res, qskey(&__key_result__), 1);
                smart_object_set_string(res, qskey(&__key_message__), qlkey("success"));
                smart_object_set_object(res, qskey(&__key_data__), response);
                struct string *cmd = smart_object_get_string(cud->obj, qskey(&__key_cmd__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                smart_object_set_string(res, qskey(&__key_cmd__), qskey(cmd));

                struct string *d        = smart_object_to_json(res);
                cs_server_send_to_client(cud->p, cud->fd, cud->mask, d->ptr, d->len, 0);
                string_free(d);
                smart_object_free(res);
        } else {
                __response_success(cud->p, cud->fd, cud->mask,  cud->obj,
                        qlkey("nothing found!\n"));
        }

        cs_server_callback_user_data_free(cud);
}

static void __get_location(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        struct supervisor *supervisor = (struct supervisor *)
                ((char *)p->user_head.next - offsetof(struct supervisor , server));

        struct smart_object *latlng = smart_object_get_object(obj, qskey(&__key_latlng__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        double lat = smart_object_get_double(latlng, qskey(&__key_lat__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        double lon = smart_object_get_double(latlng, qskey(&__key_lon__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *es_version_code = smart_object_get_string(p->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = smart_object_get_string(p->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *content = cs_request_string_from_file("res/supervisor/location/search_free/search_nearby.json", FILE_INNER);
        string_replace_double(content, "{LAT}", lat);
        string_replace_double(content, "{LON}", lon);

        struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                qskey(es_version_code), qskey(es_pass));
        string_free(content);

        struct cs_server_callback_user_data *cud = cs_server_callback_user_data_alloc(p, fd, mask, obj);

        cs_request_alloc(supervisor->es_server_requester, request_data,
                (cs_request_callback)__get_location_callback, cud);
}

void supervisor_process_location_search_nearby_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        if(!__validate_input(p, fd, mask, obj)) {
                return;
        }

        __get_location(p, fd, mask, obj);
}
