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
#include <native_ui/manager.h>

static void __cl_talker_update(struct cl_talker *p, float delta)
{
        cl_talker_process_datas(p);
}

static struct cl_talker *__instance = NULL;

static void __clear()
{
        if(__instance) {
                cl_talker_free(__instance);
                __instance = NULL;
        }
}

static void __setup()
{
        if(__instance == NULL) {
                cache_add(__clear);

                struct sobj *config = sobj_from_json_file("res/config.json", FILE_INNER);
                __instance = cl_talker_alloc(config);
        }
}

struct cl_talker *cl_talker_shared()
{
        __setup();
        return __instance;
}

struct cl_talker *cl_talker_alloc(struct sobj *config)
{
        struct cl_talker *p     = smalloc(sizeof(struct cl_talker), cl_talker_free);
        INIT_LIST_HEAD(&p->listeners);
        INIT_LIST_HEAD(&p->datas);
        INIT_LIST_HEAD(&p->task);
        pthread_mutex_init(&p->lock, NULL);
        p->config                               = config;
        p->requester                            = cs_requester_alloc();

        cs_requester_connect(p->requester,
                sobj_get_str(config, qskey(&__key_ip__), RPL_TYPE)->ptr,
                sobj_get_int(config, qskey(&__key_port__), RPL_TYPE));

        struct ntask *task = ntask_alloc();
        list_add_tail(&task->user_head, &p->task);
        task->count     = -1;
        task->data      = p;
        task->delegate  = __cl_talker_update;

        return p;
}

void cl_talker_free(struct cl_talker *p)
{
        struct list_head *context_head;
        list_for_each_secure_mutex_lock(context_head, &p->listeners, &p->lock, {
                struct cl_listener *context = (struct cl_listener *)
                        ((char *)context_head - offsetof(struct cl_listener, head));

                cl_listener_clear(context);
        });

        if(!list_singular(&p->task)) {
                struct ntask *task = (struct ntask *)
                        ((char *)p->task.next - offsetof(struct ntask, user_head));
                ntask_free(task);
        }

        cs_requester_free(p->requester);
        sobj_free(p->config);
        pthread_mutex_destroy(&p->lock);
        sfree(p);
}

void cl_talker_add_context(struct cl_talker *p,
        struct cl_listener *context)
{
        cl_listener_clear(context);
        pthread_mutex_lock(&p->lock);
        list_add_tail(&context->head, &p->listeners);
        context->lock = &p->lock;
        pthread_mutex_unlock(&p->lock);
}

/*
 * all response datas will be processed in ui thread
 */
void cl_talker_callback(struct cl_talker *p, struct sobj *obj)
{
        pthread_mutex_lock(&p->lock);
        struct cl_response *d = cl_response_alloc();
        d->data = sobj_clone(obj);
        list_add_tail(&d->head, &p->datas);
        pthread_mutex_unlock(&p->lock);
}

void cl_talker_process_datas(struct cl_talker *p)
{
        struct list_head *context_head, *data_head;
        list_for_each_secure_mutex_lock(data_head, &p->datas, &p->lock, {

                pthread_mutex_lock(&p->lock);
                list_del_init(data_head);
                pthread_mutex_unlock(&p->lock);

                struct cl_response *data = (struct cl_response *)
                        ((char *)data_head - offsetof(struct cl_response, head));

                list_for_each_secure_mutex_lock(context_head, &p->listeners, &p->lock, {

                        struct cl_listener *context = (struct cl_listener *)
                                ((char *)context_head - offsetof(struct cl_listener, head));

                        if(context->delegate) {
                                context->delegate(context->ctx, data->data);
                        }
                });

                cl_response_free(data);
        });
}

void cl_talker_send(struct cl_talker *p, struct sobj *data, struct cl_dst dst)
{
        sobj_set_str(data, qskey(&__key_version__), dst.ver, dst.ver_len);
        sobj_set_str(data, qskey(&__key_pass__), dst.pss, dst.pss_len);

        cs_request_alloc_with_param(p->requester, data,
                (cs_request_callback)cl_talker_callback, p,
                (struct cs_request_param){
                        .host = dst.ip,
                        .port = dst.port
                }
        );
}
