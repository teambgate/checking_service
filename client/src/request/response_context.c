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

struct cl_listener *cl_listener_alloc()
{
        struct cl_listener *p = smalloc(
                sizeof(struct cl_listener), cl_listener_free);
        cl_listener_init(p);
        return p;
}

void cl_listener_init(struct cl_listener *p)
{
        INIT_LIST_HEAD(&p->head);
        p->delegate             = NULL;
        p->lock       = NULL;
}

void cl_listener_clear(struct cl_listener *p)
{
        if(p->lock) {
                pthread_mutex_lock(p->lock);
                if( ! list_singular(&p->head)) {
                        list_del_init(&p->head);
                }
                pthread_mutex_unlock(p->lock);
                p->lock       = NULL;
        }
}

void cl_listener_free(struct cl_listener *p)
{
        cl_listener_clear(p);
        sfree(p);
}
