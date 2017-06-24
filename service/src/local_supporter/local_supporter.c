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
#include <service/local_supporter.h>

#include <cherry/memory.h>
#include <cherry/list.h>
#include <cherry/stdio.h>
#include <cherry/math/math.h>
#include <cherry/string.h>
#include <cherry/array.h>
#include <cherry/map.h>

#include <smartfox/data.h>

#include <common/cs_server.h>
#include <common/key.h>
#include <common/command.h>
#include <common/util.h>

#include <pthread.h>

struct local_supporter *local_supporter_alloc()
{
        struct local_supporter *p       = smalloc(sizeof(struct local_supporter));
        INIT_LIST_HEAD(&p->server);

        struct cs_server *c             = cs_server_alloc(CS_SERVER_PUBLIC);
        list_add_tail(&c->user_head, &p->server);

        c->config                       = smart_object_from_json_file("res/config.json", FILE_INNER);

        map_set(c->delegates, qskey(&__cmd_local_supporter_get_code__),
                &(cs_server_delegate){local_supporter_process_get_code});

        return p;
}

void local_supporter_start(struct local_supporter *p)
{
        if(!list_singular(&p->server)) {
                struct cs_server *c     = (struct cs_server *)
                        ((char *)p->server.next - offsetof(struct cs_server, user_head));

        begin:;
                /*
                 * local supporter resets after each 30 minutes
                 */
                c->lifetime             = 1800;
                /*
                 * local supporter counts after each 10 seconds
                 */
                c->timeout              = 10;

                /*
                 * reset local supporter port and code
                 */
                pthread_mutex_lock(&__shared_ram_config_mutex__);
                int port                = rand_ri(10001, 60000);
                struct string *local_ip = common_get_local_ip_adress();

                smart_object_set_int(__shared_ram_config__, qskey(&__key_local_port__), port);
                smart_object_set_string(__shared_ram_config__, qskey(&__key_local_ip__), qskey(local_ip));

                char buf[9];
                common_gen_random(buf, sizeof(buf) / sizeof(buf[0]));
                smart_object_set_string(__shared_ram_config__, qskey(&__key_local_code__), qlkey(buf));

                string_free(local_ip);
                pthread_mutex_unlock(&__shared_ram_config_mutex__);

                /*
                 * start local supporter
                 */
                cs_server_start(c, port);

                goto begin;
        }
}

void local_supporter_free(struct local_supporter *p)
{
        if(!list_singular(&p->server)) {
                struct cs_server *c     = (struct cs_server *)
                        ((char *)p->server.next - offsetof(struct cs_server, user_head));
                cs_server_free(c);
        }

        sfree(p);
}
