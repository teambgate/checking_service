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
#include <common/cs_server.h>
#include <common/command.h>
#include <common/key.h>
#include <common/request.h>
#include <cherry/memory.h>
#include <cherry/list.h>
#include <cherry/map.h>
#include <cherry/array.h>
#include <cherry/stdio.h>
#include <cherry/string.h>
#include <smartfox/data.h>

static void __register_handler(struct cs_server *p, cs_server_handler_delegate delegate, double time_rate)
{
        struct cs_server_handler *handler = cs_server_handler_alloc(delegate, time_rate);
        list_add_tail(&handler->head, &p->handlers);
}

static void __load_base_map_callback(void *p, struct smart_object *data)
{
        struct string *j = smart_object_to_json(data);
        debug("load base map : %s\n",j->ptr);
        string_free(j);
}

static void __load_base_map(struct checking_service *p, char *file)
{
        struct cs_server *c     = (struct cs_server *)
                ((char *)p->server.next - offsetof(struct cs_server, user_head));

        struct string *es_version_code = smart_object_get_string(c->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = smart_object_get_string(c->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct smart_object *request = cs_request_data_from_file(file, FILE_INNER, qskey(es_version_code), qskey(es_pass));
        cs_request_alloc(p->es_server_requester, request, __load_base_map_callback, p);
}

static void __load_es_server(struct checking_service *p)
{
        if(!list_singular(&p->server)) {
                struct cs_server *c     = (struct cs_server *)
                        ((char *)p->server.next - offsetof(struct cs_server, user_head));

                p->es_server_requester  = cs_requester_alloc();

                struct string *host     = smart_object_get_string(c->config, qlkey("es_host"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                u16 port = smart_object_get_short(c->config, qlkey("es_port"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                cs_requester_connect(p->es_server_requester, host->ptr, port);

                __load_base_map(p, "res/checking_service/index.json");
                __load_base_map(p, "res/checking_service/check/map.json");
                __load_base_map(p, "res/checking_service/device/map.json");
                __load_base_map(p, "res/checking_service/dish/map.json");
                __load_base_map(p, "res/checking_service/meal/map.json");
                __load_base_map(p, "res/checking_service/permission_add_employee/map.json");
                __load_base_map(p, "res/checking_service/permission_clear_checkout/map.json");
                __load_base_map(p, "res/checking_service/permission_add_work_time/map.json");
                __load_base_map(p, "res/checking_service/report/map.json");
                __load_base_map(p, "res/checking_service/report_to/map.json");
                __load_base_map(p, "res/checking_service/work_time/map.json");
                __load_base_map(p, "res/checking_service/user/map.json");
                __load_base_map(p, "res/checking_service/blocked_device/map.json");
        }
}

static void __load_supervisor(struct checking_service *p)
{
        if(!list_singular(&p->server)) {
                struct cs_server *c     = (struct cs_server *)
                        ((char *)p->server.next - offsetof(struct cs_server, user_head));

                p->supervisor_requester  = cs_requester_alloc();

                struct string *host     = smart_object_get_string(c->config, qlkey("supervisor_host"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                u16 port = smart_object_get_short(c->config, qlkey("supervisor_port"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                cs_requester_connect(p->supervisor_requester, host->ptr, port);
        }
}

struct checking_service *checking_service_alloc(u8 local_only)
{
        struct checking_service *p      = smalloc(sizeof(struct checking_service));
        INIT_LIST_HEAD(&p->server);

        struct cs_server *c             = cs_server_alloc(local_only);
        list_add_tail(&c->user_head, &p->server);

        p->command_flag                 = 1;
        pthread_mutex_init(&p->command_mutex, NULL);
        pthread_cond_init (&p->command_cond, NULL);

        c->config                       = smart_object_from_json_file("res/config.json", FILE_INNER);
        p->config                       = c->config;

        if(local_only) {
                map_set(c->delegates, qskey(&__cmd_service_register__),
                        &(cs_server_delegate){checking_service_process_service_register_username});
                map_set(c->delegates, qskey(&__cmd_service_validate__),
                        &(cs_server_delegate){checking_service_process_service_validate_username});
                map_set(c->delegates, qskey(&__cmd_location_register__),
                        &(cs_server_delegate){checking_service_process_location_register});
                map_set(c->delegates, qskey(&__cmd_location_update_ip_port__),
                        &(cs_server_delegate){checking_service_process_location_update_ip});
                map_set(c->delegates, qskey(&__cmd_location_update_latlng__),
                        &(cs_server_delegate){checking_service_process_location_update_latlng});
                map_set(c->delegates, qskey(&__cmd_user_reserve__),
                        &(cs_server_delegate){checking_service_process_user_reserve});
                map_set(c->delegates, qskey(&__cmd_user_validate__),
                        &(cs_server_delegate){checking_service_process_user_validate});
                map_set(c->delegates, qskey(&__cmd_device_add__),
                        &(cs_server_delegate){checking_service_process_device_add});
                map_set(c->delegates, qskey(&__cmd_work_time_new__),
                        &(cs_server_delegate){checking_service_process_work_time_new});
                map_set(c->delegates, qskey(&__cmd_permission_add_employee__),
                        &(cs_server_delegate){checking_service_process_permission_add_employee});
                map_set(c->delegates, qskey(&__cmd_permission_add_work_time__),
                        &(cs_server_delegate){checking_service_process_permission_add_work_time});
                map_set(c->delegates, qskey(&__cmd_permission_clear_checkout__),
                        &(cs_server_delegate){checking_service_process_permission_clear_checkout});
        } else {
                map_set(c->delegates, qskey(&__cmd_user_reserve__),
                        &(cs_server_delegate){checking_service_process_user_reserve});
                map_set(c->delegates, qskey(&__cmd_device_add__),
                        &(cs_server_delegate){checking_service_process_device_add});
                map_set(c->delegates, qskey(&__cmd_work_time_new_by_user__),
                        &(cs_server_delegate){checking_service_process_work_time_new_by_user});
                map_set(c->delegates, qskey(&__cmd_check_in__),
                        &(cs_server_delegate){checking_service_process_check_in});
                map_set(c->delegates, qskey(&__cmd_check_out__),
                        &(cs_server_delegate){checking_service_process_check_out});
        }

        __load_es_server(p);
        __load_supervisor(p);

        return p;
}

void checking_service_start(struct checking_service *p)
{
        if(!list_singular(&p->server)) {
                struct cs_server *c     = (struct cs_server *)
                        ((char *)p->server.next - offsetof(struct cs_server, user_head));

                if(c->local_only) {
                        u16 port                = smart_object_get_short(c->config, qlkey("service_local_port"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                        cs_server_start(c, port);
                } else {
                        u16 port                = smart_object_get_short(c->config, qlkey("service_port"), SMART_GET_REPLACE_IF_WRONG_TYPE);

                        cs_server_start(c, port);
                }
        }
}

void checking_service_free(struct checking_service *p)
{
        if(!list_singular(&p->server)) {
                struct cs_server *c     = (struct cs_server *)
                        ((char *)p->server.next - offsetof(struct cs_server, user_head));
                cs_server_free(c);
        }
        pthread_mutex_destroy(&p->command_mutex);
        pthread_cond_destroy(&p->command_cond);

        sfree(p);
}
