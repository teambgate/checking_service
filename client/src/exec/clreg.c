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

static void clreg_exec_on_linked(struct nexec *p);
static void clreg_exec_on_removed(struct nexec *p);
static void clreg_exec_touch_hover(struct nexec *p, struct nview *s, u8 t);

struct exec_data {
        struct cl_listener      *lsr;
        struct map              *cmds;
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

        return p;
}

struct nexec *clreg_exec_alloc()
{
        struct nexec *p                         = nexec_alloc();
        p->on_linked                            = clreg_exec_on_linked;
        p->on_removed                           = clreg_exec_on_removed;
        p->custom_data                          = exec_data_alloc(p);
        p->custom_data_free                     = exec_data_free;
        return p;
}

static void clreg_exec_on_linked(struct nexec *p)
{
        struct exec_data *data = (struct exec_data *)p->custom_data;

        struct nview *view = nexec_get_view(p);
        struct nparser *parser = nview_get_parser(view);

        struct ntouch *t = nparser_get_touch(parser, qlkey("hover"));
        ntouch_set_f(t, clreg_exec_touch_hover, p, NULL);

        cl_talker_add_context(cl_talker_shared(), data->lsr);
}

static void clreg_exec_on_removed(struct nexec *p)
{
        struct exec_data *data = (struct exec_data *)p->custom_data;
        cl_listener_clear(data->lsr);
}

static void clreg_exec_touch_hover(struct nexec *p, struct nview *s, u8 t)
{
        switch (t) {
                case NATIVE_UI_TOUCH_MOVED:
                case NATIVE_UI_TOUCH_ENDED:
                {
                        struct nparser *pr = nview_get_parser(s);
                        struct nview *m = nparser_get_hash_view(pr, qlkey("content"));
                        nview_request_margin(m, (union vec4){
                                .left = m->align->margin.left + s->touch_offset.x
                        });
                }
                        break;
                default:
                        break;
        }
}

void clreg_exec_sname(struct nexec *p, char *name, i32 len)
{
        struct nview *view = nexec_get_view(p);
        struct nparser *parser = nview_get_parser(view);

        struct nview *label = nparser_get_hash_view(parser, qlkey("location_name"));

        nview_set_text(label, name, len);
}
