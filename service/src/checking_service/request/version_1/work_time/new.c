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

#include "share.h"

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

/*
 * response success
 */
static void __response_success(struct cs_server *p, int fd, u32 mask, struct smart_object *obj, char *msg, size_t msg_len)
{
        struct smart_object *res = smart_object_alloc();
        smart_object_set_long(res, qskey(&__key_request_id__), smart_object_get_long(obj, qskey(&__key_request_id__), 0));
        smart_object_set_bool(res, qskey(&__key_result__), 1);
        smart_object_set_string(res, qskey(&__key_message__), msg, msg_len);

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

        struct string *user_name = smart_object_get_string(obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
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

        struct string *time_start = smart_object_get_string(obj, qskey(&__key_time_start__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(time_start);
        if(time_start->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide time start!"));
                return 0;
        }

        int h, m, s= 0;
        int count = sscanf(time_start->ptr, "%d:%d:%d", &h, &m, &s);
        if (count >= 2) {
                char str[20];
                memset(str, 0, 20);
                sprintf(str, "%02d:%02d:%02d", h, m, s);
                time_start->len = 0;
                string_cat(time_start, str, strlen(str));
        } else {
                __response_invalid_data(p, fd, mask, obj, qlkey("input time start invalid!"));
                return 0;
        }

        return 1;
}

static void __create_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *result = smart_object_get_string(data, qlkey("result"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        if(strcmp(result->ptr, "created") == 0) {
                __response_success(cud->p, cud->fd, cud->mask,
                        cud->obj, qlkey("create work time successfully!"));
        } else {
                int status = smart_object_get_int(data, qlkey("status"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                if(status == 409) {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,
                                cud->obj, qlkey("error!"));
                } else {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,
                                cud->obj, qlkey("server error!"));
                }
        }
        cs_server_callback_user_data_free(cud);
}

static void __update_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int _version = smart_object_get_int(data, qlkey("_version"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int _old_version = smart_object_get_int(cud->obj, qlkey("_version"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(_version > 0 && _version - 1 == _old_version) {
                __response_success(cud->p, cud->fd, cud->mask,  cud->obj,
                        qlkey("update work time successfully!"));
        } else {
                if(_version == 0) {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                                qlkey("error!"));
                } else if(_old_version != _version) {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                                qlkey("please try again!"));
                } else if(_old_version == _version) {
                        __response_success(cud->p, cud->fd, cud->mask,  cud->obj,
                                qlkey("update work time successfully!"));
                } else {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                                qlkey("error!"));
                }
        }

        cs_server_callback_user_data_free(cud);
}

static void __store_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
}

static void __search_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)cud->p->user_head.next - offsetof(struct checking_service , server));

        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct smart_object *hits = smart_object_get_object(data, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        int total = smart_object_get_int(hits, qlkey("total"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *user_name = smart_object_get_string(cud->obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(total == 1) {
                struct smart_array *hits_hits     = smart_object_get_array(hits, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct smart_object *cwt        = smart_array_get_object(hits_hits, 0, SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *_id              = smart_object_get_string(cwt, qlkey("_id"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                int _version                     = smart_object_get_int(cwt, qlkey("_version"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct smart_object *_source      = smart_object_get_object(cwt, qlkey("_source"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                smart_object_set_int(cud->obj, qlkey("_version"), _version);
                /*
                 * store old work time
                 */
                {
                        struct string *current_time = current_time_to_string();
                        string_replace(current_time, "-", "_");
                        string_replace(current_time, " ", "_");
                        string_replace(current_time, ":", "_");

                        struct string *id = string_alloc(0);
                        string_cat_string(id, user_name);
                        string_cat(id, qlkey("_work_time_"));
                        string_cat_string(id, current_time);

                        string_free(current_time);
                        current_time = current_time_to_string();

                        struct string *es_version_code = smart_object_get_string(cs->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                        struct string *es_pass = smart_object_get_string(cs->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                        struct string *content = cs_request_string_from_file("res/checking_service/work_time/create/create.json", FILE_INNER);
                        string_replace(content, "{ID}", id->ptr);
                        string_replace(content, "{PARENT_ID}", user_name->ptr);
                        string_replace(content, "{DATE_END}", current_time->ptr);
                        string_replace(content, "{TIME_START}", smart_object_get_string(_source, qskey(&__key_time_start__), SMART_GET_REPLACE_IF_WRONG_TYPE)->ptr);
                        string_replace(content, "{DATE_START}", smart_object_get_string(_source, qskey(&__key_date_start__), SMART_GET_REPLACE_IF_WRONG_TYPE)->ptr);

                        struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                                qskey(es_version_code), qskey(es_pass));
                        string_free(content);

                        cs_request_alloc(cs->es_server_requester, request_data,
                                (cs_request_callback)__store_callback, cud);
                        string_free(id);
                        string_free(current_time);
                }
                /*
                 * update new work time
                 */
                {
                        /*
                         * create new work time
                         */
                        struct string *current_time = offset_time_to_string(28800);

                        struct string *id = string_alloc(0);
                        string_cat_string(id, user_name);
                        string_cat(id, qlkey("_work_time_now"));

                        struct string *es_version_code = smart_object_get_string(cs->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                        struct string *es_pass = smart_object_get_string(cs->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                        struct string *content = cs_request_string_from_file("res/checking_service/work_time/update/update_time_start_date_start.json", FILE_INNER);
                        string_replace(content, "{ID}", id->ptr);
                        string_replace(content, "{USER_NAME}", user_name->ptr);
                        string_replace(content, "{DATE_START}", current_time->ptr);
                        string_replace_int(content, "{VERSION_NUMBER}", _version);
                        string_replace(content, "{TIME_START}", smart_object_get_string(cud->obj, qskey(&__key_time_start__), SMART_GET_REPLACE_IF_WRONG_TYPE)->ptr);

                        struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                                qskey(es_version_code), qskey(es_pass));
                        string_free(content);

                        cs_request_alloc(cs->es_server_requester, request_data,
                                (cs_request_callback)__update_callback, cud);
                        string_free(id);
                        string_free(current_time);
                }
        } else {
                /*
                 * create new work time. offset date start to the next 8h
                 * to prevent new user checked as late
                 */
                struct string *current_time = offset_time_to_string(28800);

                struct string *id = string_alloc(0);
                string_cat_string(id, user_name);
                string_cat(id, qlkey("_work_time_now"));

                struct string *es_version_code = smart_object_get_string(cs->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct string *es_pass = smart_object_get_string(cs->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *content = cs_request_string_from_file("res/checking_service/work_time/create/create.json", FILE_INNER);
                string_replace(content, "{ID}", id->ptr);
                string_replace(content, "{PARENT_ID}", user_name->ptr);
                string_replace(content, "{DATE_START}", current_time->ptr);
                string_replace(content, "{TIME_START}", smart_object_get_string(cud->obj, qskey(&__key_time_start__), SMART_GET_REPLACE_IF_WRONG_TYPE)->ptr);
                string_replace(content, "{DATE_END}", "9999-01-01 00:00:00");

                struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                        qskey(es_version_code), qskey(es_pass));
                string_free(content);

                cs_request_alloc(cs->es_server_requester, request_data,
                        (cs_request_callback)__create_callback, cud);
                string_free(id);
                string_free(current_time);
        }
}

void checking_service_process_work_time_new_start_v1(struct cs_server_callback_user_data *cud)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)cud->p->user_head.next - offsetof(struct checking_service , server));

        struct string *user_name = smart_object_get_string(cud->obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *id = string_alloc(0);
        string_cat_string(id, user_name);
        string_cat(id, qlkey("_work_time_now"));

        struct string *es_version_code = smart_object_get_string(cs->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = smart_object_get_string(cs->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *content = cs_request_string_from_file("res/checking_service/work_time/search/search_specific.json", FILE_INNER);
        string_replace(content, "{ID}", id->ptr);

        struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                qskey(es_version_code), qskey(es_pass));
        string_free(content);

        cs_request_alloc(cs->es_server_requester, request_data,
                (cs_request_callback)__search_callback, cud);
        string_free(id);
}

void checking_service_process_work_time_new_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        if( ! __validate_input(p, fd, mask, obj)) {
                return;
        }

        struct cs_server_callback_user_data *cud = cs_server_callback_user_data_alloc(p, fd, mask, obj);

        checking_service_process_work_time_new_start_v1(cud);
}
