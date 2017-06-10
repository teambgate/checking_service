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
#include <checking_client/root_view_controller/root_view_controller.h>
#include <native_ui/view_controller.h>
#include <cherry/stdio.h>

static void __root_view_controller_on_linked(struct native_view_controller *p)
{
        debug("LINKED\n");
}

static void __root_view_controller_on_removed(struct native_view_controller *p)
{

}

struct native_view_controller *root_view_controller_alloc()
{
        struct native_view_controller *p        = native_view_controller_alloc();
        p->on_linked                            = __root_view_controller_on_linked;
        p->on_removed                           = __root_view_controller_on_removed;

        return p;
}
