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
#include <checking_client/register_controller/register_controller.h>
#include <native_ui/view_controller.h>
#include <native_ui/action.h>
#include <native_ui/touch_handle.h>
#include <native_ui/parser.h>
#include <native_ui/device.h>
#include <smartfox/data.h>
#include <cherry/stdio.h>
#include <native_ui/view.h>
#include <cherry/math/math.h>
#include <cherry/map.h>
#include <cherry/array.h>
#include <cherry/memory.h>
#include <cherry/string.h>
#include <checking_client/request/request.h>
#include <common/request.h>
#include <common/key.h>
#include <common/command.h>

ADD_FUNCTION_LISTEN(struct register_controller_data);

ADD_CONTROLLER_DATA_ALLOC(struct register_controller_data, {
        map_set(p->cmd_delegate, qskey(&__cmd_user_reserve__),
                &(view_controller_command_delegate){register_controller_on_listen_register});
});

ADD_CONTROLLER_DATA_FREE(struct register_controller_data, {

});

ADD_CONTROLLER_ALLOC(register_controller);

void register_controller_on_linked(struct native_view_controller *p)
{
        struct register_controller_data *data = (struct register_controller_data *)p->custom_data;

        struct native_view *view = native_view_controller_get_view(p);
        struct native_view_parser *parser = native_view_get_parser(view);

        REGISTER_TOUCH(parser, qlkey("back"), register_controller_on_touch_back, p, NULL)
        REGISTER_TOUCH(parser, qlkey("register"), register_controller_on_touch_register, p, NULL)

        checking_client_requester_add_context(checking_client_requester_get_instance(), data->response_context);
}

void register_controller_on_removed(struct native_view_controller *p)
{
        struct register_controller_data *data = (struct register_controller_data *)p->custom_data;
        checking_client_requester_response_context_clear(data->response_context);
}

void register_controller_on_touch_back(struct native_view_controller *p, struct native_view *sender, u8 type)
{

}

void register_controller_on_touch_register(struct native_view_controller *p, struct native_view *sender, u8 type)
{

}

void register_controller_on_listen_register(struct native_view_controller *p, struct smart_object *obj)
{

}
