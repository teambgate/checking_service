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
#include <checking_client/welcome_controller/welcome_controller.h>
#include <native_ui/view_controller.h>
#include <native_ui/action.h>
#include <native_ui/touch_handle.h>
#include <native_ui/parser.h>
#include <smartfox/data.h>
#include <cherry/stdio.h>
#include <native_ui/view.h>
#include <cherry/math/math.h>
#include <cherry/map.h>
#include <cherry/array.h>
#include <cherry/memory.h>
#include <cherry/string.h>
#include <checking_client/request/request.h>
#include <checking_client/request/functions/search_around.h>
#include <common/request.h>
#include <common/key.h>
#include <common/command.h>

#define REGISTER_TOUCH(parser, name, handle, data, fdel) \
{       \
        struct native_view_touch_handle *th = native_view_parser_get_touch_handle(parser, name);      \
        if(th) {        \
                native_view_touch_handle_set_touch_delegate(th, handle, data, fdel);    \
        }       \
}

static void __welcome_controller_on_listen(struct native_view_controller *p, struct smart_object *obj)
{
        struct welcome_controller_data *data = (struct welcome_controller_data *)p->custom_data;
        struct string *cmd = smart_object_get_string(obj, qskey(&__key_cmd__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        view_controller_command_delegate *delegate = map_get_pointer(data->cmd_delegate, qskey(cmd));

        if(*delegate) {
                (*delegate)(p, obj);
        }
}

static struct welcome_controller_data *__welcome_controller_data_alloc(struct native_view_controller *controller)
{
        struct welcome_controller_data *p       = smalloc(sizeof(struct welcome_controller_data));
        p->state                                = WELCOME_SHOW_SEARCH_IP;
        p->response_context                     = checking_client_requester_response_context_alloc();
        p->response_context->ctx                = controller;
        p->response_context->delegate           = __welcome_controller_on_listen;
        p->cmd_delegate                         = map_alloc(sizeof(view_controller_command_delegate));
        p->searching_around                     = 0;

        map_set(p->cmd_delegate, qskey(&__cmd_location_search_nearby__),
                &(view_controller_command_delegate){welcome_controller_on_listen_search_around});

        return p;
}

static void __welcome_controller_data_free(struct welcome_controller_data *p)
{
        checking_client_requester_response_context_free(p->response_context);
        map_free(p->cmd_delegate);
        sfree(p);
}

struct native_view_controller *welcome_controller_alloc()
{
        struct native_view_controller *p        = native_view_controller_alloc();
        p->on_linked                            = welcome_controller_on_linked;
        p->on_removed                           = welcome_controller_on_removed;
        p->custom_data                          = __welcome_controller_data_alloc(p);
        p->custom_data_free                     = __welcome_controller_data_free;
        return p;
}

void welcome_controller_on_linked(struct native_view_controller *p)
{
        struct welcome_controller_data *data = (struct welcome_controller_data *)p->custom_data;

        struct native_view *view = native_view_controller_get_view(p);
        struct native_view_parser *parser = native_view_get_parser(view);

        REGISTER_TOUCH(parser, qlkey("set_ip"), welcome_controller_on_touch_set_ip, p, NULL)
        REGISTER_TOUCH(parser, qlkey("search_ip"), welcome_controller_on_touch_search_ip, p, NULL)
        REGISTER_TOUCH(parser, qlkey("search_around"), welcome_controller_on_touch_search_around, p, NULL)

        checking_client_requester_add_context(checking_client_requester_get_instance(), data->response_context);
}

void welcome_controller_on_removed(struct native_view_controller *p)
{
        struct welcome_controller_data *data = (struct welcome_controller_data *)p->custom_data;
        checking_client_requester_response_context_clear(data->response_context);
}

static void __set_state(struct native_view_controller *p, u8 state)
{
        struct native_view *view = native_view_controller_get_view(p);
        struct native_view_parser *parser = native_view_get_parser(view);

        struct native_view *set_ip_button = native_view_parser_get_hash_view(parser, qlkey("set_ip_button"));
        struct native_view *search_around_button = native_view_parser_get_hash_view(parser, qlkey("search_around_button"));
        struct native_view *search_box_view = native_view_parser_get_hash_view(parser, qlkey("search_box_view"));
        struct native_view *search_around_view = native_view_parser_get_hash_view(parser, qlkey("search_around_view"));

        struct welcome_controller_data *data = (struct welcome_controller_data *)p->custom_data;
        switch (state) {
                case WELCOME_SHOW_SEARCH_IP:
                        native_view_clear_all_actions(set_ip_button);
                        native_view_clear_all_actions(search_around_button);
                        native_view_run_action(set_ip_button,
                                native_view_alpha_to(set_ip_button,
                                        1.0f,
                                        0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                NULL);
                        native_view_run_action(search_around_button,
                                native_view_alpha_to(search_around_button,
                                        0.35f,
                                        0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                NULL);
                        break;
                case WELCOME_SHOW_SEARCH_AROUND:
                        native_view_clear_all_actions(set_ip_button);
                        native_view_clear_all_actions(search_around_button);
                        native_view_run_action(set_ip_button,
                                native_view_alpha_to(set_ip_button,
                                        0.35f,
                                        0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                NULL);
                        native_view_run_action(search_around_button,
                                native_view_alpha_to(search_around_button,
                                        1.0f,
                                        0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                NULL);
                        break;
                default:
                        break;
        }

        if(data->state != state) {
                switch (state) {
                        case WELCOME_SHOW_SEARCH_IP:
                                native_view_clear_all_actions(search_box_view);
                                native_view_clear_all_actions(search_around_view);
                                native_view_run_action(search_box_view,
                                        native_ui_action_sequence(
                                                native_view_show(search_box_view),
                                                native_view_alpha_to(search_box_view,
                                                        1.0f, 0.25f, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                                NULL
                                        ),
                                        NULL
                                );
                                native_view_run_action(search_around_view,
                                        native_ui_action_sequence(
                                                native_view_alpha_to(search_around_view,
                                                        0.0f, 0.25f, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                                native_view_hide(search_around_view),
                                                NULL
                                        ),
                                        NULL
                                );
                                break;
                        case WELCOME_SHOW_SEARCH_AROUND:
                                native_view_clear_all_actions(search_box_view);
                                native_view_clear_all_actions(search_around_view);
                                native_view_run_action(search_box_view,
                                        native_ui_action_sequence(
                                                native_view_alpha_to(search_box_view,
                                                        0.0f, 0.25f, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                                native_view_hide(search_box_view),
                                                NULL
                                        ),
                                        NULL
                                );
                                native_view_run_action(search_around_view,
                                        native_ui_action_sequence(
                                                native_view_show(search_around_view),
                                                native_view_alpha_to(search_around_view,
                                                        1.0f, 0.25f, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                                NULL
                                        ),
                                        NULL
                                );
                                break;
                        default:
                                break;
                }
        }

        data->state = state;
}

void welcome_controller_on_touch_set_ip(struct native_view_controller *p, struct native_view *sender, u8 type)
{
        struct welcome_controller_data *data = (struct welcome_controller_data *)p->custom_data;
        switch(type) {
                case NATIVE_UI_TOUCH_BEGAN:
                        native_view_clear_all_actions(sender);
                        native_view_run_action(sender,
                               native_view_alpha_to(sender, 0.5f, 0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                               NULL);
                        break;
                case NATIVE_UI_TOUCH_ENDED:
                        __set_state(p, WELCOME_SHOW_SEARCH_IP);
                        break;
                case NATIVE_UI_TOUCH_CANCELLED:
                        __set_state(p, data->state);
                        break;
        }
}

void welcome_controller_on_touch_search_around(struct native_view_controller *p, struct native_view *sender, u8 type)
{
        struct welcome_controller_data *data = (struct welcome_controller_data *)p->custom_data;
        switch(type) {
                case NATIVE_UI_TOUCH_BEGAN:
                        native_view_clear_all_actions(sender);
                        native_view_run_action(sender,
                               native_view_alpha_to(sender, 0.5f, 0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                               NULL);
                        break;
                case NATIVE_UI_TOUCH_ENDED:
                        __set_state(p, WELCOME_SHOW_SEARCH_AROUND);
                        if( ! data->searching_around) {
                                data->searching_around = 1;
                                checking_client_requester_search_around(checking_client_requester_get_instance(),
                                        (struct checking_client_request_search_around_param){
                                                .lat = 23.0,
                                                .lon = 99.122
                                        }
                                );
                        }
                        break;
                case NATIVE_UI_TOUCH_CANCELLED:
                        __set_state(p, data->state);
                        break;
        }
}

void welcome_controller_on_touch_search_ip(struct native_view_controller *p, struct native_view *sender, u8 type)
{

}

void welcome_controller_on_listen_search_around(struct native_view_controller *p, struct smart_object *obj)
{
        struct welcome_controller_data *data    = (struct welcome_controller_data *)p->custom_data;
        data->searching_around                  = 0;

        struct native_view *view                = native_view_controller_get_view(p);
        struct native_view_parser *parser       = native_view_get_parser(view);

        struct native_view *search_around_view  = native_view_parser_get_hash_view(parser, qlkey("search_around_view"));

        struct native_view_parser *search_around_parser       = native_view_get_parser(search_around_view);

        struct native_view *list                = native_view_parser_get_hash_view(search_around_parser ,qlkey("list"));

        native_view_remove_all_children(list);

        struct smart_object *objdata            = smart_object_get_object(obj, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct smart_array *locations           = smart_object_get_array(objdata, qskey(&__key_locations__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int i;
        for_i(i, locations->data->len) {
                struct smart_object *location   = smart_array_get_object(locations, i, SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct native_view_parser *location_parser = native_view_parser_alloc();
                native_view_parser_parse_template(location_parser, search_around_parser, qlkey("item"));

                struct native_view *location_view = native_view_parser_get_view(location_parser);
                native_view_add_child(list, location_view);

                struct native_view *location_text = native_view_parser_get_hash_view(location_parser, qlkey("name"));

                struct string *location_name    = smart_object_get_string(location, qskey(&__key_location_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);

                native_view_set_text(location_text, qskey(location_name));

                native_view_request_layout(location_view);
        }
}
