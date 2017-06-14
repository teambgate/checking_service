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

/*
 * response success
 */
static void __response_success(struct supervisor *p, int fd, u32 mask, struct sfs_object *obj, char *msg, size_t msg_len)
{
        struct sfs_object *res = sfs_object_alloc();
        sfs_object_set_long(res, qskey(&__key_request_id__), sfs_object_get_long(obj, qskey(&__key_request_id__), 0));
        sfs_object_set_bool(res, qskey(&__key_result__), 1);
        sfs_object_set_string(res, qskey(&__key_message__), msg, msg_len);

        struct string *d        = sfs_object_to_json(res);
        supervisor_send_to_client(p, fd, mask, d->ptr, d->len, 0);
        string_free(d);
        sfs_object_free(res);
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

        struct string *user_name = sfs_object_get_string(obj, qskey(&__key_user_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(user_name);
        if(user_name->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide user name!\n"));
                return 0;
        }
        if(!common_username_valid(user_name->ptr, user_name->len)) {
                __response_invalid_data(p, fd, mask, obj, qlkey("username contains invalid character!\n"));
                return 0;
        }

        return 1;
}

/*
 * validate service
 */
static void __update_validated_callback(struct callback_user_data *p, struct sfs_object *result)
{
        struct sfs_object *data = sfs_object_get_object(result, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        int _version = sfs_object_get_int(data, qlkey("_version"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        int _old_version = sfs_object_get_int(p->obj, qlkey("_version"), SFS_GET_REPLACE_IF_WRONG_TYPE);

        if(_version > 0 && _version - 1 == _old_version) {
                __response_success(p->p, p->fd, p->mask,  p->obj, qlkey("user name is validated successfully!\n"));
        } else {
                if(_version == 0) {
                        __response_invalid_data(p->p, p->fd, p->mask,  p->obj, qlkey("error!\n"));
                } else if(_old_version != _version) {
                        __response_invalid_data(p->p, p->fd, p->mask,  p->obj, qlkey("error!\n"));
                } else if(_old_version == _version) {
                        __response_success(p->p, p->fd, p->mask,  p->obj, qlkey("user name has been validated already!\n"));
                } else {
                        __response_invalid_data(p->p, p->fd, p->mask,  p->obj, qlkey("error!\n"));
                }
        }

        callback_user_data_free(p);
}

static void __update_validate_count_callback(struct callback_user_data *p, struct sfs_object *result)
{

        callback_user_data_free(p);
}

static void __validate_service_callback(struct callback_user_data *cud, struct sfs_object *result)
{
        struct sfs_object *data = sfs_object_get_object(result, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        u8 found = sfs_object_get_bool(data, qlkey("found"), SFS_GET_REPLACE_IF_WRONG_TYPE);

        if(found) {
                int _version                    = sfs_object_get_int(data, qlkey("_version"), SFS_GET_REPLACE_IF_WRONG_TYPE);

                struct sfs_object *_source      = sfs_object_get_object(data, qlkey("_source"), SFS_GET_REPLACE_IF_WRONG_TYPE);

                int validate_count              = sfs_object_get_int(_source, qskey(&__key_validate_count__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                int validated                   = sfs_object_get_int(_source, qskey(&__key_validated__), SFS_GET_REPLACE_IF_WRONG_TYPE);

                if(validated) {
                        __response_success(cud->p, cud->fd, cud->mask,  cud->obj, qlkey("This user name has been validated already!\n"));
                        callback_user_data_free(cud);
                } else {
                        if(validate_count <= 0) {
                                __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj, qlkey("This user name has been blocked!\n"));
                                callback_user_data_free(cud);
                        } else {
                                struct string *validate_code = sfs_object_get_string(_source, qskey(&__key_validate_code__), SFS_GET_REPLACE_IF_WRONG_TYPE);

                                struct string *input_code = sfs_object_get_string(cud->obj, qskey(&__key_validate_code__), SFS_GET_REPLACE_IF_WRONG_TYPE);

                                if(validate_code->len && strcmp(input_code->ptr, validate_code->ptr) == 0) {
                                        /*
                                         * put _version into obj to recompare next stage
                                         */
                                        sfs_object_set_int(cud->obj, qlkey("_version"), _version);
                                        /*
                                         * try validate successfuly
                                         */
                                        struct string *es_version_code = sfs_object_get_string(cud->p->config, qlkey("es_version_code"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                                        struct string *es_pass = sfs_object_get_string(cud->p->config, qlkey("es_pass"), SFS_GET_REPLACE_IF_WRONG_TYPE);

                                        struct sfs_object *request_data = cs_request_data_from_file("res/supervisor/service/update_validated.json", FILE_INNER,
                                                qskey(es_version_code), qskey(es_pass));

                                        struct string *request_data_path = sfs_object_get_string(request_data, qskey(&__key_path__), SFS_GET_REPLACE_IF_WRONG_TYPE);

                                        struct string *_ids = sfs_object_get_string(data, qlkey("_id"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                                        string_trim(_ids);

                                        string_replace(request_data_path, "{ID}", _ids->ptr);

                                        struct string *temp = string_alloc(0);
                                        string_cat_int(temp, _version);
                                        string_replace(request_data_path, "{VERSION_NUMBER}", temp->ptr);
                                        string_free(temp);

                                        cs_request_alloc(cud->p->es_server_requester, request_data,
                                                (cs_request_callback)__update_validated_callback, cud);
                                } else {
                                        /*
                                         * try validate successfuly
                                         */
                                        struct string *es_version_code = sfs_object_get_string(cud->p->config, qlkey("es_version_code"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                                        struct string *es_pass = sfs_object_get_string(cud->p->config, qlkey("es_pass"), SFS_GET_REPLACE_IF_WRONG_TYPE);

                                        struct sfs_object *request_data = cs_request_data_from_file("res/supervisor/service/update_validate_count.json", FILE_INNER,
                                                qskey(es_version_code), qskey(es_pass));

                                        struct string *request_data_path = sfs_object_get_string(request_data, qskey(&__key_path__), SFS_GET_REPLACE_IF_WRONG_TYPE);

                                        struct string *_ids = sfs_object_get_string(data, qlkey("_id"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                                        string_trim(_ids);

                                        string_replace(request_data_path, "{ID}", _ids->ptr);

                                        cs_request_alloc(cud->p->es_server_requester, request_data,
                                                (cs_request_callback)__update_validate_count_callback, cud);

                                        __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj, qlkey("wrong code!\n"));
                                }
                        }
                }
        } else {
                __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj, qlkey("user name is not registered!\n"));
                callback_user_data_free(cud);
        }
}

static void __validate_service(struct supervisor *p, int fd, u32 mask, struct sfs_object *obj)
{
        struct string *user_name = sfs_object_get_string(obj, qskey(&__key_user_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        struct string *es_version_code = sfs_object_get_string(p->config, qlkey("es_version_code"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = sfs_object_get_string(p->config, qlkey("es_pass"), SFS_GET_REPLACE_IF_WRONG_TYPE);

        struct sfs_object *request_data = cs_request_data_from_file("res/supervisor/service/search_by_username.json", FILE_INNER,
                qskey(es_version_code), qskey(es_pass));

        struct string *path = sfs_object_get_string(request_data, qskey(&__key_path__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        string_replace(path, "{USER_NAME}", user_name->ptr);

        struct callback_user_data *cud = callback_user_data_alloc(p, fd, mask, obj);

        cs_request_alloc(p->es_server_requester, request_data,
                (cs_request_callback)__validate_service_callback, cud);
}

void supervisor_process_validate_service_v1(struct supervisor *p, int fd, u32 mask, struct sfs_object *obj)
{
        if(!__validate_input(p, fd, mask, obj)) {
                return;
        }

        __validate_service(p, fd, mask, obj);
}
