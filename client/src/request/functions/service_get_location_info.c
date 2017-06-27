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
#include <checking_client/request/functions/service_get_location_info.h>
#include <checking_client/request/request.h>
#include <smartfox/data.h>
#include <common/key.h>
#include <common/command.h>
#include <common/request.h>
#include <cherry/string.h>
#include <cherry/array.h>
#include <cherry/map.h>

void checking_client_requester_service_get_location_info(struct checking_client_requester *p,
        struct checking_client_request_service_get_location_info_param param)
{
        struct string *version_code = smart_object_get_string(p->config, qlkey("service_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *pass = smart_object_get_string(p->config, qlkey("service_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct smart_object *data = smart_object_alloc();
        smart_object_set_string(data, qskey(&__key_version__), qskey(version_code));
        smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_service_get_location_info__));
        smart_object_set_string(data, qskey(&__key_pass__),qskey(pass));

        cs_request_alloc_with_param(p->requester, data,
                (cs_request_callback)checking_client_requester_callback, p,
                (struct cs_request_param){
                        .host = param.host,
                        .port = param.port
                }
        );
}
