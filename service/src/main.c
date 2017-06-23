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
#include <service/local_supporter.h>

#include <pthread.h>

#include <common/request.h>

#include <cherry/time.h>

#include <common/util.h>

/*
 * define extern variables
 */
static JavaVM*  jvm;
JavaVM*         __jvm;
struct map *    __jni_env_map;

struct smart_object     *__shared_ram_config__;
pthread_mutex_t         __shared_ram_config_mutex__;

/*
 * define public and private service
 */
static struct checking_service  *public_service = NULL;
static struct checking_service  *local_service  = NULL;

/*
 * define local supporter
 */
static struct local_supporter   *local_supporter = NULL;

/*
 * local requester
 */
struct cs_requester *local_requester = NULL;

/*
 * setup JNI
 */
static void __setup_jni()
{
        JNIEnv*                 env;
        JavaVMInitArgs          vm_args;
        JavaVMOption            options[10];

        int oi = 0;
        options[oi++].optionString = "-Djava.class.path=./";

        vm_args.version                 = JNI_VERSION_1_4;
        vm_args.options                 = options;
        vm_args.nOptions                = oi;
        vm_args.ignoreUnrecognized      = 1;

        JNI_CreateJavaVM( &jvm, (void**)&env, &vm_args );

        __jvm = jvm;
        __jni_env_map = map_alloc(sizeof(JNIEnv *));
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
                app_log( PRINT_GRN);
                if(fgets(line, BUFFER_LEN, stdin) != NULL) {
                        app_log( PRINT_RESET);
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
                        } else if(strcmp(cmd->ptr, "work_time_new") == 0) {
                                checking_service_work_time_new(local_service, com);
                        } else if(strcmp(cmd->ptr, "permission_add_work_time") == 0) {
                                checking_service_permission_add_work_time(local_service, com);
                        } else if(strcmp(cmd->ptr, "permission_add_employee") == 0) {
                                checking_service_permission_add_employee(local_service, com);
                        } else if(strcmp(cmd->ptr, "permission_clear_checkout") == 0) {
                                checking_service_permission_clear_checkout(local_service, com);
                        } else if(strcmp(cmd->ptr, "test_check_in") == 0) {
                                checking_service_check_in(local_service, com);
                        } else if(strcmp(cmd->ptr, "test_check_out") == 0) {
                                checking_service_check_out(local_service, com);
                        } else if(strcmp(cmd->ptr, "test_check_search_by_date_by_user") == 0) {
                                checking_service_check_search_by_date_by_user(local_service, com);
                        } else if(strcmp(cmd->ptr, "test_work_time_search_by_date_by_user") == 0) {
                                checking_service_work_time_search_by_date_by_user(local_service, com);
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
                                app_log("- " PRINT_YEL "work_time_new\n" PRINT_RESET);
                                app_log("\t\tchange work time for a user\n\n");
                                app_log("- " PRINT_YEL "permission_add_work_time\n" PRINT_RESET);
                                app_log("\t\tchange permission add work time of a user\n\n");
                                app_log("- " PRINT_YEL "permission_add_employee\n" PRINT_RESET);
                                app_log("\t\tchange permission add employee of a user\n\n");
                                app_log("- " PRINT_YEL "permission_clear_checkout\n" PRINT_RESET);
                                app_log("\t\tchange permission clear checkout of a user\n\n");
                                app_log("- " PRINT_YEL "test_check_in\n" PRINT_RESET);
                                app_log("\t\ttest check in\n\n");
                                app_log("- " PRINT_YEL "test_check_out\n" PRINT_RESET);
                                app_log("\t\ttest check out\n\n");
                                app_log("- " PRINT_YEL "test_check_search_by_date_by_user\n" PRINT_RESET);
                                app_log("\t\ttest search user checks\n\n");
                                app_log("- " PRINT_YEL "test_work_time_search_by_date_by_user\n" PRINT_RESET);
                                app_log("\t\ttest search user work times\n\n");
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

static void __start_local_supporter(void *d)
{
        struct local_supporter *supporter = (struct local_supporter *)d;
        local_supporter_start(supporter);
}

int main( int argc, char** argv )
{
        srand ( time(NULL) );

        __setup_jni();

        __shared_ram_config__ = smart_object_alloc();
        pthread_mutex_init(&__shared_ram_config_mutex__, NULL);

        local_service   = checking_service_alloc(CS_SERVER_LOCAL);
        public_service  = checking_service_alloc(CS_SERVER_PUBLIC);
        local_supporter = local_supporter_alloc();

        local_requester = cs_requester_alloc();
        u16 port        = smart_object_get_short(local_service->config, qlkey("service_local_port"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        cs_requester_connect(local_requester, "192.168.1.246", 9898);

        pthread_t tid[4];
        pthread_create(&tid[0], NULL, (void*(*)(void*))__read_input, (void*)NULL);
        pthread_create(&tid[1], NULL, (void*(*)(void*))__start_service, (void*)local_service);
        pthread_create(&tid[2], NULL, (void*(*)(void*))__start_service, (void*)public_service);
        pthread_create(&tid[3], NULL, (void*(*)(void*))__start_local_supporter, (void*)local_supporter);

        int i;
        for(i = 1 ; i < 4; i++) {
                int rc = pthread_join(tid[i], NULL);
                if (rc) {
                       break;
                }
        }

finish:;
        /*
         * deallocate local and public services
         */
        checking_service_free(local_service);
        checking_service_free(public_service);

        /*
         * deallocate local supporter
         */
        local_supporter_free(local_supporter);

        /*
         * deallocate jni map
         */
        map_free(__jni_env_map);

        /*
         * deallocate shared ram config
         */
        smart_object_free(__shared_ram_config__);
        pthread_mutex_destroy(&__shared_ram_config_mutex__);

        /*
         * clean memory pool
         */
        cache_free();
        dim_memory();

        /*
         * destroy JVM
         */
        (*jvm)->DestroyJavaVM( jvm );

        return 0;
}
