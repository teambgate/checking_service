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

 static void __delete_user_name_callback(struct supervisor *p, struct sfs_object *result)
 {
         struct string *res = sfs_object_to_json(result);
         debug("delete user name : %s\n", res->ptr);
         string_free(res);
 }

 void supervisor_process_clear_invalidated_service(struct supervisor *p)
 {
         double offset = sfs_object_get_double(p->config, qlkey("service_created_timeout"), SFS_GET_REPLACE_IF_WRONG_TYPE);
         debug("offset %f\n", offset);
         struct string *last_time = offset_time_to_string(-offset);

         struct string *es_version_code = sfs_object_get_string(p->config, qlkey("es_version_code"), SFS_GET_REPLACE_IF_WRONG_TYPE);
         struct string *es_pass = sfs_object_get_string(p->config, qlkey("es_pass"), SFS_GET_REPLACE_IF_WRONG_TYPE);

         struct sfs_object *request_data = cs_request_data_from_file(
                 "res/supervisor/service/delete_by_date.json", FILE_INNER,
                 qskey(es_version_code), qskey(es_pass));

        struct sfs_object *request_data_data = sfs_object_get_object(request_data, qskey(&__key_data__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct sfs_object *query = sfs_object_get_object(request_data_data, qlkey("query"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct sfs_object *_bool = sfs_object_get_object(query, qlkey("bool"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct sfs_array *must = sfs_object_get_array(_bool, qlkey("must"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct sfs_object *must_1 = sfs_array_get_object(must, 1, SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct sfs_object *range = sfs_object_get_object(must_1, qlkey("range"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct sfs_object *reserved = sfs_object_get_object(range, qlkey("reserved"), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct string *to = sfs_object_get_string(reserved, qlkey("to"), SFS_GET_REPLACE_IF_WRONG_TYPE);

        to->len = 0;
        string_cat_string(to, last_time);

        cs_request_alloc(p->es_server_requester, request_data, (cs_request_callback)__delete_user_name_callback, p);
        string_free(last_time);
 }
