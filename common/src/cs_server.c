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
#include <common/cs_server.h>
#include <common/request.h>
#include <common/command.h>
#include <common/key.h>
#include <common/util.h>

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

/*
 * client buffer definition
 */
static struct cs_client_buffer *__client_buffer_alloc()
{
        struct cs_client_buffer *p      = smalloc(sizeof(struct cs_client_buffer));
        p->buff                         = string_alloc(0);
        p->requested_len                = 0;
        return p;
}

static void __client_buffer_free(struct cs_client_buffer *p)
{
        string_free(p->buff);
        sfree(p);
}

/*
 * handler definition
 */
struct cs_server_handler *cs_server_handler_alloc(cs_server_handler_delegate delegate, double time_rate)
{
        struct cs_server_handler *p    = smalloc(sizeof(struct cs_server_handler));
        INIT_LIST_HEAD(&p->head);
        p->delegate                     = delegate;
        p->time_rate                    = time_rate;
        gettimeofday(&p->last_executed_time, NULL);

        return p;
}

void cs_server_handler_free(struct cs_server_handler *p)
{
        list_del(&p->head);
        sfree(p);
}

/*
 * callback data
 */
struct cs_server_callback_user_data *cs_server_callback_user_data_alloc(struct cs_server *p, int fd, u32 mask, struct smart_object *obj)
{
        struct cs_server_callback_user_data *cud = smalloc(sizeof(struct cs_server_callback_user_data));
        cud->p = p;
        cud->fd = fd;
        cud->mask = mask;
        cud->obj = smart_object_clone(obj);
        return cud;
}

void cs_server_callback_user_data_free(struct cs_server_callback_user_data *p)
{
        smart_object_free(p->obj);
        sfree(p);
}

/*
 *cs server definition
 */

static void *get_in_addr(struct sockaddr *sa)
{
        if(sa->sa_family == AF_INET) {
                return &(((struct sockaddr_in *)sa)->sin_addr);
        }
        return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/*
 * close client
 */
static void __cs_server_close_client(struct cs_server *p, int fd)
{
        debug("cs_server: force client closed\n");
        pthread_mutex_lock(&p->client_data_mutex);
        /*
         * increase mask to prohibit current pending packets sending to this fd
         */
        array_reserve(p->fd_mask, fd + 1);
        u32 current_mask = array_get(p->fd_mask, u32, fd);
        current_mask++;
        array_set(p->fd_mask, fd, &current_mask);
        /*
         * clear fd client buffer
         */
        struct cs_client_buffer *cb = map_get(p->clients_datas, struct cs_client_buffer *, qpkey(fd));
        if(cb) {
                __client_buffer_free(cb);
                map_remove_key(p->clients_datas, &fd, sizeof(fd));
        }
        /*
         * need shutdown and close fd : important!
         */
        shutdown(fd, SHUT_RDWR);
        socket_close(fd);
        file_descriptor_set_remove(p->master, fd);
        pthread_mutex_unlock(&p->client_data_mutex);
        debug("cs_server: force close connection\n\n");
}

static void __cs_server_check_old_and_close_client(struct cs_server *ws, int fd)
{
        int can_close = 1;
        /*
         * make sure fd will be closed once
         */
        pthread_mutex_lock(&ws->client_data_mutex);
        int i;
        for_i(i, ws->fd_invalids->len) {
                i32 cfd = array_get(ws->fd_invalids, i32, i);
                if(cfd == fd) {
                        array_remove(ws->fd_invalids, i);
                        break;
                }
        }
        pthread_mutex_unlock(&ws->client_data_mutex);

        if(can_close) __cs_server_close_client(ws, fd);
}

static void __cs_server_push_close(struct cs_server *p, int fd)
{
        pthread_mutex_lock(&p->client_data_mutex);

        array_reserve(p->fd_mask, fd + 1);
        u32 current_mask = array_get(p->fd_mask, u32, fd);
        current_mask++;
        array_set(p->fd_mask, fd, &current_mask);

        array_push(p->fd_invalids, &fd);
        pthread_mutex_unlock(&p->client_data_mutex);
}

void cs_server_send_to_client(struct cs_server *p, int fd, u32 mask, char *ptr, int len, u8 keep)
{
        pthread_mutex_lock(&p->client_data_mutex);
        array_reserve(p->fd_mask, fd + 1);
        u32 current_mask = array_get(p->fd_mask, u32, fd);
        pthread_mutex_unlock(&p->client_data_mutex);

        if(current_mask != mask) {
                /*
                 * prohibit sending to fd
                 */
                debug("cs_server: prohibit sending to client with invalid mask!\n");
                return;
        }

        debug("cs_server: send to client %d : %s\n", fd, ptr);
        int bytes_send          = 0;
        int slen                = len;
        /*
         * send packet len
         */
        pthread_mutex_lock(&p->client_data_mutex);
        u32 num                 = htonl((u32)len);
        /*
         * use MSG_NOSIGNAL to prevent killing app when send() returns -1
         */
        send(fd, &num, sizeof(num), MSG_NOSIGNAL);

        /*
         * send packet content
         */
        while(bytes_send < slen) {
                int ss = send(fd, ptr, len, MSG_NOSIGNAL);
                if(ss <= 0) break;

                bytes_send += ss;
                if(bytes_send <= 0) {
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
                debug("PUSH CLOSE\n");
                __cs_server_push_close(p, fd);
        } else {
                debug("KEEp\n");
        }
}

static void __cs_server_handle_msg(struct cs_server *ws, u32 fd, char* msg, size_t msg_len)
{
        debug("cs_server: handle message\n");
        pthread_mutex_lock(&ws->client_data_mutex);
        if( ! map_has_key(ws->clients_datas, qpkey(fd))) {
                struct cs_client_buffer *cb        = __client_buffer_alloc();
                map_set(ws->clients_datas, qpkey(fd), &cb);
        }
        struct cs_client_buffer *cb = map_get(ws->clients_datas, struct cs_client_buffer *, qpkey(fd));
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
        debug("cs_server: receive: %s\n", cb->buff->ptr);
        struct smart_object *obj = smart_object_from_json(cb->buff->ptr, cb->buff->len, &counter);
        struct string *cmd = smart_object_get_string(obj, qskey(&__key_cmd__), SMART_GET_REPLACE_IF_WRONG_TYPE);
        cs_server_delegate *delegate = map_get_pointer(ws->delegates, qskey(cmd));

        int force_close = strcmp(cb->buff->ptr, "0") == 0 ? 1 : 0;

        cb->requested_len       = 0;
        cb->buff->len           = 0;

        if(*delegate) {
                pthread_mutex_lock(&ws->client_data_mutex);
                array_reserve(ws->fd_mask, fd + 1);
                u32 mask = array_get(ws->fd_mask, u32, fd);
                pthread_mutex_unlock(&ws->client_data_mutex);

                (*delegate)(ws, fd, mask, obj);
        } else {
                if(force_close) {
                        debug("CS_SERVER force close client\n");
                        __cs_server_check_old_and_close_client(ws, fd);
                } else {
                        __cs_server_push_close(ws, fd);
                }
        }
        smart_object_free(obj);
end:;
}

void cs_server_start(struct cs_server *ws, u16 port)
{
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
                debug("cs_server error: %s\n", gai_strerror(rv));
                goto finish;
        }
        q = p;
        for(p = ai; p != NULL; p = p->ai_next) {
                int prev_listener = ws->listener;
                ws->listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
                if(ws->listener < 0) {
                        continue;
                }
                setsockopt(ws->listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

                if(bind(ws->listener, p->ai_addr, p->ai_addrlen) < 0) {
                        socket_close(ws->listener);
                        ws->listener = 0;
                        continue;
                } else {
                        if(prev_listener) {
                                socket_close(prev_listener);
                        }
                        q = p;
                        continue;
                        // break;
                }
                break;
        }
        p = q;

        if(p == NULL) {
                debug("cs_server failed to bind\n");

                goto finish;
        }
        freeaddrinfo(ai);

        if(listen(ws->listener, 10) == -1) {
                perror("listener");
                goto finish;
        }

        debug("cs_server started at port no. %d\n",port);

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

        while(1) {
                /*
                 * listen for next incomming sockets
                 */
                pthread_mutex_lock(&ws->client_data_mutex);
                file_descriptor_set_assign(ws->incomming, ws->master);
                pthread_mutex_unlock(&ws->client_data_mutex);

                // debug("cs_server: start select\n");
                if(select(ws->fdmax + 1, ws->incomming->set->ptr, NULL, NULL, NULL) == -1) {
                        perror("select");
                        break;
                }
                // debug("cs_server: process select\n");

                /*
                 * process handlers
                 */
                gettimeofday(&t1, NULL);
                list_for_each_secure(head, &ws->handlers, {
                        struct cs_server_handler *handler = (struct cs_server_handler *)
                                ((char *)head - offsetof(struct cs_server_handler, head));
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

                                        {
                                                socklen_t client_len = sizeof(struct sockaddr_storage);

                                                char hoststr[NI_MAXHOST];
                                                char portstr[NI_MAXSERV];

                                                int rc = getnameinfo((struct sockaddr *)&remoteaddr,
                                                    client_len, hoststr, sizeof(hoststr), portstr, sizeof(portstr),
                                                    NI_NUMERICHOST | NI_NUMERICSERV);

                                                if (rc == 0)
                                                    printf("New connection from %s %s\n", hoststr, portstr);
                                        }
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
                                        debug("CLOSESESE\n");
                                        __cs_server_check_old_and_close_client(ws, *fd);
                                } else {
                                        __cs_server_handle_msg(ws, *fd, recvbuf, nbytes);
                                }
                        }
                }

                /*
                 * close old sockets
                 */
        check_old_fd:;
                pthread_mutex_lock(&ws->client_data_mutex);
                while(ws->fd_invalids->len) {
                        i32 cfd = array_get(ws->fd_invalids, i32, 0);
                        array_remove(ws->fd_invalids, 0);

                        pthread_mutex_unlock(&ws->client_data_mutex);
                        __cs_server_close_client(ws, cfd);
                        pthread_mutex_lock(&ws->client_data_mutex);
                }
                pthread_mutex_unlock(&ws->client_data_mutex);
        }
        finish:;
        debug("cs_server: shutdown\n");
        shutdown(ws->listener, SHUT_RDWR);
        socket_close(ws->listener);
        string_free(ports);
        sfree(recvbuf);
        array_free(actives);

        #undef MAX_RECV_BUF_LEN
}

struct cs_server *cs_server_alloc()
{
        struct cs_server *p     = smalloc(sizeof(struct cs_server));
        p->master               = file_descriptor_set_alloc();
        p->incomming            = file_descriptor_set_alloc();
        p->fdmax                = 0;
        p->listener             = 0;
        p->root                 = string_alloc(0);

        INIT_LIST_HEAD(&p->handlers);
        INIT_LIST_HEAD(&p->user_head);

        p->fd_mask              = array_alloc(sizeof(u32), ORDERED);
        p->fd_invalids          = array_alloc(sizeof(i32), NO_ORDERED);

        p->clients_datas        = map_alloc(sizeof(struct cs_client_buffer *));
        pthread_mutex_init(&p->client_data_mutex, NULL);

        p->delegates            = map_alloc(sizeof(cs_server_delegate));

        p->config               = NULL;

        return p;
}


void cs_server_free(struct cs_server *p)
{
        list_del_init(&p->user_head);

        string_free(p->root);
        file_descriptor_set_free(p->master);
        file_descriptor_set_free(p->incomming);

        array_free(p->fd_mask);
        array_free(p->fd_invalids);

        map_free(p->delegates);

        struct list_head *head;
        list_while_not_singular(head, &p->handlers) {
                struct cs_server_handler *handler = (struct cs_server_handler *)
                        ((char *)head - offsetof(struct cs_server_handler, head));
                cs_server_handler_free(handler);
        }

        pthread_mutex_lock(&p->client_data_mutex);
        map_deep_free(p->clients_datas, struct cs_client_buffer *, __client_buffer_free);
        pthread_mutex_unlock(&p->client_data_mutex);
        pthread_mutex_destroy(&p->client_data_mutex);

        if(p->config) {
                smart_object_free(p->config);
        }

        sfree(p);
}
