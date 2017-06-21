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

        struct string *admin_name = smart_object_get_string(obj, qskey(&__key_admin_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(admin_name);
        if(admin_name->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide admin name!"));
                return 0;
        }
        if(admin_name->len < 6) {
                __response_invalid_data(p, fd, mask, obj, qlkey("adminname needs have at least 6 characters!"));
                return 0;
        }
        if(!common_username_valid(admin_name->ptr, admin_name->len)) {
                __response_invalid_data(p, fd, mask, obj, qlkey("adminname contains invalid character!"));
                return 0;
        }

        struct string *admin_pass = smart_object_get_string(obj, qskey(&__key_admin_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        string_trim(admin_pass);
        if(admin_pass->len == 0) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Please provide admin password!"));
                return 0;
        }
        if(admin_pass->len < 8) {
                __response_invalid_data(p, fd, mask, obj, qlkey("Admin Password need have at least 8 characters!"));
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

static void __search_callback(struct cs_server_callback_user_data *cud, struct smart_object *recv)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)cud->p->user_head.next - offsetof(struct checking_service , server));

        struct smart_object *data = smart_object_get_object(recv, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct smart_object *hits = smart_object_get_object(data, qlkey("hits"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int total = smart_object_get_int(hits, qlkey("total"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        if(total == 1) {
                checking_service_process_work_time_new_start_v1(cud);
        } else {
                __response_invalid_data(cud->p, cud->fd, cud->mask,  cud->obj, qlkey("permission denied!"));
                cs_server_callback_user_data_free(cud);
        }
}

static void __search(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        struct checking_service *cs = (struct checking_service *)
                ((char *)p->user_head.next - offsetof(struct checking_service , server));

        struct string *admin_name = smart_object_get_string(obj, qskey(&__key_admin_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *admin_pass = smart_object_get_string(obj, qskey(&__key_admin_pass__), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *es_version_code = smart_object_get_string(cs->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = smart_object_get_string(cs->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct string *content = cs_request_string_from_file("res/checking_service/user/search_valid/search_by_username_has_permission_add_work_time.json", FILE_INNER);
        string_replace(content, "{USER_NAME}", admin_name->ptr);
        string_replace(content, "{USER_PASS}", admin_pass->ptr);

        struct smart_object *request_data = cs_request_data_from_string(qskey(content),
                qskey(es_version_code), qskey(es_pass));
        string_free(content);

        struct cs_server_callback_user_data *cud = cs_server_callback_user_data_alloc(p, fd, mask, obj);

        cs_request_alloc(cs->es_server_requester, request_data,
                (cs_request_callback)__search_callback, cud);
}


void checking_service_process_work_time_new_by_user_v1(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        if( ! __validate_input(p, fd, mask, obj)) {
                return;
        }

        __search(p, fd, mask, obj);
}
