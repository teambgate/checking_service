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
#include <checking_client/request/functions/search_around.h>
#include <checking_client/request/request.h>
#include <smartfox/data.h>
#include <common/key.h>
#include <common/command.h>
#include <common/request.h>
#include <cherry/string.h>
#include <cherry/array.h>
#include <cherry/map.h>

void checking_client_requester_search_around(struct checking_client_requester *p,
        struct checking_client_request_search_around_param param)
{
        struct string *version_code = smart_object_get_string(p->config, qlkey("supervisor_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *pass = smart_object_get_string(p->config, qlkey("supervisor_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *host = smart_object_get_string(p->config, qlkey("supervisor_host"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        int port = smart_object_get_int(p->config, qlkey("supervisor_port"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct smart_object *data = smart_object_alloc();
        smart_object_set_string(data, qskey(&__key_version__), qskey(version_code));
        smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_location_search_nearby__));
        smart_object_set_string(data, qskey(&__key_pass__),qskey(pass));

        struct smart_object *latlng = smart_object_get_object(data, qskey(&__key_latlng__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        smart_object_set_double(latlng, qskey(&__key_lat__), param.lat);
        smart_object_set_double(latlng, qskey(&__key_lon__), param.lon);

        cs_request_alloc_with_param(p->requester, data,
                (cs_request_callback)checking_client_requester_callback, p,
                (struct cs_request_param){
                        .host = host->ptr,
                        .port = port
                }
        );
}
