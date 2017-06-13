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
#include "version.h"

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

#include <cherry/time.h>

/*
 * callback user data
 */
struct callback_user_data {
        struct supervisor *p;
        int fd;
        u32 mask;
        struct sfs_object *obj;
};

static struct callback_user_data *__callback_user_data_alloc(struct supervisor *p, int fd, u32 mask, struct sfs_object *obj)
{
        struct callback_user_data *cud = smalloc(sizeof(struct callback_user_data));
        cud->p = p;
        cud->fd = fd;
        cud->mask = mask;
        cud->obj = sfs_object_clone(obj);
        return cud;
}

static void __callback_user_data_free(struct callback_user_data *p)
{
        sfs_object_free(p->obj);
        sfree(p);
}

/*
 * response invalid data
 */
static void __response_invalid_data(struct supervisor *p, int fd, u32 mask, struct sfs_object *obj, char *msg, size_t msg_len)
{
        struct sfs_object *res = sfs_object_alloc();
        sfs_object_set_long(res, qskey(&__key_request_id__), sfs_object_get_long(obj, qskey(&__key_request_id__), 0));
        sfs_object_set_bool(res, qskey(&__key_result__), 0);
        sfs_object_set_string(res, qskey(&__key_message__), msg, msg_len);
        sfs_object_set_long(res, qskey(&__key_error__), ERROR_DATA_INVALID);

        struct string *d        = sfs_object_to_json(res);
        supervisor_send_to_client(p, fd, mask, d->ptr, d->len, 0);
        string_free(d);
        sfs_object_free(res);
}

static void __register_service_callback(struct callback_user_data *p, struct sfs_object *data)
{
        // struct sfs_object *res = sfs_object_alloc();
        // sfs_object_set_long(res, qskey(&__key_request_id__), sfs_object_get_long(p->obj, qskey(&__key_request_id__), 0));
        // sfs_object_set_bool(res, qskey(&__key_result__), 1);
        // sfs_object_set_string(res, qskey(&__key_message__), qlkey("success"));
        //
        // // int counter = 0;
        // // struct sfs_object *result = sfs_object_from_json(ptr, realsize, &counter);
        // // sfs_object_set_object(res, qskey(&__key_data__), result);
        //
        // struct string *d        = sfs_object_to_json(res);
        // supervisor_send_to_client(p->p, p->fd, d->ptr, d->len);
        // string_free(d);
        // sfs_object_free(res);
        //
        // sfs_object_free(p->obj);
        // sfree(p);
}
/*
 * validate input
 */
static int __validate_input(struct supervisor *p, int fd, u32 mask, struct sfs_object *obj)
{
        struct string *service_pass = sfs_object_get_string(p->config, qlkey("service_pass"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct string *pass = sfs_object_get_string(obj, qskey(&__key_pass__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        if(strcmp(service_pass->ptr, pass->ptr) != 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("User unauthorized!\n"));
                return 0;
        }

        struct string *name = sfs_object_get_string(obj, qskey(&__key_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(name);
        if(name->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide name!\n"));
                return 0;
        }

        struct string *user_name = sfs_object_get_string(obj, qskey(&__key_user_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(user_name);
        if(user_name->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide user name!\n"));
                return 0;
        }
        if(user_name->len < 6) {
                __response_invalid_data(p, fd, mask, obj, qlkey("username need have at least 6 characters!\n"));
                return 0;
        }

        struct string *user_pass = sfs_object_get_string(obj, qskey(&__key_user_pass__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(user_pass);
        if(user_pass->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide password!\n"));
                return 0;
        }
        if(user_pass->len < 8) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Password need have at least 8 characters!\n"));
                return 0;
        }

        return 1;
}

/*
 * reserve user name
 */
static void __reserve_user_name_callback(struct callback_user_data *p, struct sfs_object *result)
{
        struct string *res = sfs_object_to_json(result);
        debug("validate user name : %s\n", res->ptr);
        string_free(res);
}

/*
 * validate user name
 */
static void __validate_user_name_callback(struct callback_user_data *p, struct sfs_object *result)
{
        // struct string *res = sfs_object_to_json(data);
        // debug("validate user name : %s\n", res->ptr);
        // string_free(res);

        struct sfs_object *data = sfs_object_get_object(result, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        struct sfs_object *hits = sfs_object_get_object(data, qlkey("hits"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        int total = sfs_object_get_int(hits, qlkey("total"), SFS_GET_REPLACE_IF_WRONG_TYPE);

        if(total <= 0) {
                struct string *es_version_code = sfs_object_get_string(p->p->config, qlkey("es_version_code"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                struct string *es_pass = sfs_object_get_string(p->p->config, qlkey("es_pass"), SFS_GET_REPLACE_IF_WRONG_TYPE);

                struct string *name             = sfs_object_get_string(p->obj, qskey(&__key_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                struct string *user_name        = sfs_object_get_string(p->obj, qskey(&__key_user_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                struct string *user_pass        = sfs_object_get_string(p->obj, qskey(&__key_user_pass__), SFS_GET_REPLACE_IF_WRONG_TYPE);

                struct string *current_time     = current_time_to_string();

                struct sfs_object *request_data = cs_request_data_from_file(
                        "res/supervisor/service/create.json", FILE_INNER,
                        qskey(es_version_code), qskey(es_pass));

                struct sfs_object *request_data_data = sfs_object_get_object(request_data, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                sfs_object_set_string(request_data_data, qskey(&__key_name__), qskey(name));
                sfs_object_set_string(request_data_data, qskey(&__key_user_name__), qskey(user_name));
                sfs_object_set_string(request_data_data, qskey(&__key_user_pass__), qskey(user_pass));
                sfs_object_set_string(request_data_data, qskey(&__key_reserved__), qskey(current_time));

                cs_request_alloc(p->p->es_server_requester, request_data, (cs_request_callback)__reserve_user_name_callback, p);

                string_free(current_time);
        } else {
                __response_invalid_data(p->p, p->fd, p->mask,  p->obj, qlkey("user name is not avaliable!\n"));
                __callback_user_data_free(p);
        }
}

static void __validate_user_name(struct supervisor *p, int fd, u32 mask, struct sfs_object *obj)
{
        struct string *user_name = sfs_object_get_string(obj, qskey(&__key_user_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        struct string *es_version_code = sfs_object_get_string(p->config, qlkey("es_version_code"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = sfs_object_get_string(p->config, qlkey("es_pass"), SFS_GET_REPLACE_IF_WRONG_TYPE);

        struct sfs_object *request_data = cs_request_data_from_file("res/supervisor/service/search_by_username.json", FILE_INNER,
                qskey(es_version_code), qskey(es_pass));

        struct sfs_object *request_data_data = sfs_object_get_object(request_data, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct sfs_object *query = sfs_object_get_object(request_data_data, qlkey("query"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct sfs_object *match = sfs_object_get_object(query, qlkey("match"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        sfs_object_set_string(match, qskey(&__key_user_name__), qskey(user_name));

        struct callback_user_data *cud = __callback_user_data_alloc(p, fd, mask, obj);

        cs_request_alloc(p->es_server_requester, request_data,
                (cs_request_callback)__validate_user_name_callback, cud);
}

void supervisor_process_register_service_v1(struct supervisor *p, int fd, u32 mask, struct sfs_object *obj)
{
        if(!__validate_input(p, fd, mask, obj)) {
                return;
        }

        __validate_user_name(p, fd, mask, obj);

        // struct string *es_version_code = sfs_object_get_string(p->config, qlkey("es_version_code"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        // struct string *es_pass = sfs_object_get_string(p->config, qlkey("es_pass"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        //
        // struct sfs_object *request_data = cs_request_data_from_file("res/supervisor/service/create.json", FILE_INNER,
        //         qskey(es_version_code), qskey(es_pass));
        //
        // struct sfs_object *request_data_data = sfs_object_get_object(request_data, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        // sfs_object_set_string(request_data_data, qskey(&__key_name__), qskey(name));
        // sfs_object_set_string(request_data_data, qskey(&__key_user_pass__), qskey(user_pass));
        //
        // struct callback_user_data *cud = smalloc(sizeof(struct callback_user_data));
        // cud->p = p;
        // cud->fd = fd;
        // cud->obj = sfs_object_clone(obj);
        //
        // cs_request_alloc(p->es_server_requester, request_data, (cs_request_callback)__register_service_callback, cud);
}
