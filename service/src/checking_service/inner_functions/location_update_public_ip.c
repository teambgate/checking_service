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

#include <curl/curl.h>
#include <arpa/inet.h>

#include <pthread.h>

#define BUFFER_LEN 1024

static size_t func(void *ptr, size_t size, size_t nmemb, void *d)
{
        struct string *ip = d;
        size_t realsize = size * nmemb;

        string_cat(ip, ptr, realsize - 1);

        if(common_is_ip(ip->ptr)) {
        } else {
                ip->len = 0;
        }

        return size * nmemb;
}

static void __search_ip(struct string *ip)
{
        CURL *curl;
        CURLcode res;

        curl = curl_easy_init();
        if(curl) {
                struct string *temp = string_alloc(0);
                string_cat(temp, qlkey("http://ipinfo.io/ip"));
                curl_easy_setopt(curl, CURLOPT_URL, temp->ptr);
                string_free(temp);

                /* example.com is redirected, so we tell libcurl to follow redirection */
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,"GET");
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, func);
                int num;
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)ip);

                /* Perform the request, res will get the return code */
                res = curl_easy_perform(curl);
                /* Check for errors */
                if(res != CURLE_OK)
                        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

                /* always cleanup */
                curl_easy_cleanup(curl);
        }
}

void checking_service_location_update_public_ip(struct checking_service *p, struct sobj *in)
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

        #define ADD_PASS(val, log, key) \
        struct string *val = sobj_get_str(in, qlkey(key), RPL_TYPE); \
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

        struct string *ip = sobj_get_str(in, qskey(&__key_ip__), RPL_TYPE);
        __search_ip(ip);

        if( ! ip->len) {
                app_log( PRINT_RED "Search for public ip failed!\n\n" PRINT_RESET);
                return;
        } else {
                app_log("Enter ip               : %s\n", ip->ptr);
        }

        ADD_INFO(port,          "Enter port             : ", "port");

        struct string *device_id = common_get_mac_address();

        /*
         * prevent executing other command
         */
        p->command_flag = 0;

        struct string *version_code = sobj_get_str(p->config, qlkey("service_version_code"), RPL_TYPE);
        struct string *pass = sobj_get_str(p->config, qlkey("service_pass"), RPL_TYPE);

        struct sobj *data = sobj_alloc();
        sobj_set_str(data, qskey(&__key_version__), qskey(version_code));
        sobj_set_str(data, qskey(&__key_cmd__), qskey(&__cmd_location_update_ip_port__));
        sobj_set_str(data, qskey(&__key_pass__),qskey(pass));

        sobj_set_str(data, qskey(&__key_ip__), qskey(ip));
        sobj_set_i32(data, qskey(&__key_port__), atoi(port->ptr));
        sobj_set_str(data, qskey(&__key_user_name__), qskey(user_name));
        sobj_set_str(data, qskey(&__key_user_pass__), qskey(user_pass));
        sobj_set_str(data, qskey(&__key_device_id__), qskey(device_id));
        cs_request_alloc(local_requester, data, (cs_request_callback)checking_service_location_update_public_ip_callback, p);

        string_free(device_id);
}

void checking_service_location_update_public_ip_callback(struct checking_service *p, struct sobj *recv)
{
        u8 result = sobj_get_u8(recv, qskey(&__key_result__), RPL_TYPE);
        struct string *message = sobj_get_str(recv, qskey(&__key_message__), RPL_TYPE);
        if(result) {
                app_log( PRINT_CYN "%s\n\n" PRINT_RESET, message->ptr);
        } else {
                app_log( PRINT_RED "%s\n\n" PRINT_RESET, message->ptr);
        }

        p->command_flag = 1;
        pthread_cond_signal(&p->command_cond);
}
