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

#include <pthread.h>

#include <common/request.h>

static JavaVM*  jvm;
JNIEnv*         __jni_env;

static struct checking_service  *public_service = NULL;
static struct checking_service  *local_service  = NULL;

struct cs_requester *local_requester = NULL;

static void __setup_jni()
{
        JNIEnv*                 env;
        JavaVMInitArgs          vm_args;
        JavaVMOption            options[10];

        int oi = 0;
        options[oi++].optionString = "-Djava.class.path=./s2.jar";

        vm_args.version                 = JNI_VERSION_1_4;
        vm_args.options                 = options;
        vm_args.nOptions                = oi;
        vm_args.ignoreUnrecognized      = 1;

        JNI_CreateJavaVM( &jvm, (void**)&env, &vm_args );

        __jni_env = env;
}

static struct smart_object *__parse_input(char *line, int len)
{
        int counter             = 0;
        int start               = 0;
        int end                 = 0;
        struct smart_object *p    = smart_object_alloc();
        struct string *key      = string_alloc(0);
        struct string *val      = string_alloc(0);

#define increase_count() \
        counter++;      \
        if(counter == len - 1) goto end;

find_cmd:;
        if(isspace(line[counter])) {
                increase_count()
                goto find_cmd;
        }
        start   = counter;
        end     = start;
        increase_count()
read_cmd:;
        if( ! isspace(line[counter])) {
                end = counter;
                increase_count()
                goto read_cmd;
        }
        smart_object_set_string(p, qlkey("cmd"), line + start, end - start + 1);

find_argument:;
        if(line[counter] != '-') {
                increase_count();
                goto find_argument;
        }
        increase_count()
        start   = counter;
        end     = start;
        increase_count()
read_argument:;
        if( ! isspace(line[counter])) {
                end = counter;
                increase_count()
                goto read_argument;
        }
        key->len = 0;
        string_cat(key, line + start, end - start + 1);

find_value:;
        if(isspace(line[counter])) {
                increase_count();
                goto find_value;
        }
        start   = counter;
        end     = start;
        increase_count()
read_value:;
        if(line[counter] != '-' && line[counter] != '\n') {
                end = counter;
                increase_count()
                goto read_value;
        }
        val->len = 0;
        string_cat(val, line + start, end - start + 1);
        string_trim(key);
        string_trim(val);
        smart_object_set_string(p, key->ptr, key->len, val->ptr, val->len);

        goto find_argument;

end:;
        string_free(val);
        string_free(key);
        return p;
}

static void __read_input(void *d)
{
#define BUFFER_LEN 1024
get_line:;
        char line[BUFFER_LEN];
        int counter = 0;

        pthread_mutex_lock(&local_service->command_mutex);
        if(local_service->command_flag) {
                pthread_mutex_unlock(&local_service->command_mutex);
                memset(line, 0, BUFFER_LEN);
                if(fgets(line, BUFFER_LEN, stdin) != NULL) {
                        struct smart_object *com = __parse_input(line, BUFFER_LEN);

                        struct string *cmd = smart_object_get_string(com, qlkey("cmd"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                        if(strcmp(cmd->ptr, "service_register") == 0) {
                                checking_service_register_username(local_service, com);
                        } else if(strcmp(cmd->ptr, "service_validate") == 0) {
                                checking_service_validate_username(local_service, com);
                        } else if(strcmp(cmd->ptr, "location_register") == 0) {
                                checking_service_register_location(local_service, com);
                        } else if(strcmp(cmd->ptr, "location_update_ip") == 0) {
                                checking_service_location_update_ip(local_service, com);
                        } else if(strcmp(cmd->ptr, "location_update_public_ip") == 0) {
                                checking_service_location_update_public_ip(local_service, com);
                        } else if(strcmp(cmd->ptr, "location_update_latlng") == 0) {
                                checking_service_location_update_latlng(local_service, com);
                        } else if(strcmp(cmd->ptr, "user_reserve") == 0) {
                                checking_service_user_reserve(local_service, com);
                        } else if(strcmp(cmd->ptr, "user_validate") == 0) {
                                checking_service_user_validate(local_service, com);
                        } else if(strcmp(cmd->ptr, "device_add") == 0) {
                                checking_service_device_add(local_service, com);
                        } else {
                                if(strcmp(cmd->ptr, "help") != 0) {
                                        app_log("command not found! Commands available :\n");
                                } else {
                                        app_log("Commands available :\n");
                                }
                                app_log("- " PRINT_YEL "service_register\n" PRINT_RESET);
                                app_log("\t\tregister new service\n\n");
                                app_log("- " PRINT_YEL "service_validate\n" PRINT_RESET);
                                app_log("\t\tvalidate one service\n\n");
                                app_log("- " PRINT_YEL "location_register\n" PRINT_RESET);
                                app_log("\t\tregister this machine as a location for a service\n\n");
                                app_log("- " PRINT_YEL "location_update_ip\n" PRINT_RESET);
                                app_log("\t\tupdate ip for location registered by this machine\n\n");
                                app_log("- " PRINT_YEL "location_update_public_ip\n" PRINT_RESET);
                                app_log("\t\tsearch for public ip and update for location registered by this machine\n\n");
                                app_log("- " PRINT_YEL "location_update_latlng\n" PRINT_RESET);
                                app_log("\t\tupdate latlng for location registered by this machine\n\n");
                                app_log("- " PRINT_YEL "user_reserve\n" PRINT_RESET);
                                app_log("\t\treserve an user\n\n");
                                app_log("- " PRINT_YEL "user_validate\n" PRINT_RESET);
                                app_log("\t\tvalidate an user\n\n");
                                app_log("- " PRINT_YEL "device_add\n" PRINT_RESET);
                                app_log("\t\tattach a device to a user\n\n");
                                app_log("\n");
                        }

                        smart_object_free(com);
                }
        } else {
                pthread_cond_wait(&local_service->command_cond, &local_service->command_mutex);
                pthread_mutex_unlock(&local_service->command_mutex);
        }

        goto get_line;
}

static void __start_service(void *d)
{
        struct checking_service *service = (struct checking_service *)d;
        checking_service_start(service);
}

int main( int argc, char** argv )
{
        srand ( time(NULL) );

        __setup_jni();

        local_service   = checking_service_alloc(CS_SERVER_LOCAL);
        public_service  = checking_service_alloc(CS_SERVER_PUBLIC);

        local_requester = cs_requester_alloc();
        u16 port        = smart_object_get_short(local_service->config, qlkey("service_local_port"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        cs_requester_connect(local_requester, "127.0.0.1", port);

        pthread_t tid[3];
        pthread_create(&tid[0], NULL, (void*(*)(void*))__read_input, (void*)NULL);
        pthread_create(&tid[1], NULL, (void*(*)(void*))__start_service, (void*)local_service);
        pthread_create(&tid[2], NULL, (void*(*)(void*))__start_service, (void*)public_service);

        int i;
        for(i = 1 ; i < 3; i++) {
                int rc = pthread_join(tid[i], NULL);
                if (rc) {
                       break;
                }
        }

        checking_service_free(local_service);
        checking_service_free(public_service);
        cache_free();
        dim_memory();

        (*jvm)->DestroyJavaVM( jvm );
        return 0;
}
