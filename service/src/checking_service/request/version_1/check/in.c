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

#include <s2/s2_helper.h>
#include <s2/s2_cell_union.h>
#include <s2/s2_cell_id.h>
#include <s2/s2_lat_lng.h>
#include <s2/s2_point.h>

#include <cherry/time.h>

#include <cherry/crypt/md5.h>

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

        /*
         * return local ip and local port
         */
        struct smart_object *data_res = smart_object_get_object(res, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        pthread_mutex_lock(&__shared_ram_config_mutex__);
        struct string *__ram_local_ip = smart_object_get_string(__shared_ram_config__, qskey(&__key_local_ip__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int __ram_local_port = smart_object_get_int(__shared_ram_config__, qskey(&__key_local_port__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        smart_object_set_string(data_res, qskey(&__key_local_ip__), qskey(__ram_local_ip));
        smart_object_set_int(data_res, qskey(&__key_local_port__), __ram_local_port);
        pthread_mutex_unlock(&__shared_ram_config_mutex__);

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

        struct string *user_pass = smart_object_get_string(obj, qskey(&__key_user_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(user_pass);
        if(user_pass->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide password!"));
                return 0;
        }
        if(user_pass->len < 8) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Password need have at least 8 characters!"));
                return 0;
        }

        struct string *device_id = smart_object_get_string(obj, qskey(&__key_device_id__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(device_id);
        if(device_id->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please device id!"));
                return 0;
        }

        return 1;
}

static void __create_check_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *result = smart_object_get_string(data, qlkey("result"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        if(strcmp(result->ptr, "created") == 0) {
                struct smart_object *res = smart_object_alloc();
                smart_object_set_long(res, qskey(&__key_request_id__),
                        smart_object_get_long(cud->obj, qskey(&__key_request_id__), 0));
                smart_object_set_bool(res, qskey(&__key_result__), 1);
                smart_object_set_string(res, qskey(&__key_message__), qlkey("check in successfully!"));

                struct smart_object *res_data = smart_object_get_object(res, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct string *check_in = smart_object_get_string(cud->obj, qskey(&__key_check_in__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                smart_object_set_string(res_data, qskey(&__key_check_in__), qskey(check_in));

                struct string *d        = smart_object_to_json(res);
                cs_server_send_to_client(cud->p, cud->fd, cud->mask, d->ptr, d->len, 0);
                string_free(d);
                smart_object_free(res);
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

static void __check_in(struct cs_server_callback_user_data *cud)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)cud->p->user_head.next - offsetof(struct checking_service , server));

        struct string *user_name = smart_object_get_string(cud->obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *checkid = string_alloc_chars(qskey(user_name));
        string_cat(checkid, qlkey("_check_"));
        struct string *temp = current_time_to_string(TIME_FORMAT_YY_MM_DD);
        string_replace(temp, "-", "_");
        string_cat_string(checkid, temp);
        string_free(temp);

        struct string *today_time = current_time_to_string(TIME_FORMAT_YY_MM_DD_HH_MM_SS);

        /*
         * remember check in time
         */
        smart_object_set_string(cud->obj, qskey(&__key_check_in__), qskey(today_time));

        struct string *es_version_code = smart_object_get_string(cs->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = smart_object_get_string(cs->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *content = cs_request_string_from_file("res/checking_service/check/create/create.json", FILE_INNER);
        string_replace(content, "{ID}", checkid->ptr);
        string_replace(content, "{PARENT_ID}", user_name->ptr);
        string_replace(content, "{CHECK_IN}", today_time->ptr);
        string_replace(content, "{CHECK_OUT}", "9999-01-01 00:00:00");
        string_free(checkid);
        string_free(today_time);

        struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                qskey(es_version_code), qskey(es_pass));
        string_free(content);

        cs_request_alloc(cs->es_server_requester, request_data,
                (cs_request_callback)__create_check_callback, cud);
}

static void __search_check_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)cud->p->user_head.next - offsetof(struct checking_service , server));

        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct smart_object *hits = smart_object_get_object(data, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int total = smart_object_get_int(hits, qlkey("total"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(total == 1) {
                struct smart_array *hits_hits     = smart_object_get_array(hits, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct smart_object *check        = smart_array_get_object(hits_hits, 0, SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct smart_object *_source      = smart_object_get_object(check, qlkey("_source"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct smart_object *res = smart_object_alloc();
                smart_object_set_long(res, qskey(&__key_request_id__),
                        smart_object_get_long(cud->obj, qskey(&__key_request_id__), 0));
                smart_object_set_bool(res, qskey(&__key_result__), 1);
                smart_object_set_string(res, qskey(&__key_message__), qlkey("check in successfully!"));

                struct smart_object *res_data = smart_object_get_object(res, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct string *check_in = smart_object_get_string(_source, qskey(&__key_check_in__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct string *check_out = smart_object_get_string(_source, qskey(&__key_check_out__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                smart_object_set_string(res_data, qskey(&__key_check_in__), qskey(check_in));
                smart_object_set_string(res_data, qskey(&__key_check_out__), qskey(check_out));

                struct string *d        = smart_object_to_json(res);
                cs_server_send_to_client(cud->p, cud->fd, cud->mask, d->ptr, d->len, 0);
                string_free(d);
                smart_object_free(res);

                cs_server_callback_user_data_free(cud);
        } else {
                struct smart_object *latlng     = smart_object_get_object(cud->obj, qskey(&__key_latlng__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                double lat = smart_object_get_double(latlng, qskey(&__key_lat__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                double lon = smart_object_get_double(latlng, qskey(&__key_lon__), SMART_GET_REPLACE_IF_WRONG_TYPE);

                double service_lat = smart_object_get_double(cs->config, qskey(&__key_lat__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                double service_lon = smart_object_get_double(cs->config, qskey(&__key_lon__), SMART_GET_REPLACE_IF_WRONG_TYPE);

                double range                    = 20;
                struct s2_cell_union *scu       = s2_helper_get_cell_union(cs->s2_helper, service_lat, service_lon, range);
                if(s2_cell_union_contain(scu, lat, lon)) {
                        __check_in(cud);
                } else {
                        /*
                         * check if client is local
                         */
                        struct string *local_code = smart_object_get_string(cud->obj, qskey(&__key_local_code__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                        pthread_mutex_lock(&__shared_ram_config_mutex__);
                        struct string *__ram_local_code = smart_object_get_string(__shared_ram_config__, qskey(&__key_local_code__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                        string_trim(local_code);
                        string_trim(__ram_local_code);
                        int same = strcmp(local_code->ptr, __ram_local_code->ptr) == 0 && local_code->len;
                        pthread_mutex_unlock(&__shared_ram_config_mutex__);

                        if(same) {
                                __check_in(cud);
                        } else {
                                __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj, qlkey("checkin error!"));
                                cs_server_callback_user_data_free(cud);
                        }
                }

                s2_cell_union_free(scu);
        }
}

static void __search_device_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)cud->p->user_head.next - offsetof(struct checking_service , server));

        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct smart_object *hits = smart_object_get_object(data, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int total = smart_object_get_int(hits, qlkey("total"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(total == 1) {
                struct string *user_name = smart_object_get_string(cud->obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *checkid = string_alloc_chars(qskey(user_name));
                string_cat(checkid, qlkey("_check_"));
                struct string *temp = current_time_to_string(TIME_FORMAT_YY_MM_DD);
                string_replace(temp, "-", "_");
                string_cat_string(checkid, temp);
                string_free(temp);

                struct string *es_version_code = smart_object_get_string(cs->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct string *es_pass = smart_object_get_string(cs->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *content = cs_request_string_from_file("res/checking_service/check/search/search_by_id.json", FILE_INNER);
                string_replace(content, "{CHECK_ID}", checkid->ptr);
                string_free(checkid);

                struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                        qskey(es_version_code), qskey(es_pass));
                string_free(content);

                cs_request_alloc(cs->es_server_requester, request_data,
                        (cs_request_callback)__search_check_callback, cud);

        } else {
                __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj, qlkey("device is not registered!"));
                cs_server_callback_user_data_free(cud);
        }
}

static void __search_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)cud->p->user_head.next - offsetof(struct checking_service , server));

        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct smart_object *hits = smart_object_get_object(data, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int total = smart_object_get_int(hits, qlkey("total"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(total == 1) {
                struct string *user_name = smart_object_get_string(cud->obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct string *user_pass = smart_object_get_string(cud->obj, qskey(&__key_user_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct string *device_id = smart_object_get_string(cud->obj, qskey(&__key_device_id__), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *id               = string_alloc_chars(qlkey("device_"));
                struct string *encrypt_did      = md5_string(qskey(device_id));
                string_cat_string(id, encrypt_did);
                string_free(encrypt_did);

                struct string *es_version_code = smart_object_get_string(cs->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct string *es_pass = smart_object_get_string(cs->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *content = cs_request_string_from_file("res/checking_service/user/search_valid/search_by_username_password_device.json", FILE_INNER);
                string_replace(content, "{USER_NAME}", user_name->ptr);
                string_replace(content, "{USER_PASS}", user_pass->ptr);
                string_replace(content, "{DEVICE_ID}", id->ptr);
                string_free(id);

                struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                        qskey(es_version_code), qskey(es_pass));
                string_free(content);

                cs_request_alloc(cs->es_server_requester, request_data,
                        (cs_request_callback)__search_device_callback, cud);
        } else {
                __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj, qlkey("user is not registered!"));
                cs_server_callback_user_data_free(cud);
        }
}

static void __search(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)p->user_head.next - offsetof(struct checking_service , server));

        struct string *user_name = smart_object_get_string(obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *user_pass = smart_object_get_string(obj, qskey(&__key_user_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *es_version_code = smart_object_get_string(cs->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = smart_object_get_string(cs->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *content = cs_request_string_from_file("res/checking_service/user/search_valid/search_by_username.json", FILE_INNER);
        string_replace(content, "{USER_NAME}", user_name->ptr);
        string_replace(content, "{USER_PASS}", user_pass->ptr);

        struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                qskey(es_version_code), qskey(es_pass));
        string_free(content);

        struct cs_server_callback_user_data *cud = cs_server_callback_user_data_alloc(p, fd, mask, obj);

        cs_request_alloc(cs->es_server_requester, request_data,
                (cs_request_callback)__search_callback, cud);
}

void checking_service_process_check_in_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        if( ! __validate_input(p, fd, mask, obj)) {
                return;
        }

        __search(p, fd, mask, obj);
}