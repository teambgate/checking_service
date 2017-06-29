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
                __response_invalid_data(p, fd, mask, obj, qlkey("User unauthorized!"));
                return 0;
        }

        struct string *user_name = sobj_get_str(obj, qskey(&__key_user_name__), RPL_TYPE);
        string_trim(user_name);
        if(user_name->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide user name!"));
                return 0;
        }
        if(!common_username_valid(user_name->ptr, user_name->len)) {
                __response_invalid_data(p, fd, mask, obj, qlkey("username contains invalid character!"));
                return 0;
        }

        return 1;
}

/*
 * validate service
 */
static void __update_validated_callback(struct cs_server_callback_user_data *p, struct sobj *result)
{
        struct sobj *data = sobj_get_obj(result, qskey(&__key_data__), RPL_TYPE);
        int _version = sobj_get_int(data, qlkey("_version"), RPL_TYPE);
        int _old_version = sobj_get_int(p->obj, qlkey("_version"), RPL_TYPE);

        if(_version > 0 && _version - 1 == _old_version) {
                __response_success(p->p, p->fd, p->mask,  p->obj, qlkey("user name is validated successfully!"));
        } else {
                if(_version == 0) {
                        __response_invalid_data(p->p, p->fd, p->mask,  p->obj, qlkey("error!"));
                } else if(_old_version != _version) {
                        __response_invalid_data(p->p, p->fd, p->mask,  p->obj, qlkey("error!"));
                } else if(_old_version == _version) {
                        __response_success(p->p, p->fd, p->mask,  p->obj, qlkey("user name has been validated already!"));
                } else {
                        __response_invalid_data(p->p, p->fd, p->mask,  p->obj, qlkey("error!"));
                }
        }

        cs_server_callback_user_data_free(p);
}

static void __update_validate_count_callback(struct cs_server_callback_user_data *p, struct sobj *result)
{

        cs_server_callback_user_data_free(p);
}

static void __validate_service_callback(struct cs_server_callback_user_data *cud, struct sobj *recv)
{
        struct supervisor *supervisor = (struct supervisor *)
                ((char *)cud->p->user_head.next - offsetof(struct supervisor , server));

        struct sobj *data = sobj_get_obj(recv, qskey(&__key_data__), RPL_TYPE);

        struct sobj *hits = sobj_get_obj(data, qlkey("hits"), RPL_TYPE);

        int total = sobj_get_int(hits, qlkey("total"), RPL_TYPE);

        if(total == 1) {
                struct sarray *hits_hits     = sobj_get_arr(hits, qlkey("hits"), RPL_TYPE);
                struct sobj *service     = sarray_get_obj(hits_hits, 0, RPL_TYPE);

                struct string *_id              = sobj_get_str(service, qlkey("_id"), RPL_TYPE);
                int _version                     = sobj_get_int(service, qlkey("_version"), RPL_TYPE);
                struct sobj *_source      = sobj_get_obj(service, qlkey("_source"), RPL_TYPE);

                int validate_count              = sobj_get_int(_source, qskey(&__key_validate_count__), RPL_TYPE);
                int validated                   = sobj_get_int(_source, qskey(&__key_validated__), RPL_TYPE);

                if(validated) {
                        __response_success(cud->p, cud->fd, cud->mask,  cud->obj, qlkey("This user name has been validated already!\n"));
                        cs_server_callback_user_data_free(cud);
                } else {
                        if(validate_count <= 0) {
                                __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj, qlkey("This user name has been blocked!\n"));
                                cs_server_callback_user_data_free(cud);
                        } else {
                                struct string *validate_code = sobj_get_str(_source, qskey(&__key_validate_code__), RPL_TYPE);

                                struct string *input_code = sobj_get_str(cud->obj, qskey(&__key_validate_code__), RPL_TYPE);

                                if(validate_code->len && strcmp(input_code->ptr, validate_code->ptr) == 0) {
                                        /*
                                         * put _version into obj to recompare next stage
                                         */
                                        sobj_set_i32(cud->obj, qlkey("_version"), _version);
                                        /*
                                         * try validate successfuly
                                         */
                                        struct string *es_version_code = sobj_get_str(cud->p->config, qlkey("es_version_code"), RPL_TYPE);
                                        struct string *es_pass = sobj_get_str(cud->p->config, qlkey("es_pass"), RPL_TYPE);

                                        struct sobj *request_data = cs_request_data_from_file("res/supervisor/service/create/update_validated.json", FILE_INNER,
                                                qskey(es_version_code), qskey(es_pass));

                                        struct string *request_data_path = sobj_get_str(request_data, qskey(&__key_path__), RPL_TYPE);

                                        struct string *_ids = sobj_get_str(service, qlkey("_id"), RPL_TYPE);
                                        string_trim(_ids);

                                        string_replace(request_data_path, "{ID}", _ids->ptr);

                                        struct string *temp = string_alloc(0);
                                        string_cat_int(temp, _version);
                                        string_replace(request_data_path, "{VERSION_NUMBER}", temp->ptr);
                                        string_free(temp);

                                        cs_request_alloc(supervisor->es_server_requester, request_data,
                                                (cs_request_callback)__update_validated_callback, cud);
                                } else {
                                        /*
                                         * try validate successfuly
                                         */
                                        struct string *es_version_code = sobj_get_str(cud->p->config, qlkey("es_version_code"), RPL_TYPE);
                                        struct string *es_pass = sobj_get_str(cud->p->config, qlkey("es_pass"), RPL_TYPE);

                                        struct sobj *request_data = cs_request_data_from_file("res/supervisor/service/create/update_validate_count.json", FILE_INNER,
                                                qskey(es_version_code), qskey(es_pass));

                                        struct string *request_data_path = sobj_get_str(request_data, qskey(&__key_path__), RPL_TYPE);

                                        struct string *_ids = sobj_get_str(data, qlkey("_id"), RPL_TYPE);
                                        string_trim(_ids);

                                        string_replace(request_data_path, "{ID}", _ids->ptr);

                                        cs_request_alloc(supervisor->es_server_requester, request_data,
                                                (cs_request_callback)__update_validate_count_callback, cud);

                                        __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj, qlkey("wrong code!"));
                                }
                        }
                }
        } else {
                __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj, qlkey("user name is not registered!"));
                cs_server_callback_user_data_free(cud);
        }
}

static void __validate_service(struct cs_server *p, int fd, u32 mask, struct sobj *obj)
{
        struct supervisor *supervisor = (struct supervisor *)
                ((char *)p->user_head.next - offsetof(struct supervisor , server));

        struct string *user_name = sobj_get_str(obj, qskey(&__key_user_name__), RPL_TYPE);
        struct string *user_pass = sobj_get_str(obj, qskey(&__key_user_pass__), RPL_TYPE);

        struct string *es_version_code = sobj_get_str(p->config, qlkey("es_version_code"), RPL_TYPE);
        struct string *es_pass = sobj_get_str(p->config, qlkey("es_pass"), RPL_TYPE);

        struct string *content = cs_request_string_from_file("res/supervisor/service/create/search_by_username.json", FILE_INNER);
        string_replace(content, "{USER_NAME}", user_name->ptr);
        string_replace(content, "{USER_PASS}", user_pass->ptr);

        struct sobj *request_data = cs_request_data_from_string(qskey(content),
                qskey(es_version_code), qskey(es_pass));
        string_free(content);

        struct cs_server_callback_user_data *cud = cs_server_callback_user_data_alloc(p, fd, mask, obj);

        cs_request_alloc(supervisor->es_server_requester, request_data,
                (cs_request_callback)__validate_service_callback, cud);
}

void supervisor_process_service_validate_v1(struct cs_server *p, int fd, u32 mask, struct sobj *obj)
{
        if(!__validate_input(p, fd, mask, obj)) {
                return;
        }

        __validate_service(p, fd, mask, obj);
}
