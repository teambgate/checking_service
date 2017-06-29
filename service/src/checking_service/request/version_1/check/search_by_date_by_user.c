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

#include <cherry/crypt/md5.h>

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

static int __validate_input(struct cs_server *p, int fd, u32 mask, struct sobj *obj)
{
        struct string *service_pass = sobj_get_str(p->config, qlkey("service_pass"), RPL_TYPE);
        struct string *pass = sobj_get_str(obj, qskey(&__key_pass__), RPL_TYPE);

        if(strcmp(service_pass->ptr, pass->ptr) != 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("User unauthorized!"));
                return 0;
        }

        struct string *user_name = sobj_get_str(obj, qskey(&__key_user_name__), RPL_TYPE);
        string_trim(user_name);
        if(user_name->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide user name!"));
                return 0;
        }
        if(user_name->len < 6) {
                __response_invalid_data(p, fd, mask, obj, qlkey("username needs have at least 6 characters!"));
                return 0;
        }
        if(!common_username_valid(user_name->ptr, user_name->len)) {
                __response_invalid_data(p, fd, mask, obj, qlkey("username contains invalid character!"));
                return 0;
        }

        struct string *user_pass = sobj_get_str(obj, qskey(&__key_user_pass__), RPL_TYPE);
        string_trim(user_pass);
        if(user_pass->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide password!"));
                return 0;
        }
        if(user_pass->len < 8) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Password need have at least 8 characters!"));
                return 0;
        }

        {
                struct string *from = sobj_get_str(obj, qskey(&__key_from__), RPL_TYPE);
                string_trim(from);
                if(from->len == 0) {
                        __response_invalid_data(p, fd, mask, obj, qlkey("Please provide date from!"));
                        return 0;
                }

                if( ! common_fix_date_time_string(from)) {
                        __response_invalid_data(p, fd, mask, obj, qlkey("Please provide date from!"));
                        return 0;
                }
        }

        {
                struct string *to = sobj_get_str(obj, qskey(&__key_to__), RPL_TYPE);
                string_trim(to);
                if(to->len == 0) {
                        __response_invalid_data(p, fd, mask, obj, qlkey("Please provide date to!"));
                        return 0;
                }

                if( ! common_fix_date_time_string(to)) {
                        __response_invalid_data(p, fd, mask, obj, qlkey("Please provide date to!"));
                        return 0;
                }
        }


        return 1;
}

static void __search_callback(struct cs_server_callback_user_data *cud, struct sobj *recv)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)cud->p->user_head.next - offsetof(struct checking_service , server));

        struct sobj *data = sobj_get_obj(recv, qskey(&__key_data__), RPL_TYPE);
        struct sobj *hits = sobj_get_obj(data, qlkey("hits"), RPL_TYPE);
        int total = sobj_get_int(hits, qlkey("total"), RPL_TYPE);

        if(total > 0) {
                struct sarray *hits_hits     = sobj_get_arr(hits, qlkey("hits"), RPL_TYPE);

                struct sobj *res = sobj_alloc();
                sobj_set_i64(res, qskey(&__key_request_id__),
                        sobj_get_i64(cud->obj, qskey(&__key_request_id__), RPL_TYPE));
                sobj_set_u8(res, qskey(&__key_result__), 1);
                sobj_set_str(res, qskey(&__key_message__), qlkey("successfully"));
                struct string *cmd = sobj_get_str(cud->obj, qskey(&__key_cmd__), RPL_TYPE);
                sobj_set_str(res, qskey(&__key_cmd__), qskey(cmd));

                struct sobj *res_data = sobj_get_obj(res, qskey(&__key_data__), RPL_TYPE);
                struct sarray *checks = sobj_get_arr(res_data, qskey(&__key_checks__), RPL_TYPE);
                int i;
                for_i(i, total) {
                        struct sobj *check        = sarray_get_obj(hits_hits, i, RPL_TYPE);
                        struct sobj *_source    = sobj_get_obj(check, qlkey("_source"), RPL_TYPE);

                        struct string *check_in = sobj_get_str(_source, qskey(&__key_check_in__), RPL_TYPE);
                        struct string *check_out = sobj_get_str(_source, qskey(&__key_check_out__), RPL_TYPE);

                        struct sobj *ci = sobj_alloc();
                        sobj_set_str(ci, qskey(&__key_check_in__), qskey(check_in));
                        sobj_set_str(ci, qskey(&__key_check_out__), qskey(check_out));

                        sarray_add_obj(checks, ci);
                }

                struct string *d        = sobj_to_json(res);
                cs_server_send_to_client(cud->p, cud->fd, cud->mask, d->ptr, d->len, 0);
                string_free(d);
                sobj_free(res);

        } else {
                __response_success(cud->p, cud->fd, cud->mask,  cud->obj, qlkey("empty!"));
                cs_server_callback_user_data_free(cud);
        }
}

static void __search(struct cs_server *p, int fd, u32 mask, struct sobj *obj)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)p->user_head.next - offsetof(struct checking_service , server));

        struct string *user_name        = sobj_get_str(obj, qskey(&__key_user_name__), RPL_TYPE);
        struct string *user_pass        = sobj_get_str(obj, qskey(&__key_user_pass__), RPL_TYPE);
        struct string *from             = sobj_get_str(obj, qskey(&__key_from__), RPL_TYPE);
        struct string *to               = sobj_get_str(obj, qskey(&__key_to__), RPL_TYPE);

        struct string *es_version_code = sobj_get_str(cs->config, qlkey("es_version_code"), RPL_TYPE);
        struct string *es_pass = sobj_get_str(cs->config, qlkey("es_pass"), RPL_TYPE);

        struct string *content = cs_request_string_from_file("res/checking_service/check/search/search_by_date_by_user.json", FILE_INNER);
        string_replace(content, "{USER_NAME}", user_name->ptr);
        string_replace(content, "{USER_PASS}", user_pass->ptr);
        string_replace(content, "{FROM}", from->ptr);
        string_replace(content, "{TO}", to->ptr);
        string_replace_int(content, "{SIZE}", date_time_differents(from->ptr, to->ptr) + 1);

        struct sobj *request_data = cs_request_data_from_string(qskey(content),
                qskey(es_version_code), qskey(es_pass));
        string_free(content);

        struct cs_server_callback_user_data *cud = cs_server_callback_user_data_alloc(p, fd, mask, obj);

        cs_request_alloc(cs->es_server_requester, request_data,
                (cs_request_callback)__search_callback, cud);
}

void checking_service_process_check_search_by_date_by_user_v1(struct cs_server *p, int fd, u32 mask, struct sobj *obj)
{
        if( ! __validate_input(p, fd, mask, obj)) {
                return;
        }

        __search(p, fd, mask, obj);
}
