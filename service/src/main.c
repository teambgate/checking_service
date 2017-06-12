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
#include <common/request.h>
#include <cherry/unistd.h>
#include <smartfox/data.h>
#include <cherry/map.h>
#include <cherry/array.h>
#include <common/key.h>
#include <common/command.h>
#include <cherry/memory.h>
#include <cherry/string.h>
#include <cherry/stdio.h>
#include <locale.h>
#include <time.h>

static void callback(void *p, struct sfs_object *data)
{
        struct string *j = sfs_object_to_json(data);
        debug("receive : %s\n",j->ptr);
        string_free(j);
}

int main(int argc, char **argv)
{
        setlocale(LC_NUMERIC, "C");
        int count = 0;
start:;
        struct cs_requester *p  = cs_requester_alloc();
        cs_requester_connect(p, "localhost", 9898);
        for(int i = 0; i < 20000; i++)
        {
                // struct sfs_object *data = sfs_object_alloc();
                // sfs_object_set_string(data, qskey(&__key_version__), qlkey("1"));
                // sfs_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_get_service__));
                // sfs_object_set_int(data, qskey(&__key_id__), 1);
                // sfs_object_set_string(data, qskey(&__key_pass__), qlkey("123456"));
                // sfs_object_set_string(data, qskey(&__key_ip__), qlkey("192.168.1.248"));
                // sfs_object_set_long(data, qskey(&__key_port__), 1000);
                // sfs_object_set_double(data, qskey(&__key_lat__), 1.1);
                // sfs_object_set_double(data, qskey(&__key_lng__), 9999.1);
                // cs_request_alloc(p, data, callback, p);

                struct sfs_object *obj = sfs_object_from_json_file("res/request.json", FILE_INNER);
                struct string *request = sfs_object_get_string(obj, qlkey("request"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                struct string *path = sfs_object_get_string(obj, qlkey("path"), SFS_GET_REPLACE_IF_WRONG_TYPE);
                struct sfs_object *objdata = sfs_object_get_object(obj, qlkey("data"), SFS_GET_REPLACE_IF_WRONG_TYPE);

                struct sfs_object *data = sfs_object_alloc();
                sfs_object_set_string(data, qskey(&__key_version__), qlkey("1"));

                if(strcmp(request->ptr, "post") == 0) {
                        sfs_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_post__));
                } else if(strcmp(request->ptr, "get") == 0) {
                        sfs_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_get__));
                } else if(strcmp(request->ptr, "put") == 0) {
                        sfs_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_put__));
                } else if(strcmp(request->ptr, "delete") == 0) {
                        sfs_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_delete__));
                }


                sfs_object_set_string(data, qskey(&__key_pass__), qlkey("123456"));

                sfs_object_set_string(data, qskey(&__key_path__), qskey(path));

                struct string *json = sfs_object_to_json(objdata);
                int counter = 0;
                struct sfs_object *d = sfs_object_from_json(json->ptr, json->len, &counter);
                string_free(json);
                // struct sfs_object *obj = sfs_object_alloc();
                // sfs_object_set_string(obj, qskey(&__key_name__), qlkey("Johan"));
                sfs_object_set_object(data, qskey(&__key_data__), d);
                cs_request_alloc(p, data, callback, p);
                sfs_object_free(obj);
                sleep(1);
        }
        sleep(120);
        debug("free requester\n");
        cs_requester_free(p);
        sleep(3);
        cache_free();
        dim_memory();
        // count++;
        // if(count < 10000) goto start;
        return 1;
}
