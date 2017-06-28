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
#ifndef __CHECKING_SERVICE_CHECKING_CLIENT_TYPES_H__
#define __CHECKING_SERVICE_CHECKING_CLIENT_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <native_ui/types.h>
#include <common/types.h>
#include <smartfox/types.h>

typedef void(*cl_ctrl_listen_delegate)(struct nexec *, struct smart_object *);

#define DEFINE_CONTROLLER(name) \
struct nexec *name##_alloc();      \
void name##_on_linked(struct nexec *p);    \
void name##_on_removed(struct nexec *p);

#define REGISTER_TOUCH(parser, name, handle, data, fdel) \
{       \
        struct ntouch *th = nparser_get_touch(parser, name);      \
        if(th) {        \
                ntouch_set_f(th, handle, data, fdel);    \
        }       \
}

#define ADD_FUNCTION_LISTEN(data_type) \
static void __controller_on_listen(struct nexec *p, struct smart_object *obj)  \
{       \
        data_type *data = (data_type *)p->custom_data;      \
        struct string *cmd = smart_object_get_string(obj, qskey(&__key_cmd__), SMART_GET_REPLACE_IF_WRONG_TYPE);        \
        cl_ctrl_listen_delegate *delegate = map_get_pointer(data->cmd_delegate, qskey(cmd));   \
\
        if(*delegate) { \
                (*delegate)(p, obj);    \
        }       \
}

#define ADD_CONTROLLER_DATA_FREE(data_type, custom)      \
static void __controller_data_free(data_type *p)   \
{       \
        cl_listener_free(p->response_context);   \
        map_free(p->cmd_delegate);      \
        custom  \
        sfree(p);       \
}

#define ADD_CONTROLLER_DATA_ALLOC(data_type, custom)      \
static data_type *__controller_data_alloc(struct nexec *controller)       \
{       \
        data_type *p                            = smalloc(sizeof(data_type), __controller_data_free);      \
        p->response_context                     = cl_listener_alloc();   \
        p->response_context->ctx                = controller;   \
        p->response_context->delegate           = __controller_on_listen;       \
        p->cmd_delegate                         = map_alloc(sizeof(cl_ctrl_listen_delegate));  \
        custom  \
        return p;       \
}

#define ADD_CONTROLLER_ALLOC(name)      \
struct nexec *name##_alloc()       \
{       \
        struct nexec *p        = nexec_alloc();       \
        p->on_linked                            = name##_on_linked; \
        p->on_removed                           = name##_on_removed;        \
        p->custom_data                          = __controller_data_alloc(p);   \
        p->custom_data_free                     = __controller_data_free;       \
        return p;       \
}

/*
 *
 *
 * quick functions
 *
 *
 */
#define CVP(val, ...) \
        struct nexec *val = checking_client_view_controller_parse(\
                (struct view_controller_parse_param) {\
                        __VA_ARGS__\
                }\
        )

#define controller_parse(val, ...) \
        struct nexec *val = checking_client_view_controller_parse(\
                (struct view_controller_parse_param) {\
                        __VA_ARGS__\
                }\
        )

#ifdef __cplusplus
}
#endif

#endif
