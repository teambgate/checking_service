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
#include <checking_client/request/request.h>
#include <cherry/list.h>
#include <cherry/memory.h>
#include <common/request.h>
#include <common/key.h>
#include <common/command.h>
#include <common/util.h>
#include <smartfox/data.h>
#include <cherry/array.h>
#include <cherry/map.h>
#include <native_ui/native_ui_manager.h>

static void __checking_client_requester_update(struct checking_client_requester *p, float delta)
{
        checking_client_requester_process_datas(p);
}

static struct checking_client_requester *__instance = NULL;

static void __clear()
{
        if(__instance) {
                checking_client_requester_free(__instance);
                __instance = NULL;
        }
}

static void __setup()
{
        if(__instance == NULL) {
                cache_add(__clear);

                struct smart_object *config = smart_object_from_json_file("res/config.json", FILE_INNER);
                __instance = checking_client_requester_alloc(config);
        }
}

struct checking_client_requester *checking_client_requester_get_instance()
{
        __setup();
        return __instance;
}

struct checking_client_requester *checking_client_requester_alloc(struct smart_object *config)
{
        struct checking_client_requester *p     = smalloc(sizeof(struct checking_client_requester), checking_client_requester_free);
        INIT_LIST_HEAD(&p->response_contexts);
        INIT_LIST_HEAD(&p->datas);
        INIT_LIST_HEAD(&p->task);
        pthread_mutex_init(&p->ui_thread_lock, NULL);
        p->config                               = config;
        p->requester                            = cs_requester_alloc();

        cs_requester_connect(p->requester,
                smart_object_get_string(config, qskey(&__key_ip__), SMART_GET_REPLACE_IF_WRONG_TYPE)->ptr,
                smart_object_get_int(config, qskey(&__key_port__), SMART_GET_REPLACE_IF_WRONG_TYPE));

        struct native_ui_update_task *task = native_ui_update_task_alloc();
        list_add_tail(&task->user_head, &p->task);
        task->count     = -1;
        task->data      = p;
        task->delegate  = __checking_client_requester_update;

        return p;
}

void checking_client_requester_free(struct checking_client_requester *p)
{
        struct list_head *context_head;
        list_for_each_secure_mutex_lock(context_head, &p->response_contexts, &p->ui_thread_lock, {
                struct checking_client_requester_response_context *context = (struct checking_client_requester_response_context *)
                        ((char *)context_head - offsetof(struct checking_client_requester_response_context, head));

                checking_client_requester_response_context_clear(context);
        });

        if(!list_singular(&p->task)) {
                struct native_ui_update_task *task = (struct native_ui_update_task *)
                        ((char *)p->task.next - offsetof(struct native_ui_update_task, user_head));
                native_ui_update_task_free(task);
        }

        cs_requester_free(p->requester);
        smart_object_free(p->config);
        pthread_mutex_destroy(&p->ui_thread_lock);
        sfree(p);
}

void checking_client_requester_add_context(struct checking_client_requester *p,
        struct checking_client_requester_response_context *context)
{
        checking_client_requester_response_context_clear(context);
        pthread_mutex_lock(&p->ui_thread_lock);
        list_add_tail(&context->head, &p->response_contexts);
        context->ui_thread_lock = &p->ui_thread_lock;
        pthread_mutex_unlock(&p->ui_thread_lock);
}

/*
 * all response datas will be processed in ui thread
 */
void checking_client_requester_callback(struct checking_client_requester *p, struct smart_object *obj)
{
        pthread_mutex_lock(&p->ui_thread_lock);
        struct checking_client_requester_response_data *d = checking_client_requester_response_data_alloc();
        d->data = smart_object_clone(obj);
        list_add_tail(&d->head, &p->datas);
        pthread_mutex_unlock(&p->ui_thread_lock);
}

void checking_client_requester_process_datas(struct checking_client_requester *p)
{
        struct list_head *context_head, *data_head;
        list_for_each_secure_mutex_lock(data_head, &p->datas, &p->ui_thread_lock, {

                pthread_mutex_lock(&p->ui_thread_lock);
                list_del_init(data_head);
                pthread_mutex_unlock(&p->ui_thread_lock);

                struct checking_client_requester_response_data *data = (struct checking_client_requester_response_data *)
                        ((char *)data_head - offsetof(struct checking_client_requester_response_data, head));

                list_for_each_secure_mutex_lock(context_head, &p->response_contexts, &p->ui_thread_lock, {

                        struct checking_client_requester_response_context *context = (struct checking_client_requester_response_context *)
                                ((char *)context_head - offsetof(struct checking_client_requester_response_context, head));

                        if(context->delegate) {
                                context->delegate(context->ctx, data->data);
                        }
                });

                checking_client_requester_response_data_free(data);
        });
}
