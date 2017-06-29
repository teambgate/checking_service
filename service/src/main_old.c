// /*
//  * Copyright (C) 2017 Manh Tran
//  *
//  * This program is free software; you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License as published by
//  * the Free Software Foundation; either version 2 of the License, or
//  * (at your option) any later version.
//  *
//  * This program is distributed in the hope that it will be useful,
//  * but WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  * GNU General Public License for more details.
//  */
// #include <common/request.h>
// #include <cherry/unistd.h>
// #include <smartfox/data.h>
// #include <cherry/map.h>
// #include <cherry/array.h>
// #include <common/key.h>
// #include <common/command.h>
// #include <cherry/memory.h>
// #include <cherry/string.h>
// #include <cherry/stdio.h>
// #include <locale.h>
// #include <time.h>
// #include <cherry/crypt/md5.h>
// #include  <sys/types.h>
//
//
// static void callback(void *p, struct sobj *data)
// {
//         struct string *j = sobj_to_json(data);
//         debug("receive : %s\n",j->ptr);
//         string_free(j);
// }
//
// int main(int argc, char **argv)
// {
//         int i;
// start:;
//         struct cs_requester *p  = cs_requester_alloc();
//         cs_requester_connect(p, "127.0.0.1", 9999);
//
//         for(int i = 0; i < 10000; i++) {
//                 struct sobj *data = sobj_alloc();
//                 sobj_set_str(data, qskey(&__key_version__), qlkey("1"));
//                 sobj_set_str(data, qskey(&__key_cmd__), qskey(&__cmd_location_search_nearby__));
//                 sobj_set_str(data, qskey(&__key_pass__), qlkey("123456"));
//                 sobj_set_str(data, qskey(&__key_name__), qlkey("BGATE CORP"));
//                 sobj_set_str(data, qskey(&__key_user_name__), qlkey("bui_thi_xuan"));
//                 sobj_set_str(data, qskey(&__key_user_pass__), qlkey("12345678"));
//                 sobj_set_str(data, qskey(&__key_device_id__), qlkey("Manh Ubuntu"));
//                 sobj_set_str(data, qskey(&__key_validate_code__), qlkey("EKir8cdX"));
//                 // sobj_set_i32(data, qskey(&__key_id__), 1);
//                 sobj_set_str(data, qskey(&__key_ip__), qlkey("192.168.1.218"));
//                 sobj_set_i64(data, qskey(&__key_port__), 50000);
//                 sobj_set_str(data, qskey(&__key_location_name__), qlkey("Sang Tao"));
//                 struct sobj *latlng = sobj_get_obj(data, qskey(&__key_latlng__), RPL_TYPE);
//                 sobj_set_f64(latlng, qskey(&__key_lat__), 23);
//                 sobj_set_f64(latlng, qskey(&__key_lon__), 99.122);
//
//                 cs_request_alloc(p, data, callback, p);
//         }
//
//
//         // struct cs_requester *p  = cs_requester_alloc();
//         // cs_requester_connect(p, "localhost", 9898);
//         // for(int i = 0; i < 20000; i++)
//         // {
//         //         struct sobj *obj = sobj_from_json_file("res/request.json", FILE_INNER);
//         //         struct string *request = sobj_get_str(obj, qlkey("request"), RPL_TYPE);
//         //         struct string *path = sobj_get_str(obj, qlkey("path"), RPL_TYPE);
//         //         struct sobj *objdata = sobj_get_obj(obj, qlkey("data"), RPL_TYPE);
//         //
//         //         struct sobj *data = sobj_alloc();
//         //         sobj_set_str(data, qskey(&__key_version__), qlkey("1"));
//         //
//         //         if(strcmp(request->ptr, "post") == 0) {
//         //                 sobj_set_str(data, qskey(&__key_cmd__), qskey(&__cmd_post__));
//         //         } else if(strcmp(request->ptr, "get") == 0) {
//         //                 sobj_set_str(data, qskey(&__key_cmd__), qskey(&__cmd_get__));
//         //         } else if(strcmp(request->ptr, "put") == 0) {
//         //                 sobj_set_str(data, qskey(&__key_cmd__), qskey(&__cmd_put__));
//         //         } else if(strcmp(request->ptr, "delete") == 0) {
//         //                 sobj_set_str(data, qskey(&__key_cmd__), qskey(&__cmd_delete__));
//         //         }
//         //
//         //
//         //         sobj_set_str(data, qskey(&__key_pass__), qlkey("123456"));
//         //
//         //         sobj_set_str(data, qskey(&__key_path__), qskey(path));
//         //
//         //         struct string *json = sobj_to_json(objdata);
//         //         int counter = 0;
//         //         struct sobj *d = sobj_from_json(json->ptr, json->len, &counter);
//         //         string_free(json);
//         //         // struct sobj *obj = sobj_alloc();
//         //         // sobj_set_str(obj, qskey(&__key_name__), qlkey("Johan"));
//         //         sobj_set_obj(data, qskey(&__key_data__), d);
//         //         cs_request_alloc(p, data, callback, p);
//         //         sobj_free(obj);
//         //         sleep(1);
//         // }
//
//         sleep(6000);
//         debug("free requester\n");
//         cs_requester_free(p);
//         sleep(1);
//         cache_free();
//         dim_memory();
//         // count++;
//         // if(count < 10000) goto start;
//         return 1;
// }
