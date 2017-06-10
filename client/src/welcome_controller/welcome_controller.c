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
        struct welcome_controller_data *data = (struct welcome_controller_data *)p->custom_data;
        switch (data->state) {
                case WELCOME_SEARCHING_IP:
                        break;
                default:
                        break;
        }
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
                        native_view_clear_all_actions(sender);
                        switch (data->state) {
                                case WELCOME_SHOW_SEARCH_IP:
                                case WELCOME_SEARCHING_IP:
                                        native_view_run_action(sender,
                                                native_view_alpha_to(sender,
                                                        1.0f,
                                                        0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                                NULL);
                                        break;
                                case WELCOME_SHOW_SEARCH_AROUND:
                                case WELCOME_SEARCHING_AROUND:
                                        native_view_run_action(sender,
                                                native_view_alpha_to(sender,
                                                        0.35f,
                                                        0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                                NULL);
                                        break;
                        }
                        break;
        }
}

void welcome_controller_on_touch_search_around(struct native_view_controller *p, struct native_view *sender, u8 type)
{
        switch(type) {
                case NATIVE_UI_TOUCH_BEGAN:
                        native_view_clear_all_actions(sender);
                        native_view_run_action(sender,
                               native_view_alpha_to(sender, 0.5f, 0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                               NULL);
                        break;
                case NATIVE_UI_TOUCH_ENDED:
                case NATIVE_UI_TOUCH_CANCELLED:
                        native_view_clear_all_actions(sender);
                        native_view_run_action(sender,
                                native_view_alpha_to(sender, 1.0f, 0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                NULL);
                        break;
        }
}

void welcome_controller_on_touch_search_ip(struct native_view_controller *p, struct native_view *sender, u8 type)
{

}
