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
#include <smartfox/data.h>
#include <cherry/memory.h>

struct checking_client_requester_response_data *checking_client_requester_response_data_alloc()
{
        struct checking_client_requester_response_data *p = smalloc(sizeof(struct checking_client_requester_response_data));
        INIT_LIST_HEAD(&p->head);
        p->data = NULL;
        return p;
}

void checking_client_requester_response_data_free(struct checking_client_requester_response_data *p)
{
        list_del_init(&p->head);
        if(p->data) smart_object_free(p->data);
        sfree(p);
}