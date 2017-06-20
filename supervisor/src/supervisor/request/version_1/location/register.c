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

        struct string *location_name = smart_object_get_string(obj, qskey(&__key_location_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(location_name);
        if(location_name->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide location name!\n"));
                return 0;
        }

        struct string *ip       = smart_object_get_string(obj, qskey(&__key_ip__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(ip);
        if(ip->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide ip address!\n"));
                return 0;
        }
        if(!common_is_ip(ip->ptr)) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Input ip address is invalid!\n"));
                return 0;
        }

        return 1;
}
/*
 *
 */
static void __create_new_location_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *result = smart_object_get_string(data, qlkey("result"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        if(strcmp(result->ptr, "created") == 0) {
                __response_success(cud->p, cud->fd, cud->mask,
                        cud->obj, qlkey("location is created successfully!\n"));
        } else {
                int status = smart_object_get_int(data, qlkey("status"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                if(status == 409) {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,
                                cud->obj, qlkey("location is created by other!\n"));
                } else {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,
                                cud->obj, qlkey("server error!\n"));
                }
        }
        cs_server_callback_user_data_free(cud);
}

static void __create_new_location(struct cs_server_callback_user_data *cud, int location_id)
{
        struct supervisor *supervisor = (struct supervisor *)
                ((char *)cud->p->user_head.next - offsetof(struct supervisor , server));

        struct string *user_name        = smart_object_get_string(cud->obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *device_id        = smart_object_get_string(cud->obj, qskey(&__key_device_id__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *location_name    = smart_object_get_string(cud->obj, qskey(&__key_location_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *id               = string_alloc_chars(qskey(user_name));
        string_cat(id, qlkey("_location_"));
        string_cat_int(id, location_id);

        struct string *ip               = smart_object_get_string(cud->obj, qskey(&__key_ip__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        u16 port                        = smart_object_get_short(cud->obj, qskey(&__key_port__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct smart_object *latlng       = smart_object_get_object(cud->obj, qskey(&__key_latlng__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        double lat = smart_object_get_double(latlng, qskey(&__key_lat__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        double lon = smart_object_get_double(latlng, qskey(&__key_lon__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        /*
         * create new location
         */
        struct string *es_version_code = smart_object_get_string(cud->p->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = smart_object_get_string(cud->p->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *content = cs_request_string_from_file("res/supervisor/location/create/create.json", FILE_INNER);
        string_replace(content, "{ID}", id->ptr);
        string_replace(content, "{PARENT_ID}", user_name->ptr);
        string_replace(content, "{IP}", ip->ptr);
        string_replace_int(content, "{PORT}", port);
        string_replace(content, "{LOCATION_NAME}", location_name->ptr);
        string_replace(content, "{DEVICE_ID}", device_id->ptr);
        string_replace_double(content, "{LAT}", lat);
        string_replace_double(content, "{LON}", lon);

        struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                qskey(es_version_code), qskey(es_pass));
        string_free(content);

        cs_request_alloc(supervisor->es_server_requester, request_data,
                (cs_request_callback)__create_new_location_callback, cud);

        string_free(id);
}

/*
 *
 */
static void __search_all_location_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct smart_object *hits = smart_object_get_object(data, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        int total = smart_object_get_int(hits, qlkey("total"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        /*
         * make sure that service capacity is included in cud->obj from previous state
         */
        int capacity = smart_object_get_int(cud->obj, qskey(&__key_capacity__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(capacity == 0) {
                __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                        qlkey("Not enough capacity to allocate new location!\n"));
                cs_server_callback_user_data_free(cud);
                return;
        }

        if(total == 0) {
                __create_new_location(cud, 1);
        } else {
                struct smart_array *hits_hits = smart_object_get_array(hits, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                if(total < capacity) {
                        struct array *nums = array_alloc(sizeof(int), NO_ORDERED);
                        int i, j;
                        for_i(i, capacity) {
                                int id = i + 1;
                                array_push(nums, &id);
                        }
                        /*
                         * find available slot
                         */
                        for_i(i, total) {
                                struct smart_object *location     = smart_array_get_object(hits_hits, i, SMART_GET_REPLACE_IF_WRONG_TYPE);
                                struct string *_id              = smart_object_get_string(location, qlkey("_id"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                                char *ptr = _id->ptr + _id->len - 1;
                                /*
                                 * location id has format {service_user}_location_{number}
                                 */
                                while((*ptr) != '_') {
                                        ptr--;
                                }
                                ptr++;
                                int id = atoi(ptr);
                                for_i(j, nums->len) {
                                        int n_id = array_get(nums, int, j);
                                        if(n_id == id) {
                                                array_remove(nums, j);
                                                break;
                                        }
                                }
                        }

                        int id = array_get(nums, int , 0);
                        __create_new_location(cud, id);
                        array_free(nums);
                } else {
                        /*
                         * return list slot
                         */
                         __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                                 qlkey("Not enough capacity to allocate new location!\n"));
                         cs_server_callback_user_data_free(cud);
                         return;
                }
        }
}

/*
 *
 */
static void __update_immediately_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int _version = smart_object_get_int(data, qlkey("_version"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int _old_version = smart_object_get_int(cud->obj, qlkey("_version"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(_version > 0 && _version - 1 == _old_version) {
                __response_success(cud->p, cud->fd, cud->mask,  cud->obj,
                        qlkey("location is updated successfully!\n"));
        } else {
                if(_version == 0) {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                                qlkey("error!\n"));
                } else if(_old_version != _version) {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                                qlkey("error!\n"));
                } else if(_old_version == _version) {
                        __response_success(cud->p, cud->fd, cud->mask,  cud->obj,
                                qlkey("location is updated successfully!\n"));
                } else {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                                qlkey("error!\n"));
                }
        }

        cs_server_callback_user_data_free(cud);
}

static void __get_me_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct supervisor *supervisor = (struct supervisor *)
                ((char *)cud->p->user_head.next - offsetof(struct supervisor , server));

        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct smart_object *hits = smart_object_get_object(data, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        int total = smart_object_get_int(hits, qlkey("total"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *user_name        = smart_object_get_string(cud->obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(total == 1) {
                /*
                 * replace immediately
                 */
                struct smart_array *hits_hits     = smart_object_get_array(hits, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct smart_object *location     = smart_array_get_object(hits_hits, 0, SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *_id              = smart_object_get_string(location, qlkey("_id"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                int version                     = smart_object_get_int(location, qlkey("_version"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct smart_object *_source      = smart_object_get_object(location, qlkey("_source"), SMART_GET_REPLACE_IF_WRONG_TYPE);


                struct string *ip               = smart_object_get_string(cud->obj, qskey(&__key_ip__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                u16 port                        = smart_object_get_short(cud->obj, qskey(&__key_port__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct smart_object *latlng       = smart_object_get_object(cud->obj, qskey(&__key_latlng__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                double lat = smart_object_get_double(latlng, qskey(&__key_lat__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                double lon = smart_object_get_double(latlng, qskey(&__key_lon__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                /*
                 * store current _version to check collision
                 */
                smart_object_set_int(cud->obj, qlkey("_version"), version);

                struct string *es_version_code = smart_object_get_string(cud->p->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct string *es_pass = smart_object_get_string(cud->p->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *content = cs_request_string_from_file("res/supervisor/location/create/update.json", FILE_INNER);
                string_replace_int(content, "{VERSION_NUMBER}", version);
                string_replace(content, "{ID}", _id->ptr);
                string_replace(content, "{SERVICE_ID}", user_name->ptr);
                string_replace(content, "{IP}", ip->ptr);
                string_replace_int(content, "{PORT}", port);
                string_replace_double(content, "{LAT}", lat);
                string_replace_double(content, "{LON}", lon);

                struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                        qskey(es_version_code), qskey(es_pass));
                string_free(content);

                cs_request_alloc(supervisor->es_server_requester, request_data,
                        (cs_request_callback)__update_immediately_callback, cud);
        } else {
                /*
                 * search for available slot
                 */
                struct string *es_version_code = smart_object_get_string(cud->p->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct string *es_pass = smart_object_get_string(cud->p->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *content = cs_request_string_from_file("res/supervisor/location/create/search_by_service.json", FILE_INNER);
                string_replace(content, "{SERVICE_ID}", user_name->ptr);

                struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                        qskey(es_version_code), qskey(es_pass));
                string_free(content);

                cs_request_alloc(supervisor->es_server_requester, request_data,
                        (cs_request_callback)__search_all_location_callback, cud);
        }
}

static void __get_service_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct supervisor *supervisor = (struct supervisor *)
                ((char *)cud->p->user_head.next - offsetof(struct supervisor , server));

        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct smart_object *hits = smart_object_get_object(data, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        int total = smart_object_get_int(hits, qlkey("total"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(total == 1) {
                struct smart_array *hits_hits     = smart_object_get_array(hits, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct smart_object *service     = smart_array_get_object(hits_hits, 0, SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *_id              = smart_object_get_string(service, qlkey("_id"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                int _version                     = smart_object_get_int(service, qlkey("_version"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct smart_object *_source      = smart_object_get_object(service, qlkey("_source"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *user_name        = smart_object_get_string(cud->obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *device_id        = smart_object_get_string(cud->obj, qskey(&__key_device_id__), SMART_GET_REPLACE_IF_WRONG_TYPE);

                /*
                 * store capacity for next allocation
                 */
                int capacity            = smart_object_get_int(_source, qskey(&__key_capacity__), SMART_GET_REPLACE_IF_WRONG_TYPE);
                smart_object_set_int(cud->obj, qskey(&__key_capacity__), capacity);
                /*
                 * search me
                 */
                struct string *es_version_code = smart_object_get_string(cud->p->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                struct string *es_pass = smart_object_get_string(cud->p->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct string *content = cs_request_string_from_file("res/supervisor/location/create/search_by_service_by_device.json", FILE_INNER);

                string_replace(content, "{SERVICE_ID}", user_name->ptr);
                string_replace(content, "{DEVICE_ID}", device_id->ptr);

                struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                        qskey(es_version_code), qskey(es_pass));
                string_free(content);

                cs_request_alloc(supervisor->es_server_requester, request_data,
                        (cs_request_callback)__get_me_callback, cud);
        } else {
                __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                        qlkey("user name is not registered or wrong password!"));
                cs_server_callback_user_data_free(cud);
        }
}

static void __get_service(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        struct supervisor *supervisor = (struct supervisor *)
                ((char *)p->user_head.next - offsetof(struct supervisor , server));

        struct string *user_name = smart_object_get_string(obj, qskey(&__key_user_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *user_pass = smart_object_get_string(obj, qskey(&__key_user_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *es_version_code = smart_object_get_string(p->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = smart_object_get_string(p->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *content = cs_request_string_from_file("res/supervisor/service/search_valid/search_by_username.json", FILE_INNER);
        string_replace(content, "{USER_NAME}", user_name->ptr);
        string_replace(content, "{USER_PASS}", user_pass->ptr);

        struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                qskey(es_version_code), qskey(es_pass));
        string_free(content);

        struct cs_server_callback_user_data *cud = cs_server_callback_user_data_alloc(p, fd, mask, obj);

        cs_request_alloc(supervisor->es_server_requester, request_data,
                (cs_request_callback)__get_service_callback, cud);
}

void supervisor_process_location_register_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        if(!__validate_input(p, fd, mask, obj)) {
                return;
        }

        __get_service(p, fd, mask, obj);
}
