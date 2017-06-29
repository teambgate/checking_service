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

void checking_service_get_code(struct checking_service *p, struct sobj *in)
{
        char line[BUFFER_LEN];

        #define ADD_INFO(val, log, key) \
        struct string *val = sobj_get_str(in, qlkey(key), RPL_TYPE); \
        if(val->len == 0) {       \
                memset(line, 0, BUFFER_LEN);    \
                app_log(log);   \
                if(fgets(line, BUFFER_LEN, stdin) != NULL) {    \
                        string_cat(val, line, strlen(line));      \
                        string_trim(val); \
                }       \
        }

        ADD_INFO(host,     "Enter host        : ", "host");
        ADD_INFO(port,     "Enter port        : ", "port");

        /*
         * prevent executing other command
         */
        p->command_flag = 0;

        struct string *version_code = sobj_get_str(p->config, qlkey("service_version_code"), RPL_TYPE);
        struct string *pass = sobj_get_str(p->config, qlkey("service_pass"), RPL_TYPE);

        struct sobj *data = sobj_alloc();
        sobj_set_str(data, qskey(&__key_version__), qskey(version_code));
        sobj_set_str(data, qskey(&__key_cmd__), qskey(&__cmd_local_supporter_get_code__));
        sobj_set_str(data, qskey(&__key_pass__),qskey(pass));

        cs_request_alloc_with_param(local_requester, data,
                (cs_request_callback)checking_service_get_code_callback, p,
                (struct cs_request_param){
                        .host = host->ptr,
                        .port = atoi(port->ptr)
                }
        );
}

void checking_service_get_code_callback(struct checking_service *p, struct sobj *recv)
{
        struct string *json = sobj_to_json(recv);
        app_log( PRINT_CYN "%s\n\n" PRINT_RESET, json->ptr);
        string_free(json);

        p->command_flag = 1;
        pthread_cond_signal(&p->command_cond);
}
