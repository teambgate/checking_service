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
#include <supervisor/handler.h>
#include <cherry/memory.h>
#include <cherry/list.h>
#include <sys/time.h>

struct supervisor_handler *supervisor_handler_alloc(supervisor_handler_delegate delegate, double time_rate)
{
        struct supervisor_handler *p    = smalloc(sizeof(struct supervisor_handler));
        INIT_LIST_HEAD(&p->head);
        p->delegate                     = delegate;
        p->time_rate                    = time_rate;
        gettimeofday(&p->last_executed_time, NULL);

        return p;
}

void supervisor_handler_free(struct supervisor_handler *p)
{
        list_del(&p->head);
        sfree(p);
}
