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

#define REGISTER_TOUCH(parser, name, handle, data, fdel) \
{       \
        struct native_view_touch_handle *th = native_view_parser_get_touch_handle(parser, name);      \
        if(th) {        \
                native_view_touch_handle_set_touch_delegate(th, handle, data, fdel);    \
        }       \
}

static struct welcome_controller_data *__welcome_controller_data_alloc()
{
        struct welcome_controller_data *p       = smalloc(sizeof(struct welcome_controller_data));
        p->state                                = WELCOME_SHOW_SEARCH_IP;
        return p;
}

static void __welcome_controller_data_free(struct welcome_controller_data *p)
{
        sfree(p);
}

struct native_view_controller *welcome_controller_alloc()
{
        struct native_view_controller *p        = native_view_controller_alloc();
        p->on_linked                            = welcome_controller_on_linked;
        p->on_removed                           = welcome_controller_on_removed;
        p->custom_data                          = __welcome_controller_data_alloc();
        p->custom_data_free                     = __welcome_controller_data_free;
        return p;
}

void welcome_controller_on_linked(struct native_view_controller *p)
{
        struct native_view *view = native_view_controller_get_view(p);
        struct native_view_parser *parser = native_view_get_parser(view);

        REGISTER_TOUCH(parser, qlkey("set_ip"), welcome_controller_on_touch_set_ip, p, NULL)
        REGISTER_TOUCH(parser, qlkey("search_ip"), welcome_controller_on_touch_search_ip, p, NULL)
        REGISTER_TOUCH(parser, qlkey("search_around"), welcome_controller_on_touch_search_around, p, NULL)
}

void welcome_controller_on_removed(struct native_view_controller *p)
{

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
                        break;
                case NATIVE_UI_TOUCH_CANCELLED:
                        __set_state(p, data->state);
                        break;
        }
}

void welcome_controller_on_touch_search_ip(struct native_view_controller *p, struct native_view *sender, u8 type)
{

}
