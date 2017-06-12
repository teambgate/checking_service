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

static inline void __supervisor_send_to_client(struct supervisor *p, int fd, char *ptr, int len)
{
        debug("send client %s\n", ptr);
        int bytes_send          = 0;
        int slen                = len;
        /*
         * send packet len
         */
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
}

void supervisor_send_to_client(struct supervisor *p, int fd, char *ptr, int len)
{
        __supervisor_send_to_client(p, fd, ptr, len);
}

static void __supervisor_handle_msg(struct supervisor *ws, u32 fd, char* msg, size_t msg_len)
{
        if( ! map_has_key(ws->clients_datas, qpkey(fd))) {
                struct client_buffer *cb        = __client_buffer_alloc();
                map_set(ws->clients_datas, qpkey(fd), &cb);
        }

        struct client_buffer *cb = map_get(ws->clients_datas, struct client_buffer *, qpkey(fd));
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
        struct sfs_object *obj = sfs_object_from_json(cb->buff->ptr, cb->buff->len, &counter);
        struct sfs_data *cmd = map_get(obj->data, struct sfs_data *, qskey(&__key_cmd__));
        if(cmd) {
                supervisor_delegate *delegate = map_get_pointer(ws->delegates, qskey(cmd->_string));
                if(delegate) {
                        (*delegate)(ws, fd, obj);
                }

        }
        sfs_object_free(obj);
        cb->requested_len       = 0;
        cb->buff->len           = 0;

        debug("force client closed\n");
        if(cb) {
                __client_buffer_free(cb);
                map_remove_key(ws->clients_datas, &fd, sizeof(fd));
        }

        shutdown(fd, SHUT_RDWR);
        socket_close(fd);
        file_descriptor_set_remove(ws->master, fd);
        debug("force close connection\n\n");

        // goto check;
end:;
}

void supervisor_start(struct supervisor *ws)
{
        char *root      = sfs_object_get_string(ws->config, qlkey("service_root"), SFS_GET_REPLACE_IF_WRONG_TYPE)->ptr;
        u16 port        = sfs_object_get_short(ws->config, qlkey("service_port"), SFS_GET_REPLACE_IF_WRONG_TYPE);

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
        struct timeval t1, t2;
        double elapsedTime;
        gettimeofday(&t1, NULL);
        while(1) {
                /*
                 * listen for next incomming sockets
                 */
                file_descriptor_set_assign(ws->incomming, ws->master);
                if(select(ws->fdmax + 1, ws->incomming->set->ptr, NULL, NULL, NULL) == -1) {
                        perror("select");
                        break;
                }
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
                                        file_descriptor_set_add(ws->master, newfd);
                                        ws->fdmax = MAX(ws->fdmax, newfd);
                                        debug("new connection from %s on socket %d\n",
                                                inet_ntop(remoteaddr.ss_family,
                                                        get_in_addr((struct sockaddr *)&remoteaddr),
                                                        remoteIP, INET6_ADDRSTRLEN), newfd);
                                }
                        } else {
                                /*
                                 * receive client request
                                 */
                                nbytes = recv(*fd, recvbuf, MAX_RECV_BUF_LEN, 0);
                                if(nbytes <= 0) {
                                        debug("client closed\n");

                                        struct client_buffer *cb = map_get(ws->clients_datas, struct client_buffer *, fd, sizeof(*fd));
                                        if(cb) {
                                                __client_buffer_free(cb);
                                                map_remove_key(ws->clients_datas, fd, sizeof(*fd));
                                        }

                                        shutdown(*fd, SHUT_RDWR);
                                        socket_close(*fd);
                                        file_descriptor_set_remove(ws->master, *fd);
                                        debug("close connection\n\n");
                                } else {
                                        __supervisor_handle_msg(ws, *fd, recvbuf, nbytes);
                                }
                        }
                }
        }
finish:;
#if OS == WINDOWS
        WSACleanup();
#endif
        shutdown(ws->listener, SHUT_RDWR);
        socket_close(ws->listener);
        string_free(ports);
        sfree(recvbuf);
        array_free(actives);

#undef MAX_RECV_BUF_LEN
}

struct supervisor *supervisor_alloc()
{
        struct supervisor *p    = smalloc(sizeof(struct supervisor));
        p->master               = file_descriptor_set_alloc();
        p->incomming            = file_descriptor_set_alloc();
        p->fdmax                = 0;
        p->listener             = 0;
        p->root                 = string_alloc(0);

        /*
         * load config
         */
        p->config               = sfs_object_from_json_file("res/config.json", FILE_INNER);

        /*
         * setup delegates
         */
        p->delegates            = map_alloc(sizeof(supervisor_delegate));
        map_set(p->delegates, qskey(&__cmd_get_service__), &(supervisor_delegate){supervisor_process_get_service});
        map_set(p->delegates, qskey(&__cmd_register_service__), &(supervisor_delegate){supervisor_process_register_service});

        p->clients_datas        = map_alloc(sizeof(struct client_buffer *));
        return p;
}

void supervisor_free(struct supervisor *p)
{
        string_free(p->root);
        file_descriptor_set_free(p->master);
        file_descriptor_set_free(p->incomming);

        sfs_object_free(p->config);

        map_free(p->delegates);
        map_deep_free(p->clients_datas, struct client_buffer *, __client_buffer_free);

        sfree(p);
}
