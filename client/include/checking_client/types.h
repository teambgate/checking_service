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

typedef void(*view_controller_command_delegate)(struct native_view_controller *, struct smart_object *);

#define DEFINE_CONTROLLER(name) \
struct native_view_controller *name##_alloc();      \
void name##_on_linked(struct native_view_controller *p);    \
void name##_on_removed(struct native_view_controller *p);

#define REGISTER_TOUCH(parser, name, handle, data, fdel) \
{       \
        struct native_view_touch_handle *th = native_view_parser_get_touch_handle(parser, name);      \
        if(th) {        \
                native_view_touch_handle_set_touch_delegate(th, handle, data, fdel);    \
        }       \
}

#define ADD_FUNCTION_LISTEN(data_type) \
static void __controller_on_listen(struct native_view_controller *p, struct smart_object *obj)  \
{       \
        data_type *data = (data_type *)p->custom_data;      \
        struct string *cmd = smart_object_get_string(obj, qskey(&__key_cmd__), SMART_GET_REPLACE_IF_WRONG_TYPE);        \
        view_controller_command_delegate *delegate = map_get_pointer(data->cmd_delegate, qskey(cmd));   \
\
        if(*delegate) { \
                (*delegate)(p, obj);    \
        }       \
}

#define ADD_CONTROLLER_DATA_ALLOC(data_type, custom)      \
static data_type *__controller_data_alloc(struct native_view_controller *controller)       \
{       \
        data_type *p                            = smalloc(sizeof(data_type));      \
        p->response_context                     = checking_client_requester_response_context_alloc();   \
        p->response_context->ctx                = controller;   \
        p->response_context->delegate           = __controller_on_listen;       \
        p->cmd_delegate                         = map_alloc(sizeof(view_controller_command_delegate));  \
        custom  \
        return p;       \
}

#define ADD_CONTROLLER_DATA_FREE(data_type, custom)      \
static void __controller_data_free(data_type *p)   \
{       \
        checking_client_requester_response_context_free(p->response_context);   \
        map_free(p->cmd_delegate);      \
        custom  \
        sfree(p);       \
}

#define ADD_CONTROLLER_ALLOC(name)      \
struct native_view_controller *name##_alloc()       \
{       \
        struct native_view_controller *p        = native_view_controller_alloc();       \
        p->on_linked                            = name##_on_linked; \
        p->on_removed                           = name##_on_removed;        \
        p->custom_data                          = __controller_data_alloc(p);   \
        p->custom_data_free                     = __controller_data_free;       \
        return p;       \
}

#ifdef __cplusplus
}
#endif

#endif
