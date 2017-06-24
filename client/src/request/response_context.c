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

struct checking_client_requester_response_context *checking_client_requester_response_context_alloc()
{
        struct checking_client_requester_response_context *p = smalloc(sizeof(struct checking_client_requester_response_context));
        checking_client_requester_response_context_init(p);
        return p;
}

void checking_client_requester_response_context_init(struct checking_client_requester_response_context *p)
{
        INIT_LIST_HEAD(&p->head);
        p->delegate             = NULL;
        p->ui_thread_lock       = NULL;
}

void checking_client_requester_response_context_clear(struct checking_client_requester_response_context *p)
{
        if(p->ui_thread_lock) {
                pthread_mutex_lock(p->ui_thread_lock);
                if( ! list_singular(&p->head)) {
                        list_del_init(&p->head);
                }
                p->ui_thread_lock       = NULL;
                pthread_mutex_unlock(p->ui_thread_lock);
        }
}

void checking_client_requester_response_context_free(struct checking_client_requester_response_context *p)
{
        checking_client_requester_response_context_clear(p);
        sfree(p);
}
