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
#include <cherry/array.h>
#include <cherry/map.h>
#include <cherry/stdio.h>
#include <cherry/memory.h>
#include <cherry/string.h>
#include <cherry/stdlib.h>
#include <cherry/unistd.h>
#include <cherry/ctype.h>
#include <cherry/time.h>
#include <s2/types.h>

#include <smartfox/data.h>
#include <service/checking_service.h>

#include <common/command.h>
#include <common/key.h>
#include <common/request.h>
#include <common/util.h>

#include <pthread.h>

#define BUFFER_LEN 1024

void checking_service_check_in(struct checking_service *p, struct smart_object *in)
{
        char line[BUFFER_LEN];

        #define ADD_INFO(val, log, key) \
        struct string *val = smart_object_get_string(in, qlkey(key), SMART_GET_REPLACE_IF_WRONG_TYPE); \
        if(val->len == 0) {       \
                memset(line, 0, BUFFER_LEN);    \
                app_log(log);   \
                if(fgets(line, BUFFER_LEN, stdin) != NULL) {    \
                        string_cat(val, line, strlen(line));      \
                        string_trim(val); \
                }       \
        }

        #define ADD_PASS(val, log, key) \
        struct string *val = smart_object_get_string(in, qlkey(key), SMART_GET_REPLACE_IF_WRONG_TYPE); \
        if(val->len == 0) {       \
                memset(line, 0, BUFFER_LEN);    \
                app_log(log);   \
                if(common_getpasswd(line, BUFFER_LEN, '*', stdin) > 0) {    \
                        string_cat(val, line, strlen(line));      \
                        string_trim(val); \
                }       \
        }

        ADD_INFO(user_name,     "Enter user_name        : ", "user_name");
        ADD_PASS(user_pass,     "Enter user_pass        : ", "user_pass");
        ADD_INFO(device_id,     "Enter device_id        : ", "device_id");
        ADD_INFO(lat,           "Enter lat              : ", "lat");
        ADD_INFO(lon,           "Enter lon              : ", "lon");

        /*
         * prevent executing other command
         */
        p->command_flag = 0;

        struct string *version_code = smart_object_get_string(p->config, qlkey("service_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *pass = smart_object_get_string(p->config, qlkey("service_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct smart_object *data = smart_object_alloc();
        smart_object_set_string(data, qskey(&__key_version__), qskey(version_code));
        smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_check_in__));
        smart_object_set_string(data, qskey(&__key_pass__),qskey(pass));

        smart_object_set_string(data, qskey(&__key_user_name__), qskey(user_name));
        smart_object_set_string(data, qskey(&__key_device_id__), qskey(device_id));
        smart_object_set_string(data, qskey(&__key_user_pass__), qskey(user_pass));

        struct smart_object *latlng = smart_object_get_object(data, qskey(&__key_latlng__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        smart_object_set_double(latlng, qskey(&__key_lat__), satof(lat->ptr));
        smart_object_set_double(latlng, qskey(&__key_lon__), satof(lon->ptr));

        cs_request_alloc(local_requester, data, (cs_request_callback)checking_service_check_in_callback, p);
}

void checking_service_check_in_callback(struct checking_service *p, struct smart_object *recv)
{
        u8 result = smart_object_get_bool(recv, qskey(&__key_result__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *message = smart_object_get_string(recv, qskey(&__key_message__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        if(result) {
                app_log( PRINT_CYN "%s\n\n" PRINT_RESET, message->ptr);
        } else {
                app_log( PRINT_RED "%s\n\n" PRINT_RESET, message->ptr);
                struct string *json = smart_object_to_json(recv);
                app_log( PRINT_RED "%s\n\n" PRINT_RESET, json->ptr);
                string_free(json);
        }

        p->command_flag = 1;
        pthread_cond_signal(&p->command_cond);
}