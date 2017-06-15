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

#include <cherry/memory.h>
#include <cherry/string.h>
#include <cherry/server/file_descriptor.h>
#include <cherry/stdio.h>
#include <cherry/array.h>
#include <cherry/map.h>
#include <cherry/list.h>
#include <cherry/bytes.h>
#include <cherry/math/math.h>
#include <cherry/server/socket.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <sys/time.h>

#include <cherry/unistd.h>
#include <smartfox/data.h>

#include <common/command.h>
#include <common/key.h>
#include <common/request.h>

#include <supervisor/handler.h>

static struct client_buffer *__client_buffer_alloc()
{
        struct client_buffer *p = smalloc(sizeof(struct client_buffer));
        p->buff                 = string_alloc(0);
        p->requested_len        = 0;
        return p;
}

static void __client_buffer_free(struct client_buffer *p)
{
        string_free(p->buff);
        sfree(p);
}

static void *get_in_addr(struct sockaddr *sa)
{
        if(sa->sa_family == AF_INET) {
                return &(((struct sockaddr_in *)sa)->sin_addr);
        }
        return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

static int __supervisor_check_path(struct supervisor *ws, char *path, u32 len)
{
        char c;
        int i;
        int depth = 0;
        int check_dot = 0;
        for_i(i, len) {
                c = path[i];
                switch (c) {
                        case '/':
                                if(check_dot == 2) {
                                        depth--;
                                } else {
                                        if(i + 1 < len && path[i + 1] != '/')
                                                depth++;
                                }
                                check_dot = 0;
                                break;
                        case '.':
                                check_dot++;
                                break;
                        default:
                                check_dot = 0;
                                break;
                }
                if(depth < 0) break;
        }
        return depth >= 0;
}

static void __supervisor_close_client(struct supervisor *p, int fd)
{
        debug("force client closed\n");

        pthread_mutex_lock(&p->client_data_mutex);

        debug("force client closed begin\n");

        /*
         * increase mask to prohibit current pending packets sending to this fd
         */
        array_reserve(p->fd_mask, fd + 1);
        u32 current_mask = array_get(p->fd_mask, u32, fd);
        current_mask++;
        array_set(p->fd_mask, fd, &current_mask);

        struct client_buffer *cb = map_get(p->clients_datas, struct client_buffer *, qpkey(fd));
        if(cb) {
                __client_buffer_free(cb);
                map_remove_key(p->clients_datas, &fd, sizeof(fd));
        }
        shutdown(fd, SHUT_RDWR);
        socket_close(fd);
        file_descriptor_set_remove(p->master, fd);
        pthread_mutex_unlock(&p->client_data_mutex);

        debug("force close connection\n\n");
}

static void __supervisor_check_old_and_close_client(struct supervisor *ws, int fd)
{
        int can_close = 1;
        /*
         * make sure fd will be closed once
         */
        pthread_mutex_lock(&ws->client_data_mutex);
        int i;
        for_i(i, ws->fd_invalids->len) {
                struct client_step cfd = array_get(ws->fd_invalids, struct client_step, i);
                if(cfd.fd == fd) {
                        // if(cfd.step < ws->step) {
                                array_remove(ws->fd_invalids, i);
                        // } else {
                        //         can_close = 0;
                        // }

                        break;
                }
        }
        pthread_mutex_unlock(&ws->client_data_mutex);

        if(can_close) __supervisor_close_client(ws, fd);
}

static void __supervisor_push_close(struct supervisor *p, int fd)
{
        pthread_mutex_lock(&p->client_data_mutex);

        array_reserve(p->fd_mask, fd + 1);
        u32 current_mask = array_get(p->fd_mask, u32, fd);
        current_mask++;
        array_set(p->fd_mask, fd, &current_mask);

        struct client_step step;
        step.fd = fd;
        step.step = p->step;

        array_push(p->fd_invalids, &step);
        pthread_mutex_unlock(&p->client_data_mutex);
}

static void __supervisor_send_to_client(struct supervisor *p, int fd, u32 mask, char *ptr, int len, u8 keep)
{
        pthread_mutex_lock(&p->client_data_mutex);
        array_reserve(p->fd_mask, fd + 1);
        u32 current_mask = array_get(p->fd_mask, u32, fd);
        pthread_mutex_unlock(&p->client_data_mutex);

        if(current_mask != mask) {
                /*
                 * prohibit sending to fd
                 */
                debug("prohibit\n");
                return;
        }

        debug("send client %d : %s\n", fd, ptr);
        int bytes_send          = 0;
        int slen                = len;
        /*
         * send packet len
         */
        pthread_mutex_lock(&p->client_data_mutex);
        u32 num                 = htonl((u32)len);
        send(fd, &num, sizeof(num), 0);

        /*
         * send packet content
         */
        while(bytes_send < slen) {
                bytes_send += send(fd, ptr, len, 0);
                if(bytes_send < 0) {
                        break;
                }
                len -= bytes_send;
                ptr += bytes_send;
        }
        pthread_mutex_unlock(&p->client_data_mutex);

        if(!keep) {
                /*
                 * push fd to invalids array
                 * if we close fd after calling select then
                 * it will block infinitely
                 */
                __supervisor_push_close(p, fd);
        }
}

void supervisor_send_to_client(struct supervisor *p, int fd, u32 mask, char *ptr, int len, u8 keep)
{
        __supervisor_send_to_client(p, fd, mask, ptr, len, keep);
}

static void __supervisor_handle_msg(struct supervisor *ws, u32 fd, char* msg, size_t msg_len)
{
        debug("handle message\n");
        pthread_mutex_lock(&ws->client_data_mutex);
        if( ! map_has_key(ws->clients_datas, qpkey(fd))) {
                struct client_buffer *cb        = __client_buffer_alloc();
                map_set(ws->clients_datas, qpkey(fd), &cb);
        }
        struct client_buffer *cb = map_get(ws->clients_datas, struct client_buffer *, qpkey(fd));
        pthread_mutex_unlock(&ws->client_data_mutex);

        int amount;
check:;
        if(msg_len == 0) goto end;

        if(cb->requested_len == 0) {
                if(msg_len < sizeof(u32)) {
                        amount = msg_len;
                        string_cat(cb->buff, msg, amount);
                        goto end;
                } else {
                        if(cb->buff->len) {
                                amount = sizeof(u32) - cb->buff->len;
                                string_cat(cb->buff, msg, amount);
                                u32 *num                = (u32 *)cb->buff->ptr;
                                cb->requested_len       = ntohl(*num);
                                msg += amount;
                                msg_len -= amount;
                                cb->buff->len           = 0;
                        } else {
                                u32 *num                = (u32 *)msg;
                                cb->requested_len       = ntohl(*num);
                                msg                     += sizeof(u32);
                                msg_len                 -= sizeof(u32);
                        }
                }
        }

        if(msg_len + cb->buff->len >= cb->requested_len) {
                goto receive_full_packet;
        } else {
                string_cat(cb->buff, msg, msg_len);
                goto end;
        }


receive_full_packet:;
        amount = cb->requested_len - cb->buff->len;
        string_cat(cb->buff, msg, amount);
        msg += amount;
        msg_len -= amount;

send_client:;
        int counter = 0;
        debug("receive: %s\n", cb->buff->ptr);
        struct smart_object *obj = smart_object_from_json(cb->buff->ptr, cb->buff->len, &counter);
        struct string *cmd = smart_object_get_string(obj, qskey(&__key_cmd__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        supervisor_delegate *delegate = map_get_pointer(ws->delegates, qskey(cmd));
        cb->requested_len       = 0;
        cb->buff->len           = 0;

        if(*delegate) {
                pthread_mutex_lock(&ws->client_data_mutex);
                array_reserve(ws->fd_mask, fd + 1);
                u32 mask = array_get(ws->fd_mask, u32, fd);
                pthread_mutex_unlock(&ws->client_data_mutex);

                (*delegate)(ws, fd, mask, obj);
        } else {
                if(strcmp(cb->buff->ptr, "0") == 0) {
                        __supervisor_check_old_and_close_client(ws, fd);
                } else {
                        __supervisor_push_close(ws, fd);
                }
        }
        smart_object_free(obj);
end:;
}

void supervisor_start(struct supervisor *ws)
{
        char *root      = smart_object_get_string(ws->config, qlkey("service_root"), SMART_GET_REPLACE_IF_WRONG_TYPE)->ptr;
        u16 port        = smart_object_get_short(ws->config, qlkey("service_port"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        ws->root->len = 0;
        string_cat(ws->root, root, strlen(root));

        struct sockaddr_storage remoteaddr;
        socklen_t addrlen;
        i32 newfd;

        char remoteIP[INET6_ADDRSTRLEN];
        int yes                 = 1;
        int rv;
        struct addrinfo hints, *ai, *p, *q;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family         = AF_UNSPEC;
        hints.ai_socktype       = SOCK_STREAM;
        hints.ai_protocol       = IPPROTO_TCP;
        hints.ai_flags          = AI_PASSIVE;

        struct string *ports    = string_alloc(0);
        string_cat_int(ports, port);

        if((rv = getaddrinfo(NULL, ports->ptr, &hints, &ai)) != 0) {
                debug("server error: %s\n", gai_strerror(rv));
                goto finish;
        }
        q = p;
        for(p = ai; p != NULL; p = p->ai_next) {
                ws->listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
                if(ws->listener < 0) {
                        continue;
                }
                setsockopt(ws->listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

                if(bind(ws->listener, p->ai_addr, p->ai_addrlen) < 0) {
                        socket_close(ws->listener);
                        continue;
                } else {
                        q = p;
                        continue;
                }
                break;
        }
        p = q;

        if(p == NULL) {
                debug("server failed to bind\n");

                goto finish;
        }
        freeaddrinfo(ai);

        if(listen(ws->listener, 10) == -1) {
                perror("listener");
                goto finish;
        }

        debug("Server started at port no. %d with root directory as %s\n",port, root);

        file_descriptor_set_add(ws->master, ws->listener);
        ws->fdmax               = ws->listener;
        struct array *actives   = array_alloc(sizeof(u32), ORDERED);

#define MAX_RECV_BUF_LEN 99999
        char *recvbuf           = smalloc(sizeof(char) * MAX_RECV_BUF_LEN);
        int nbytes              = 0;
        struct timeval t1;
        double elapsedTime;
        gettimeofday(&t1, NULL);
        struct list_head *head;

        ws->step = 0;

        while(1) {
                /*
                 * listen for next incomming sockets
                 */
                pthread_mutex_lock(&ws->client_data_mutex);
                file_descriptor_set_assign(ws->incomming, ws->master);
                ws->step++;
                pthread_mutex_unlock(&ws->client_data_mutex);

                debug("start select\n");
                if(select(ws->fdmax + 1, ws->incomming->set->ptr, NULL, NULL, NULL) == -1) {
                        perror("select");
                        break;
                }
                debug("process select\n");

                /*
                 * process handlers
                 */
                gettimeofday(&t1, NULL);
                list_for_each_secure(head, &ws->handlers, {
                        struct supervisor_handler *handler = (struct supervisor_handler *)
                                ((char *)head - offsetof(struct supervisor_handler, head));
                        elapsedTime = (t1.tv_sec - handler->last_executed_time.tv_sec);
                        elapsedTime += (t1.tv_usec - handler->last_executed_time.tv_usec) / 1000000.0;
                        if(elapsedTime >= handler->time_rate) {
                                if(handler->delegate) {
                                        handler->delegate(ws);
                                }
                                handler->last_executed_time = t1;
                        }
                });
                /*
                 * get active sockets
                 */
                array_force_len(actives, 0);
                file_descriptor_set_get_active(ws->incomming, actives);

                u32 *fd;
                array_for_each(fd, actives) {
                        if(*fd == ws->listener) {
                                /*
                                 * new connection
                                 */
                                addrlen = sizeof(remoteaddr);
                                newfd   = accept(ws->listener, (struct sockaddr *) &remoteaddr, &addrlen);
                                if(newfd == -1) {
                                        perror("accept\n");
                                } else {
                                        pthread_mutex_lock(&ws->client_data_mutex);

                                        file_descriptor_set_add(ws->master, newfd);
                                        ws->fdmax = MAX(ws->fdmax, newfd);
                                        debug("new connection from %s on socket %d\n",
                                                inet_ntop(remoteaddr.ss_family,
                                                        get_in_addr((struct sockaddr *)&remoteaddr),
                                                        remoteIP, INET6_ADDRSTRLEN), newfd);
                                        array_reserve(ws->fd_mask, newfd + 1);
                                        u32 mask = array_get(ws->fd_mask, u32, newfd);
                                        mask++;
                                        array_set(ws->fd_mask, newfd, &mask);

                                        pthread_mutex_unlock(&ws->client_data_mutex);
                                }
                        } else {
                                /*
                                 * receive client request
                                 */
                                nbytes = recv(*fd, recvbuf, MAX_RECV_BUF_LEN, 0);
                                if(nbytes <= 0) {
                                        __supervisor_check_old_and_close_client(ws, *fd);
                                } else {
                                        __supervisor_handle_msg(ws, *fd, recvbuf, nbytes);
                                }
                        }
                }

                /*
                 * close old sockets
                 */
        check_old_fd:;
                pthread_mutex_lock(&ws->client_data_mutex);
                int i;
                for_i(i, ws->fd_invalids->len) {
                        struct client_step cfd = array_get(ws->fd_invalids, struct client_step, i);
                        // if(cfd.step < ws->step) {
                                array_remove(ws->fd_invalids, i);
                                pthread_mutex_unlock(&ws->client_data_mutex);
                                __supervisor_close_client(ws, cfd.fd);
                                pthread_mutex_lock(&ws->client_data_mutex);
                                i--;
                        // }
                }
                pthread_mutex_unlock(&ws->client_data_mutex);
        }
finish:;
        debug("shutdown\n");
        shutdown(ws->listener, SHUT_RDWR);
        socket_close(ws->listener);
        string_free(ports);
        sfree(recvbuf);
        array_free(actives);

#undef MAX_RECV_BUF_LEN
}

static void __load_base_map_callback(void *p, struct smart_object *data)
{
        struct string *j = smart_object_to_json(data);
        debug("load base map : %s\n",j->ptr);
        string_free(j);
}


static void __load_base_map(struct supervisor *p, char *file)
{
        struct string *es_version_code = smart_object_get_string(p->config, qlkey("es_version_code"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *es_pass = smart_object_get_string(p->config, qlkey("es_pass"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        struct smart_object *request = cs_request_data_from_file(file, FILE_INNER, qskey(es_version_code), qskey(es_pass));
        cs_request_alloc(p->es_server_requester, request, __load_base_map_callback, p);
}

static void __load_es_server(struct supervisor *p)
{
        p->es_server_requester  = cs_requester_alloc();

        struct string *host     = smart_object_get_string(p->config, qlkey("es_host"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        u16 port = smart_object_get_short(p->config, qlkey("es_port"), SMART_GET_REPLACE_IF_WRONG_TYPE);

        cs_requester_connect(p->es_server_requester, host->ptr, port);

        __load_base_map(p, "res/supervisor/index.json");
        __load_base_map(p, "res/supervisor/admin/map.json");
        __load_base_map(p, "res/supervisor/location/map.json");
        __load_base_map(p, "res/supervisor/service/map.json");
}

static void __register_handler(struct supervisor *p, supervisor_handler_delegate delegate, double time_rate)
{
        struct supervisor_handler *handler = supervisor_handler_alloc(delegate, time_rate);
        list_add_tail(&handler->head, &p->handlers);
}

struct supervisor *supervisor_alloc()
{
        struct supervisor *p    = smalloc(sizeof(struct supervisor));
        p->master               = file_descriptor_set_alloc();
        p->incomming            = file_descriptor_set_alloc();
        p->fdmax                = 0;
        p->listener             = 0;
        p->root                 = string_alloc(0);

        INIT_LIST_HEAD(&p->handlers);

        p->fd_mask              = array_alloc(sizeof(u32), ORDERED);
        p->fd_invalids          = array_alloc(sizeof(struct client_step), ORDERED);

        p->clients_datas        = map_alloc(sizeof(struct client_buffer *));

        pthread_mutex_init(&p->client_data_mutex, NULL);

        /*
         * load config
         */
        p->config               = smart_object_from_json_file("res/config.json", FILE_INNER);

        __load_es_server(p);

        /*
         * setup delegates
         */
        p->delegates            = map_alloc(sizeof(supervisor_delegate));

        map_set(p->delegates, qskey(&__cmd_get_service__), &(supervisor_delegate){supervisor_process_get_service});
        map_set(p->delegates, qskey(&__cmd_service_register__), &(supervisor_delegate){supervisor_process_service_register});
        map_set(p->delegates, qskey(&__cmd_service_get_by_username__), &(supervisor_delegate){supervisor_process_service_get_by_username});
        map_set(p->delegates, qskey(&__cmd_service_validate__), &(supervisor_delegate){supervisor_process_service_validate});

        map_set(p->delegates, qskey(&__cmd_location_register__), &(supervisor_delegate){supervisor_process_location_register});
        map_set(p->delegates, qskey(&__cmd_location_update_latlng__), &(supervisor_delegate){supervisor_process_location_update_latlng});
        map_set(p->delegates, qskey(&__cmd_location_update_ip_port__), &(supervisor_delegate){supervisor_process_location_update_ip_port});
        map_set(p->delegates, qskey(&__cmd_location_search_nearby__), &(supervisor_delegate){supervisor_process_location_search_nearby});

        __register_handler(p,
                supervisor_process_clear_invalidated_service,
                smart_object_get_double(p->config, qlkey("service_created_timeout"), SMART_GET_REPLACE_IF_WRONG_TYPE));
        return p;
}

void supervisor_free(struct supervisor *p)
{
        cs_requester_free(p->es_server_requester);
        string_free(p->root);
        file_descriptor_set_free(p->master);
        file_descriptor_set_free(p->incomming);

        array_free(p->fd_mask);
        array_free(p->fd_invalids);

        smart_object_free(p->config);

        map_free(p->delegates);

        struct list_head *head;
        list_while_not_singular(head, &p->handlers) {
                struct supervisor_handler *handler = (struct supervisor_handler *)
                        ((char *)head - offsetof(struct supervisor_handler, head));
                supervisor_handler_free(handler);
        }

        pthread_mutex_lock(&p->client_data_mutex);
        map_deep_free(p->clients_datas, struct client_buffer *, __client_buffer_free);
        pthread_mutex_unlock(&p->client_data_mutex);
        pthread_mutex_destroy(&p->client_data_mutex);

        sfree(p);
}
