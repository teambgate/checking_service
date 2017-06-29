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
#include <native_ui/view_controller.h>
#include <checking_client/request/request.h>
#include <smartfox/data.h>
#include <cherry/stdio.h>
#include <cherry/array.h>
#include <cherry/map.h>

static struct sobj *cfg = NULL;

static void __clear()
{
        if(cfg) {
                sobj_free(cfg);
                cfg = NULL;
        }
}

static void __setup()
{
        if(!cfg) {
                cfg = sobj_from_json_file("res/config.json", FILE_INNER);
        }
}

struct nexec *clrt_exec_alloc()
{
        __setup();
        struct nexec *p         = nexec_alloc();

        __dst_srv.ip = sobj_get_str(cfg, qlkey("service_host"), RPL_TYPE)->ptr;
        __dst_srv.port = sobj_get_int(cfg, qlkey("service_port"), RPL_TYPE);
        __dst_srv.ver = sobj_get_str(cfg, qlkey("service_version_code"), RPL_TYPE)->ptr;
        __dst_srv.ver_len = sobj_get_str(cfg, qlkey("service_version_code"), RPL_TYPE)->len;
        __dst_srv.pss = sobj_get_str(cfg, qlkey("service_pass"), RPL_TYPE)->ptr;
        __dst_srv.pss_len = sobj_get_str(cfg, qlkey("service_pass"), RPL_TYPE)->len;

        __dst_sup.ip = sobj_get_str(cfg, qlkey("supervisor_host"), RPL_TYPE)->ptr;
        __dst_sup.port = sobj_get_int(cfg, qlkey("supervisor_port"), RPL_TYPE);
        __dst_sup.ver = sobj_get_str(cfg, qlkey("supervisor_version_code"), RPL_TYPE)->ptr;
        __dst_sup.ver_len = sobj_get_str(cfg, qlkey("supervisor_version_code"), RPL_TYPE)->len;
        __dst_sup.pss = sobj_get_str(cfg, qlkey("supervisor_pass"), RPL_TYPE)->ptr;
        __dst_sup.pss_len = sobj_get_str(cfg, qlkey("supervisor_pass"), RPL_TYPE)->len;

        return p;
}
