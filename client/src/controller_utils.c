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
#include <checking_client/exec.h>
#include <cherry/string.h>
#include <native_ui/parser.h>
#include <native_ui/view.h>
#include <native_ui/view_controller.h>

struct nexec *cl_nexec_alloc(char *name, size_t len)
{
        if(strcmp(name, "welcome_controller") == 0) {                
                return clwc_exec_alloc();
        } else if(strcmp(name, "root_view_controller") == 0) {
                return clrt_exec_alloc();
        } else if(strcmp(name, "register_controller") == 0) {
                return clreg_exec_alloc();
        }
        return NULL;
}

struct nexec *nexec_parse(struct nexec_ppr param)
{
        if(param.file) {
                struct nparser *parser = nparser_alloc();
                nparser_parse_file(parser, param.file, NULL);

                struct nview *view = nparser_get_view(parser);
                struct nexec *controller = parser->controller;
                if(controller && param.exec_p) {
                        nexec_link(param.exec_p, controller);
                }
                if(view && param.view_p) {
                        nview_add_child(param.view_p, view);
                }
                if(param.exec_d)
                        nexec_free(param.exec_d);

                nview_request_layout(view);

                return controller;
        }
        return NULL;
}
