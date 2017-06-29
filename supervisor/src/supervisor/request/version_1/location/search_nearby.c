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

/*
 * response success
 */
static void __response_success(struct cs_server *p, int fd, u32 mask, struct sobj *obj, char *msg, size_t msg_len)
{
        struct sobj *res = sobj_alloc();
        sobj_set_i64(res, qskey(&__key_request_id__), sobj_get_i64(obj, qskey(&__key_request_id__), 0));
        sobj_set_u8(res, qskey(&__key_result__), 1);
        sobj_set_str(res, qskey(&__key_message__), msg, msg_len);
        struct string *cmd = sobj_get_str(obj, qskey(&__key_cmd__), RPL_TYPE);
        sobj_set_str(res, qskey(&__key_cmd__), qskey(cmd));

        struct string *d        = sobj_to_json(res);
        cs_server_send_to_client(p, fd, mask, d->ptr, d->len, 0);
        string_free(d);
        sobj_free(res);
}

/*
 * validate input
 */
static int __validate_input(struct cs_server *p, int fd, u32 mask, struct sobj *obj)
{
        struct string *service_pass = sobj_get_str(p->config, qlkey("service_pass"), RPL_TYPE);
        struct string *pass = sobj_get_str(obj, qskey(&__key_pass__), RPL_TYPE);

        if(strcmp(service_pass->ptr, pass->ptr) != 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("User unauthorized!\n"));
                return 0;
        }

        return 1;
}

static void __get_location_callback(struct cs_server_callback_user_data *cud, struct sobj *recv)
{
        struct supervisor *supervisor = (struct supervisor *)
                ((char *)cud->p->user_head.next - offsetof(struct supervisor , server));

        struct sobj *data = sobj_get_obj(recv, qskey(&__key_data__), RPL_TYPE);

        struct sobj *hits = sobj_get_obj(data, qlkey("hits"), RPL_TYPE);

        int total = sobj_get_int(hits, qlkey("total"), RPL_TYPE);

        if(total) {
                struct sobj     *response               = sobj_alloc();
                struct sarray      *response_location      = sobj_get_arr(response, qskey(&__key_locations__), RPL_TYPE);

                struct sarray *hits_hits                   = sobj_get_arr(hits, qlkey("hits"), RPL_TYPE);
                int i;
                for_i(i, total) {
                        struct sobj *location           = sarray_get_obj(hits_hits, i, RPL_TYPE);
                        struct sobj *_source            = sobj_get_obj(location, qlkey("_source"), RPL_TYPE);

                        sarray_add_obj(response_location, sobj_clone(_source));
                }

                struct sobj *res = sobj_alloc();
                sobj_set_i64(res, qskey(&__key_request_id__), sobj_get_i64(cud->obj, qskey(&__key_request_id__), 0));
                sobj_set_u8(res, qskey(&__key_result__), 1);
                sobj_set_str(res, qskey(&__key_message__), qlkey("success"));
                sobj_set_obj(res, qskey(&__key_data__), response);
                struct string *cmd = sobj_get_str(cud->obj, qskey(&__key_cmd__), RPL_TYPE);
                sobj_set_str(res, qskey(&__key_cmd__), qskey(cmd));

                struct string *d        = sobj_to_json(res);
                cs_server_send_to_client(cud->p, cud->fd, cud->mask, d->ptr, d->len, 0);
                string_free(d);
                sobj_free(res);
        } else {
                __response_success(cud->p, cud->fd, cud->mask,  cud->obj,
                        qlkey("nothing found!\n"));
        }

        cs_server_callback_user_data_free(cud);
}

static void __get_location(struct cs_server *p, int fd, u32 mask, struct sobj *obj)
{
        struct supervisor *supervisor = (struct supervisor *)
                ((char *)p->user_head.next - offsetof(struct supervisor , server));

        struct sobj *latlng = sobj_get_obj(obj, qskey(&__key_latlng__), RPL_TYPE);
        double lat = sobj_get_f64(latlng, qskey(&__key_lat__), RPL_TYPE);
        double lon = sobj_get_f64(latlng, qskey(&__key_lon__), RPL_TYPE);

        struct string *es_version_code = sobj_get_str(p->config, qlkey("es_version_code"), RPL_TYPE);
        struct string *es_pass = sobj_get_str(p->config, qlkey("es_pass"), RPL_TYPE);

        struct string *content = cs_request_string_from_file("res/supervisor/location/search_free/search_nearby.json", FILE_INNER);
        string_replace_double(content, "{LAT}", lat);
        string_replace_double(content, "{LON}", lon);

        struct sobj *request_data = cs_request_data_from_string(qskey(content),
                qskey(es_version_code), qskey(es_pass));
        string_free(content);

        struct cs_server_callback_user_data *cud = cs_server_callback_user_data_alloc(p, fd, mask, obj);

        cs_request_alloc(supervisor->es_server_requester, request_data,
                (cs_request_callback)__get_location_callback, cud);
}

void supervisor_process_location_search_nearby_v1(struct cs_server *p, int fd, u32 mask, struct sobj *obj)
{
        if(!__validate_input(p, fd, mask, obj)) {
                return;
        }

        __get_location(p, fd, mask, obj);
}
