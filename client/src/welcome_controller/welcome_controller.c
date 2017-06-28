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
#include <checking_client/register_controller/register_controller.h>
#include <checking_client/controller_utils.h>
#include <native_ui/view_controller.h>
#include <native_ui/action.h>
#include <native_ui/touch_handle.h>
#include <native_ui/parser.h>
#include <native_ui/preferences.h>
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

ADD_FUNCTION_LISTEN(struct clwc_exec_data);

ADD_CONTROLLER_DATA_FREE(struct clwc_exec_data, {

});


ADD_CONTROLLER_DATA_ALLOC(struct clwc_exec_data, {
        p->state                = WELCOME_SHOW_SEARCH_IP;
        p->searching_around     = 0;
        map_set(p->cmd_delegate, qskey(&__cmd_location_search_nearby__),
                &(cl_ctrl_listen_delegate){clwc_exec_listen_scan});
        map_set(p->cmd_delegate, qskey(&__cmd_service_get_location_info__),
                &(cl_ctrl_listen_delegate){clwc_exec_listen_loc_info});
});


ADD_CONTROLLER_ALLOC(clwc_exec);

void clwc_exec_on_linked(struct nexec *p)
{
        struct clwc_exec_data *data = (struct clwc_exec_data *)p->custom_data;

        struct nview *view = nexec_get_view(p);
        struct nparser *parser = nview_get_parser(view);

        REGISTER_TOUCH(parser, qlkey("set_ip"), clwc_exec_touch_set_ip, p, NULL)
        REGISTER_TOUCH(parser, qlkey("search_ip"), clwc_exec_touch_search_ip, p, NULL)
        REGISTER_TOUCH(parser, qlkey("search_around"), clwc_exec_touch_scan, p, NULL)
        REGISTER_TOUCH(parser, qlkey("searched_item"), clwc_exec_touch_item, p, NULL)

        cl_talker_add_context(cl_talker_get_instance(), data->response_context);
}

void clwc_exec_on_removed(struct nexec *p)
{
        struct clwc_exec_data *data = (struct clwc_exec_data *)p->custom_data;
        cl_listener_clear(data->response_context);
}

static void __set_state(struct nexec *p, u8 state)
{
        struct nview *view = nexec_get_view(p);
        struct nparser *parser = nview_get_parser(view);

        struct nview *set_ip_button = nparser_get_hash_view(parser, qlkey("set_ip_button"));
        struct nview *search_around_button = nparser_get_hash_view(parser, qlkey("search_around_button"));
        struct nview *search_box_view = nparser_get_hash_view(parser, qlkey("search_box_view"));
        struct nview *search_around_view = nparser_get_hash_view(parser, qlkey("search_around_view"));

        struct clwc_exec_data *data = (struct clwc_exec_data *)p->custom_data;
        switch (state) {
                case WELCOME_SHOW_SEARCH_IP:
                        nview_clear_all_actions(set_ip_button);
                        nview_clear_all_actions(search_around_button);
                        nview_run_action(set_ip_button,
                                nview_alpha_to(set_ip_button,
                                        1.0f,
                                        0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                NULL);
                        nview_run_action(search_around_button,
                                nview_alpha_to(search_around_button,
                                        0.35f,
                                        0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                NULL);
                        break;
                case WELCOME_SHOW_SEARCH_AROUND:
                        nview_clear_all_actions(set_ip_button);
                        nview_clear_all_actions(search_around_button);
                        nview_run_action(set_ip_button,
                                nview_alpha_to(set_ip_button,
                                        0.35f,
                                        0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                NULL);
                        nview_run_action(search_around_button,
                                nview_alpha_to(search_around_button,
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
                                nview_clear_all_actions(search_box_view);
                                nview_clear_all_actions(search_around_view);
                                nview_run_action(search_box_view,
                                        naction_sequence(
                                                nview_show(search_box_view),
                                                nview_alpha_to(search_box_view,
                                                        1.0f, 0.25f, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                                NULL
                                        ),
                                        NULL
                                );
                                nview_run_action(search_around_view,
                                        naction_sequence(
                                                nview_alpha_to(search_around_view,
                                                        0.0f, 0.25f, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                                nview_hide(search_around_view),
                                                NULL
                                        ),
                                        NULL
                                );
                                break;
                        case WELCOME_SHOW_SEARCH_AROUND:
                                nview_clear_all_actions(search_box_view);
                                nview_clear_all_actions(search_around_view);
                                nview_run_action(search_box_view,
                                        naction_sequence(
                                                nview_alpha_to(search_box_view,
                                                        0.0f, 0.25f, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                                                nview_hide(search_box_view),
                                                NULL
                                        ),
                                        NULL
                                );
                                nview_run_action(search_around_view,
                                        naction_sequence(
                                                nview_show(search_around_view),
                                                nview_alpha_to(search_around_view,
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

/*
 * touch delegates
 */
void clwc_exec_touch_set_ip(struct nexec *p, struct nview *sender, u8 type)
{
        struct clwc_exec_data *data = (struct clwc_exec_data *)p->custom_data;
        switch(type) {
                case NATIVE_UI_TOUCH_BEGAN:
                        nview_clear_all_actions(sender);
                        nview_run_action(sender,
                               nview_alpha_to(sender, 0.5f, 0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
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

void clwc_exec_touch_scan(struct nexec *p, struct nview *sender, u8 type)
{
        struct clwc_exec_data *data = (struct clwc_exec_data *)p->custom_data;
        switch(type) {
                case NATIVE_UI_TOUCH_BEGAN:
                        nview_clear_all_actions(sender);
                        nview_run_action(sender,
                               nview_alpha_to(sender, 0.5f, 0.25, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                               NULL);
                        break;
                case NATIVE_UI_TOUCH_ENDED:
                        __set_state(p, WELCOME_SHOW_SEARCH_AROUND);
                        if( ! data->searching_around) {
                                data->searching_around = 1;
                                // cl_talker_search_around(cl_talker_get_instance(),
                                //         (struct checking_client_request_search_around_param){
                                //                 .lat = 21.0141,
                                //                 .lon = 105.8499
                                //         }
                                // );
                        }
                        break;
                case NATIVE_UI_TOUCH_CANCELLED:
                        __set_state(p, data->state);
                        break;
        }
}

static void __get_location_info(struct nexec *p, char *host, size_t host_len, int port)
{
        // struct nview *view                = nexec_get_view(p);
        // struct nparser *parser       = nview_get_parser(view);
        // struct nview *search_box_view     = nparser_get_hash_view(parser, qlkey("search_box_view"));
        // struct nparser *search_box_view_parser = nview_get_parser(search_box_view);
        //
        // struct npref *pref = npref_get(qlkey("pref/data.json"));
        //
        // smart_object_set_string(pref->data, qskey(&__key_ip__), host, host_len);
        // smart_object_set_int(pref->data, qskey(&__key_port__), port);
        // npref_save(pref);
        //
        // struct nview *prevent_touch = nparser_get_hash_view(parser, qlkey("prevent_touch"));
        // nview_set_user_interaction_enabled(prevent_touch, 1);
        //
        // cl_talker_service_get_location_info(cl_talker_get_instance(),
        //         (struct checking_client_request_service_get_location_info_param){
        //                 .host = host,
        //                 .port = port
        //         }
        // );
}

void clwc_exec_touch_search_ip(struct nexec *p, struct nview *sender, u8 type)
{
        struct clwc_exec_data *data = (struct clwc_exec_data *)p->custom_data;
        switch(type) {
                case NATIVE_UI_TOUCH_ENDED:
                        goto find_ip;
                default:
                        goto end;
        }
find_ip:;
        controller_get_view(view, p);
        view_get_parser(parser, view);
        parser_hash_view(search_box_view, parser, qlkey("search_box_view"));
        struct nparser *search_box_view_parser = nview_get_parser(search_box_view);

        struct nview *box = nparser_get_hash_view(search_box_view_parser, qlkey("box"));
        struct nview *box_2 = nparser_get_hash_view(search_box_view_parser, qlkey("box_2"));
        struct string *host = nview_get_text(box);
        struct string *port = nview_get_text(box_2);
        string_trim(host);
        string_trim(port);
        int port_num = atoi(port->ptr);
        if(host->len && port_num > 0) {
                nview_set_visible(nparser_get_hash_view(search_box_view_parser, qlkey("notification")), 0);
                __get_location_info(p, host->ptr, host->len, port_num);
        }
        string_free(host);
        string_free(port);
end:;
}

void clwc_exec_touch_item(struct nexec *p, struct nview *sender, u8 type)
{
        switch (type) {
                case NATIVE_UI_TOUCH_ENDED:
                        goto to_register;
                default:
                        return;
        }
to_register:;
        object_cast(data, sender->user_data);
        object_get_string(host, data, qskey(&__key_ip__));
        object_get_int(port, data, qskey(&__key_port__));
        __get_location_info(p, host->ptr, host->len, port);
}

/*
 * listen delegates
 */
void clwc_exec_listen_loc_info(struct nexec *p, struct smart_object *obj)
{
        controller_get_view(view, p);
        view_get_parser(parser, view);
        parser_hash_view(search_box_view, parser, qlkey("search_box_view"));
        view_get_parser(search_box_view_parser, search_box_view);
        parser_hash_view(prevent_touch, parser, qlkey("prevent_touch"));
        object_get_bool(result, obj, qskey(&__key_result__));
        nview_set_user_interaction_enabled(prevent_touch, 0);

        if(result) {
                controller_parse(register_controller,
                        .file = "res/layout/register/register.xml",
                        .controller_parent = p->parent,
                        .controller_dismiss = p,
                        .view_parent = view->parent
                );
                object_get_object(data, obj, qskey(&__key_data__));
                object_get_string(location_name, data, qskey(&__key_location_name__));
                register_controller_set_location_name(register_controller, qskey(location_name));
        } else {
                nview_set_visible(
                        parser_hash_view(search_box_view_parser, qlkey("notification")),
                        1);
        }
}

void clwc_exec_listen_scan(struct nexec *p, struct smart_object *obj)
{
        struct clwc_exec_data *data    = (struct clwc_exec_data *)p->custom_data;
        data->searching_around                  = 0;

        struct nview *view                = nexec_get_view(p);
        struct nparser *parser       = nview_get_parser(view);

        struct nview *search_around_view  = nparser_get_hash_view(parser, qlkey("search_around_view"));

        struct nparser *search_around_parser       = nview_get_parser(search_around_view);

        struct nview *list                = nparser_get_hash_view(search_around_parser ,qlkey("list"));

        nview_remove_all_children(list);

        struct smart_object *objdata            = smart_object_get_object(obj, qskey(&__key_data__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct smart_array *locations           = smart_object_get_array(objdata, qskey(&__key_locations__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int i;
        for_i(i, locations->data->len) {
                struct smart_object *location   = smart_array_get_object(locations, i, SMART_GET_REPLACE_IF_WRONG_TYPE);

                struct nparser *location_parser = nparser_alloc();
                nparser_parse_template(location_parser, search_around_parser, qlkey("item"));

                struct nview *location_view = nparser_get_view(location_parser);
                nview_add_child(list, location_view);

                location_view->user_data        = smart_object_clone(location);
                location_view->user_data_free   = smart_object_free;

                struct nview *location_text = nparser_get_hash_view(location_parser, qlkey("name"));

                struct string *location_name    = smart_object_get_string(location, qskey(&__key_location_name__), SMART_GET_REPLACE_IF_WRONG_TYPE);

                nview_set_text(location_text, qskey(location_name));

                nview_request_layout(location_view);
        }
}
