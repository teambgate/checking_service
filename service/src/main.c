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
#include <cherry/crypt/md5.h>

static void callback(void *p, struct smart_object *data)
{
        struct string *j = smart_object_to_json(data);
        debug("receive : %s\n",j->ptr);
        string_free(j);
}

int main(int argc, char **argv)
{
        struct string *test_md5 = md5_string(qlkey(""));
        debug("md5 : %s\n", test_md5->ptr);
        string_free(test_md5);

        setlocale(LC_NUMERIC, "C");
        int count = 0;
start:;
        struct cs_requester *p  = cs_requester_alloc();
        cs_requester_connect(p, "127.0.0.1", 9999);
        struct smart_object *data = smart_object_alloc();
        smart_object_set_string(data, qskey(&__key_version__), qlkey("1"));
        smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_register_location__));
        smart_object_set_string(data, qskey(&__key_pass__), qlkey("123456"));
        smart_object_set_string(data, qskey(&__key_name__), qlkey("BGATE CORP"));
        smart_object_set_string(data, qskey(&__key_user_name__), qlkey("sang_tao"));
        smart_object_set_string(data, qskey(&__key_user_pass__), qlkey("12345678"));
        smart_object_set_string(data, qskey(&__key_device_id__), qlkey("Manh Ubuntu"));
        smart_object_set_string(data, qskey(&__key_validate_code__), qlkey("ZCavT1Pk"));
        // smart_object_set_int(data, qskey(&__key_id__), 1);
        smart_object_set_string(data, qskey(&__key_ip__), qlkey("192.168.1.248"));
        smart_object_set_long(data, qskey(&__key_port__), 1000);
        smart_object_set_string(data, qskey(&__key_location_name__), qlkey("Sang Tao"));
        struct smart_object *latlng = smart_object_get_object(data, qskey(&__key_latlng__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        smart_object_set_double(latlng, qskey(&__key_lat__), 1.1);
        smart_object_set_double(latlng, qskey(&__key_lon__), 99.1);

        cs_request_alloc(p, data, callback, p);

        // struct cs_requester *p  = cs_requester_alloc();
        // cs_requester_connect(p, "localhost", 9898);
        // for(int i = 0; i < 20000; i++)
        // {
        //         struct smart_object *obj = smart_object_from_json_file("res/request.json", FILE_INNER);
        //         struct string *request = smart_object_get_string(obj, qlkey("request"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        //         struct string *path = smart_object_get_string(obj, qlkey("path"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        //         struct smart_object *objdata = smart_object_get_object(obj, qlkey("data"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        //
        //         struct smart_object *data = smart_object_alloc();
        //         smart_object_set_string(data, qskey(&__key_version__), qlkey("1"));
        //
        //         if(strcmp(request->ptr, "post") == 0) {
        //                 smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_post__));
        //         } else if(strcmp(request->ptr, "get") == 0) {
        //                 smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_get__));
        //         } else if(strcmp(request->ptr, "put") == 0) {
        //                 smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_put__));
        //         } else if(strcmp(request->ptr, "delete") == 0) {
        //                 smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_delete__));
        //         }
        //
        //
        //         smart_object_set_string(data, qskey(&__key_pass__), qlkey("123456"));
        //
        //         smart_object_set_string(data, qskey(&__key_path__), qskey(path));
        //
        //         struct string *json = smart_object_to_json(objdata);
        //         int counter = 0;
        //         struct smart_object *d = smart_object_from_json(json->ptr, json->len, &counter);
        //         string_free(json);
        //         // struct smart_object *obj = smart_object_alloc();
        //         // smart_object_set_string(obj, qskey(&__key_name__), qlkey("Johan"));
        //         smart_object_set_object(data, qskey(&__key_data__), d);
        //         cs_request_alloc(p, data, callback, p);
        //         smart_object_free(obj);
        //         sleep(1);
        // }

        sleep(1);
        debug("free requester\n");
        cs_requester_free(p);
        sleep(1);
        cache_free();
        dim_memory();
        // count++;
        // if(count < 10000) goto start;
        return 1;
}
