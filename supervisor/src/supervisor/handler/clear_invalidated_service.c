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
 #include <supervisor/supervisor.h>
 #include <smartfox/data.h>
 #include <cherry/string.h>
 #include <cherry/array.h>
 #include <cherry/map.h>
 #include <common/request.h>
 #include <common/command.h>
 #include <common/key.h>
 #include <cherry/time.h>
 #include <cherry/stdio.h>
 #include <common/cs_server.h>

 static void __delete_user_name_callback(struct cs_server *p, struct smart_object *result)
 {
         struct string *res = smart_object_to_json(result);
         debug("delete user name : %s\n", res->ptr);
         string_free(res);
 }

 void supervisor_process_clear_invalidated_service(struct cs_server *p)
 {
         struct supervisor *supervisor = (struct supervisor *)
                 ((char *)p->user_head.next - offsetof(struct supervisor , server));

         double offset = smart_object_get_double(p->config, qlkey("service_created_timeout"), SMART_GET_REPLACE_IF_WRONG_TYPE);
         struct string *last_time = offset_time_to_string(-offset);

         struct string *es_version_code = smart_object_get_string(p->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
         struct string *es_pass = smart_object_get_string(p->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

         struct string *content = cs_request_string_from_file("res/supervisor/service/delete/delete_by_date.json", FILE_INNER);
         string_replace(content, "{LAST_TIME}", last_time->ptr);

         struct smart_object *request_data = cs_request_data_from_string(
                 qskey(content),
                 qskey(es_version_code), qskey(es_pass));
        string_free(content);

        cs_request_alloc(supervisor->es_server_requester, request_data, (cs_request_callback)__delete_user_name_callback, p);
        string_free(last_time);
 }
