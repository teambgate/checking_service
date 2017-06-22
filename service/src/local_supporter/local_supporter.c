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
#include <service/local_supporter.h>
#include <cherry/memory.h>
#include <cherry/list.h>

#include <common/cs_server.h>

struct local_supporter *local_supporter_alloc()
{
        struct local_supporter *p       = smalloc(sizeof(struct local_supporter));
        INIT_LIST_HEAD(&p->server);

        struct cs_server *c             = cs_server_alloc(CS_SERVER_LOCAL);
        list_add_tail(&c->user_head, &p->server);

        return p;
}

void local_supporter_start(struct local_supporter *p)
{

}

void local_supporter_free(struct local_supporter *p)
{
        if(!list_singular(&p->server)) {
                struct cs_server *c     = (struct cs_server *)
                        ((char *)p->server.next - offsetof(struct cs_server, user_head));
                cs_server_free(c);
        }

        sfree(p);
}
