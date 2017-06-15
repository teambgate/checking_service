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
#include <supervisor/callback_user_data.h>
#include <cherry/memory.h>
#include <smartfox/data.h>

struct callback_user_data *callback_user_data_alloc(struct supervisor *p, int fd, u32 mask, struct smart_object *obj)
{
        struct callback_user_data *cud = smalloc(sizeof(struct callback_user_data));
        cud->p = p;
        cud->fd = fd;
        cud->mask = mask;
        cud->obj = smart_object_clone(obj);
        return cud;
}

void callback_user_data_free(struct callback_user_data *p)
{
        smart_object_free(p->obj);
        sfree(p);
}
