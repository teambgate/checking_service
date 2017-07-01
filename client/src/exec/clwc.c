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
#include <checking_client/exec.h>
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

static void clwc_exec_on_linked(struct nexec *p);
static void clwc_exec_on_removed(struct nexec *p);
static void clwc_exec_touch_set_ip(struct nexec *p, struct nview *sender, u8 type);
static void clwc_exec_touch_scan(struct nexec *p, struct nview *sender, u8 type);
static void clwc_exec_touch_search_ip(struct nexec *p, struct nview *sender, u8 type);
static void clwc_exec_touch_item(struct nexec *p, struct nview *sender, u8 type);
static void clwc_exec_listen_loc_info(struct nexec *p, struct sobj *obj);
static void clwc_exec_listen_scan(struct nexec *p, struct sobj *obj);

enum {
        WELCOME_SHOW_SEARCH_IP,
        WELCOME_SHOW_SEARCH_AROUND,
        WELCOME_SEARCHING_IP,
        WELCOME_SEARCHING_AROUND
};

struct exec_data {
        u8                          state;
        struct cl_listener          *lsr;
        struct map                  *cmds;
        u8                          searching_around;
};

static void nexec_listen(struct nexec *p, struct sobj *obj)
{
        struct exec_data *data = (struct exec_data *)p->custom_data;
        struct string *cmd = sobj_get_str(obj, qskey(&__key_cmd__), RPL_TYPE);
        nexec_listenf *delegate = map_get_pointer(data->cmds, qskey(cmd));

        if(*delegate) {
                (*delegate)(p, obj);
        }
}

static void exec_data_free(struct exec_data *p)
{
        cl_listener_free(p->lsr);
        map_free(p->cmds);
        sfree(p);
}

static struct exec_data *exec_data_alloc(struct nexec *controller)
{
        struct exec_data *p                     = smalloc(sizeof(struct exec_data), exec_data_free);
        p->lsr                     = cl_listener_alloc();
        p->lsr->ctx                = controller;
        p->lsr->delegate           = nexec_listen;
        p->cmds                         = map_alloc(sizeof(nexec_listenf));

        p->state                = WELCOME_SHOW_SEARCH_IP;
        p->searching_around     = 0;
        map_set(p->cmds, qskey(&__cmd_location_search_nearby__),
                &(nexec_listenf){clwc_exec_listen_scan});
        map_set(p->cmds, qskey(&__cmd_service_get_location_info__),
                &(nexec_listenf){clwc_exec_listen_loc_info});

        return p;
}

struct nexec *clwc_exec_alloc()
{
        struct nexec *p                         = nexec_alloc();
        p->on_linked                            = clwc_exec_on_linked;
        p->on_removed                           = clwc_exec_on_removed;
        p->custom_data                          = exec_data_alloc(p);
        p->custom_data_free                     = exec_data_free;
        return p;
}

static void clwc_exec_on_linked(struct nexec *p)
{
        struct exec_data *data = (struct exec_data *)p->custom_data;

        struct nview *view = nexec_get_view(p);
        struct nparser *pr = nview_get_parser(view);

        struct ntouch *t = nparser_get_touch(pr, qlkey("set_ip"));
        ntouch_set_f(t, clwc_exec_touch_set_ip, p, NULL);
        t = nparser_get_touch(pr, qlkey("search_ip"));
        ntouch_set_f(t, clwc_exec_touch_search_ip, p, NULL);
        t = nparser_get_touch(pr, qlkey("search_around"));
        ntouch_set_f(t, clwc_exec_touch_scan, p, NULL);
        t = nparser_get_touch(pr, qlkey("searched_item"));
        ntouch_set_f(t, clwc_exec_touch_item, p, NULL);

        cl_talker_add_context(cl_talker_shared(), data->lsr);
}

static void clwc_exec_on_removed(struct nexec *p)
{
        struct exec_data *data = (struct exec_data *)p->custom_data;
        cl_listener_clear(data->lsr);
}

static void __set_state(struct nexec *p, u8 state)
{
        struct nview *view = nexec_get_view(p);
        struct nparser *parser = nview_get_parser(view);

        struct nview *set_ip_button = nparser_get_hash_view(parser, qlkey("set_ip_button"));
        struct nview *search_around_button = nparser_get_hash_view(parser, qlkey("search_around_button"));
        struct nview *search_box_view = nparser_get_hash_view(parser, qlkey("search_box_view"));
        struct nview *search_around_view = nparser_get_hash_view(parser, qlkey("search_around_view"));

        struct exec_data *data = (struct exec_data *)p->custom_data;
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
static void clwc_exec_touch_set_ip(struct nexec *p, struct nview *sender, u8 type)
{
        struct exec_data *data = (struct exec_data *)p->custom_data;
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

static void clwc_exec_touch_scan(struct nexec *p, struct nview *sender, u8 type)
{
        struct exec_data *data = (struct exec_data *)p->custom_data;
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

                                struct sobj *o = sobj_alloc();
                                sobj_set_str(o, qskey(&__key_cmd__), qskey(&__cmd_location_search_nearby__));

                                struct sobj *ll = sobj_get_obj(o, qskey(&__key_latlng__), RPL_TYPE);
                                sobj_set_f64(ll, qskey(&__key_lat__), 21.0141);
                                sobj_set_f64(ll, qskey(&__key_lon__), 105.8499);

                                cl_talker_send(cl_talker_shared(), sobj_clone(o), __dst_sup);
                                sobj_free(o);
                        }
                        break;
                case NATIVE_UI_TOUCH_CANCELLED:
                        __set_state(p, data->state);
                        break;
        }
}

static void __get_location_info(struct nexec *p, char *host, size_t host_len, int port)
{
        struct nview *view                = nexec_get_view(p);
        struct nparser *parser       = nview_get_parser(view);
        struct nview *search_box_view     = nparser_get_hash_view(parser, qlkey("search_box_view"));
        struct nparser *search_box_view_parser = nview_get_parser(search_box_view);

        struct npref *pref = npref_get(qlkey("pref/data.json"));

        sobj_set_str(pref->data, qskey(&__key_ip__), host, host_len);
        sobj_set_i32(pref->data, qskey(&__key_port__), port);
        npref_save(pref);

        struct nview *prevent_touch = nparser_get_hash_view(parser, qlkey("prevent_touch"));
        nview_set_user_interaction_enabled(prevent_touch, 1);

        struct sobj *o = sobj_alloc();
        sobj_set_str(o, qskey(&__key_cmd__), qskey(&__cmd_service_get_location_info__));

        struct cl_dst dst = __dst_srv;
        dst.ip = host;
        dst.port = port;

        cl_talker_send(cl_talker_shared(), o, dst);
}

static void clwc_exec_touch_search_ip(struct nexec *p, struct nview *sender, u8 type)
{
        struct exec_data *data = (struct exec_data *)p->custom_data;
        switch(type) {
                case NATIVE_UI_TOUCH_ENDED:
                        goto find_ip;
                default:
                        goto end;
        }
find_ip:;
        debug("native create touch search 1\n");
        controller_get_view(view, p);
        view_get_parser(parser, view);
        parser_hash_view(search_box_view, parser, qlkey("search_box_view"));
        struct nparser *search_box_view_parser = nview_get_parser(search_box_view);
        debug("native create touch search 2\n");
        struct nview *box = nparser_get_hash_view(search_box_view_parser, qlkey("box"));
        struct nview *box_2 = nparser_get_hash_view(search_box_view_parser, qlkey("box_2"));
        struct string *host = nview_get_text(box);
        struct string *port = nview_get_text(box_2);
        string_trim(host);
        string_trim(port);
        debug("native create touch search 3\n");
        int port_num = atoi(port->ptr);
        if(host->len && port_num > 0) {
                debug("native create touch search 4\n");
                nview_set_visible(nparser_get_hash_view(search_box_view_parser, qlkey("notification")), 0);
                __get_location_info(p, host->ptr, host->len, port_num);
        }
        string_free(host);
        string_free(port);
        debug("native create touch search 5\n");
end:;
}

static void clwc_exec_touch_item(struct nexec *p, struct nview *sender, u8 type)
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
static void clwc_exec_listen_loc_info(struct nexec *p, struct sobj *obj)
{
        struct nview *v = nexec_get_view(p);
        struct nparser *pr = nview_get_parser(v);

        struct nview *sbv = nparser_get_hash_view(pr, qlkey("search_box_view"));
        struct nparser *sbv_pr = nview_get_parser(sbv);

        struct nview *ptc = nparser_get_hash_view(pr, qlkey("prevent_touch"));
        nview_set_user_interaction_enabled(ptc, 0);

        u8 r = sobj_get_u8(obj, qskey(&__key_result__), RPL_TYPE);

        if(r) {
                debug("native create reg 1\n");
                struct nexec *exec = nexec_parse(
                        (struct nexec_ppr) {
                                .file = "res/layout/register/register.xml",
                                .exec_p = p->parent,
                                //.exec_d = p,
                                .view_p = v->parent
                        }
                );

                struct sobj *data = sobj_get_obj(obj, qskey(&__key_data__), RPL_TYPE);
                struct string *ln = sobj_get_str(data, qskey(&__key_location_name__), RPL_TYPE);
                clreg_exec_sname(exec, qskey(ln));
                debug("native create reg 2\n");
                nview_request_layout(v);

                struct nview *regv = nexec_get_view(exec);
                nview_request_margin(regv, (union vec4){
                        .left = regv->size.width
                });
                debug("native create reg 3\n");
                nview_clear_all_actions(regv);
                nview_run_action(regv,
                        naction_sequence(
                                nview_margin_to(regv, (union vec4){
                                        .left = 0
                                }, 0.3, NATIVE_UI_EASE_CUBIC_OUT, 0),
                                NULL
                        ),
                       NULL);
        } else {
                nview_set_visible(
                        nparser_get_hash_view(sbv, qlkey("notification")),
                        1);
        }
}

static void clwc_exec_listen_scan(struct nexec *p, struct sobj *obj)
{
        struct exec_data *data    = (struct exec_data *)p->custom_data;
        data->searching_around                  = 0;

        struct nview *view                = nexec_get_view(p);
        struct nparser *parser       = nview_get_parser(view);

        struct nview *search_around_view  = nparser_get_hash_view(parser, qlkey("search_around_view"));

        struct nparser *search_around_parser       = nview_get_parser(search_around_view);

        struct nview *list                = nparser_get_hash_view(search_around_parser ,qlkey("list"));

        nview_remove_all_children(list);

        struct sobj *objdata            = sobj_get_obj(obj, qskey(&__key_data__), RPL_TYPE);
        struct sarray *locations           = sobj_get_arr(objdata, qskey(&__key_locations__), RPL_TYPE);
        int i;
        for_i(i, locations->data->len) {
                struct sobj *location   = sarray_get_obj(locations, i, RPL_TYPE);

                struct nparser *location_parser = nparser_alloc();
                nparser_parse_template(location_parser, search_around_parser, qlkey("item"));

                struct nview *location_view = nparser_get_view(location_parser);
                nview_add_child(list, location_view);

                location_view->user_data        = sobj_clone(location);
                location_view->user_data_free   = sobj_free;

                struct nview *location_text = nparser_get_hash_view(location_parser, qlkey("name"));

                struct string *location_name    = sobj_get_str(location, qskey(&__key_location_name__), RPL_TYPE);

                nview_set_text(location_text, qskey(location_name));

                nview_request_layout(location_view);
        }
}
