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

        struct string *user_pass = sfs_object_get_string(obj, qskey(&__key_user_pass__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(user_pass);
        if(user_pass->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide user pass!\n"));
                return 0;
        }

        struct string *device_id = sfs_object_get_string(obj, qskey(&__key_device_id__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(device_id);
        if(device_id->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide device id!\n"));
                return 0;
        }

        struct string *location_name = sfs_object_get_string(obj, qskey(&__key_location_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(location_name);
        if(location_name->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide location name!\n"));
                return 0;
        }

        struct string *ip       = sfs_object_get_string(obj, qskey(&__key_ip__), SFS_GET_REPLACE_IF_WRONG_TYPE);
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
static void __create_new_location_callback(struct callback_user_data *cud, struct sfs_object *recv)
{
        struct sfs_object *data = sfs_object_get_object(recv, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct string *result = sfs_object_get_string(data, qlkey("result"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        if(strcmp(result->ptr, "created") == 0) {
                __response_success(cud->p, cud->fd, cud->mask,
                        cud->obj, qlkey("location is created successfully!\n"));
        } else {
                int status = sfs_object_get_int(data, qlkey("status"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                if(status == 409) {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,
                                cud->obj, qlkey("location is created by other!\n"));
                } else {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,
                                cud->obj, qlkey("server error!\n"));
                }
        }
        callback_user_data_free(cud);
}

static void __create_new_location(struct callback_user_data *cud, int location_id)
{
        struct string *user_name        = sfs_object_get_string(cud->obj, qskey(&__key_user_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct string *device_id        = sfs_object_get_string(cud->obj, qskey(&__key_device_id__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct string *location_name    = sfs_object_get_string(cud->obj, qskey(&__key_location_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        struct string *id               = string_alloc_chars(qskey(user_name));
        string_cat(id, qlkey("_location_"));
        string_cat_int(id, location_id);
        /*
         * create new location
         */
        struct string *es_version_code = sfs_object_get_string(cud->p->config, qlkey("es_version_code"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = sfs_object_get_string(cud->p->config, qlkey("es_pass"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct sfs_object *request_data = cs_request_data_from_file("res/supervisor/location/create.json", FILE_INNER,
                qskey(es_version_code), qskey(es_pass));

        struct string *path     = sfs_object_get_string(request_data, qskey(&__key_path__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        string_replace(path, "{ID}", id->ptr);
        string_replace(path, "{PARENT_ID}", user_name->ptr);

        struct sfs_object *request_data_data = sfs_object_get_object(request_data, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        struct string *ip               = sfs_object_get_string(cud->obj, qskey(&__key_ip__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        u16 port                        = sfs_object_get_short(cud->obj, qskey(&__key_port__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct sfs_object *latlng       = sfs_object_get_object(cud->obj, qskey(&__key_latlng__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        /*
         * ensure latlng will contain lat and lon
         */
        sfs_object_get_double(latlng, qskey(&__key_lat__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        sfs_object_get_double(latlng, qskey(&__key_lon__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        sfs_object_set_string(request_data_data, qskey(&__key_ip__), qskey(ip));
        sfs_object_set_short(request_data_data, qskey(&__key_port__), port);
        sfs_object_set_object(request_data_data, qskey(&__key_latlng__), sfs_object_clone(latlng));
        sfs_object_set_string(request_data_data, qskey(&__key_device_id__), qskey(device_id));
        sfs_object_set_string(request_data_data, qskey(&__key_location_name__), qskey(location_name));

        cs_request_alloc(cud->p->es_server_requester, request_data,
                (cs_request_callback)__create_new_location_callback, cud);

        string_free(id);
}

/*
 *
 */
static void __search_all_location_callback(struct callback_user_data *cud, struct sfs_object *recv)
{
        struct sfs_object *data = sfs_object_get_object(recv, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        struct sfs_object *hits = sfs_object_get_object(data, qlkey("hits"), SFS_GET_REPLACE_IF_WRONG_TYPE);

        int total = sfs_object_get_int(hits, qlkey("total"), SFS_GET_REPLACE_IF_WRONG_TYPE);

        /*
         * make sure that service capacity is included in cud->obj from previous state
         */
        int capacity = sfs_object_get_int(cud->obj, qskey(&__key_capacity__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        if(capacity == 0) {
                __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                        qlkey("Not enough capacity to allocate new location!\n"));
                callback_user_data_free(cud);
                return;
        }

        if(total == 0) {
                __create_new_location(cud, 1);
        } else {
                struct sfs_array *hits_hits = sfs_object_get_array(hits, qlkey("hits"), SFS_GET_REPLACE_IF_WRONG_TYPE);
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
                                struct sfs_object *location     = sfs_array_get_object(hits_hits, i, SFS_GET_REPLACE_IF_WRONG_TYPE);
                                struct string *_id              = sfs_object_get_string(location, qlkey("_id"), SFS_GET_REPLACE_IF_WRONG_TYPE);

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
                         callback_user_data_free(cud);
                         return;
                }
        }
}

/*
 *
 */
static void __update_immediately_callback(struct callback_user_data *cud, struct sfs_object *recv)
{
        struct sfs_object *data = sfs_object_get_object(recv, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        int _version = sfs_object_get_int(data, qlkey("_version"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        int _old_version = sfs_object_get_int(cud->obj, qlkey("_version"), SFS_GET_REPLACE_IF_WRONG_TYPE);

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

        callback_user_data_free(cud);
}

static void __get_me_callback(struct callback_user_data *cud, struct sfs_object *recv)
{
        struct sfs_object *data = sfs_object_get_object(recv, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        struct sfs_object *hits = sfs_object_get_object(data, qlkey("hits"), SFS_GET_REPLACE_IF_WRONG_TYPE);

        int total = sfs_object_get_int(hits, qlkey("total"), SFS_GET_REPLACE_IF_WRONG_TYPE);

        if(total == 1) {
                /*
                 * replace immediately
                 */
                struct string *user_name        = sfs_object_get_string(cud->obj, qskey(&__key_user_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);

                struct sfs_array *hits_hits     = sfs_object_get_array(hits, qlkey("hits"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                struct sfs_object *location     = sfs_array_get_object(hits_hits, 0, SFS_GET_REPLACE_IF_WRONG_TYPE);

                struct string *_id              = sfs_object_get_string(location, qlkey("_id"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                int version                     = sfs_object_get_int(location, qlkey("_version"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                struct sfs_object *_source      = sfs_object_get_object(location, qlkey("_source"), SFS_GET_REPLACE_IF_WRONG_TYPE);

                /*
                 * store current _version to check collision
                 */
                sfs_object_set_int(cud->obj, qlkey("_version"), version);

                struct string *es_version_code = sfs_object_get_string(cud->p->config, qlkey("es_version_code"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                struct string *es_pass = sfs_object_get_string(cud->p->config, qlkey("es_pass"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                struct sfs_object *request_data = cs_request_data_from_file("res/supervisor/location/update_ip.json", FILE_INNER,
                        qskey(es_version_code), qskey(es_pass));

                struct string *path     = sfs_object_get_string(request_data, qskey(&__key_path__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                string_replace_int(path, "{VERSION_NUMBER}", version);
                string_replace(path, "{ID}", _id->ptr);
                string_replace(path, "{SERVICE_ID}", user_name->ptr);

                struct sfs_object *request_data_data = sfs_object_get_object(request_data, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                struct sfs_object *doc          = sfs_object_get_object(request_data_data, qlkey("doc"), SFS_GET_REPLACE_IF_WRONG_TYPE);

                struct string *ip               = sfs_object_get_string(cud->obj, qskey(&__key_ip__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                u16 port                        = sfs_object_get_short(cud->obj, qskey(&__key_port__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                struct sfs_object *latlng       = sfs_object_get_object(cud->obj, qskey(&__key_latlng__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                /*
                 * ensure latlng will contain lat and lon
                 */
                sfs_object_get_double(latlng, qskey(&__key_lat__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                sfs_object_get_double(latlng, qskey(&__key_lon__), SFS_GET_REPLACE_IF_WRONG_TYPE);

                sfs_object_set_string(doc, qskey(&__key_ip__), qskey(ip));
                sfs_object_set_short(doc, qskey(&__key_port__), port);
                sfs_object_set_object(doc, qskey(&__key_latlng__), sfs_object_clone(latlng));

                cs_request_alloc(cud->p->es_server_requester, request_data,
                        (cs_request_callback)__update_immediately_callback, cud);
        } else {
                /*
                 * search for available slot
                 */
                struct string *es_version_code = sfs_object_get_string(cud->p->config, qlkey("es_version_code"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                struct string *es_pass = sfs_object_get_string(cud->p->config, qlkey("es_pass"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                struct sfs_object *request_data = cs_request_data_from_file("res/supervisor/location/search_by_service.json", FILE_INNER,
                        qskey(es_version_code), qskey(es_pass));

                struct string *user_name        = sfs_object_get_string(cud->obj, qskey(&__key_user_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);

                struct string *path     = sfs_object_get_string(request_data, qskey(&__key_path__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                string_replace(path, "{SERVICE_ID}", user_name->ptr);

                cs_request_alloc(cud->p->es_server_requester, request_data,
                        (cs_request_callback)__search_all_location_callback, cud);
        }
}

static void __get_service_callback(struct callback_user_data *cud, struct sfs_object *recv)
{
        struct sfs_object *data = sfs_object_get_object(recv, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        u8 found = sfs_object_get_bool(data, qlkey("found"), SFS_GET_REPLACE_IF_WRONG_TYPE);

        if(found) {
                struct string *user_name        = sfs_object_get_string(cud->obj, qskey(&__key_user_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);

                struct string *device_id        = sfs_object_get_string(cud->obj, qskey(&__key_device_id__), SFS_GET_REPLACE_IF_WRONG_TYPE);

                int _version                    = sfs_object_get_int(data, qlkey("_version"), SFS_GET_REPLACE_IF_WRONG_TYPE);

                struct sfs_object *_source      = sfs_object_get_object(data, qlkey("_source"), SFS_GET_REPLACE_IF_WRONG_TYPE);

                struct string *user_pass        = sfs_object_get_string(_source, qskey(&__key_user_pass__), SFS_GET_REPLACE_IF_WRONG_TYPE);

                struct string *input_pass       = sfs_object_get_string(cud->obj, qskey(&__key_user_pass__), SFS_GET_REPLACE_IF_WRONG_TYPE);

                if(strcmp(input_pass->ptr, user_pass->ptr) == 0) {
                        /*
                         * store capacity for next allocation
                         */
                        int capacity            = sfs_object_get_int(_source, qskey(&__key_capacity__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                        sfs_object_set_int(cud->obj, qskey(&__key_capacity__), capacity);
                        /*
                         * search me
                         */
                        struct string *es_version_code = sfs_object_get_string(cud->p->config, qlkey("es_version_code"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                        struct string *es_pass = sfs_object_get_string(cud->p->config, qlkey("es_pass"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                        struct sfs_object *request_data = cs_request_data_from_file("res/supervisor/location/search_by_service_by_device.json", FILE_INNER,
                                qskey(es_version_code), qskey(es_pass));

                        struct string *path = sfs_object_get_string(request_data, qskey(&__key_path__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                        string_replace(path, "{SERVICE_ID}", user_name->ptr);

                        struct sfs_object *request_data_data = sfs_object_get_object(request_data, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                        struct sfs_object *query = sfs_object_get_object(request_data_data, qlkey("query"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                        struct sfs_object *match = sfs_object_get_object(query, qlkey("match"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                        sfs_object_set_string(match, qskey(&__key_device_id__), qskey(device_id));

                        cs_request_alloc(cud->p->es_server_requester, request_data,
                                (cs_request_callback)__get_me_callback, cud);
                } else {
                        __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                                qlkey("wrong password!\n"));
                        callback_user_data_free(cud);
                }
        } else {
                __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj,
                        qlkey("user name is not registered!\n"));
                callback_user_data_free(cud);
        }
}

static void __get_service(struct supervisor *p, int fd, u32 mask, struct sfs_object *obj)
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
                (cs_request_callback)__get_service_callback, cud);
}

void supervisor_process_register_location_v1(struct supervisor *p, int fd, u32 mask, struct sfs_object *obj)
{
        if(!__validate_input(p, fd, mask, obj)) {
                return;
        }

        __get_service(p, fd, mask, obj);
}
