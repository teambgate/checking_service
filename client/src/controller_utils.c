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
#include <checking_client/controller_utils.h>
#include <checking_client/root_view_controller/root_view_controller.h>
#include <checking_client/welcome_controller/welcome_controller.h>
#include <checking_client/register_controller/register_controller.h>
#include <cherry/string.h>
#include <native_ui/parser.h>
#include <native_ui/view.h>
#include <native_ui/view_controller.h>

struct nexec *checking_client_nexec_alloc(char *name, size_t len)
{
        if(strcmp(name, "welcome_controller") == 0) {
                return clwc_exec_alloc();
        } else if(strcmp(name, "root_view_controller") == 0) {
                return root_view_controller_alloc();
        } else if(strcmp(name, "register_controller") == 0) {
                return register_controller_alloc();
        }
        return NULL;
}

struct nexec *checking_client_view_controller_parse(struct view_controller_parse_param param)
{
        if(param.file) {
                struct nparser *parser = nparser_alloc();
                nparser_parse_file(parser, param.file, NULL);

                struct nview *view = nparser_get_view(parser);
                struct nexec *controller = parser->controller;
                if(controller && param.controller_parent) {
                        nexec_link(param.controller_parent, controller);
                }
                if(view && param.view_parent) {
                        nview_add_child(param.view_parent, view);
                }
                if(param.controller_dismiss)
                        nexec_free(param.controller_dismiss);

                nview_request_layout(view);

                return controller;
        }
        return NULL;
}
