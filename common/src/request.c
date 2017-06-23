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
#include <cherry/memory.h>
#include <cherry/list.h>
#include <cherry/array.h>
#include <cherry/map.h>
#include <cherry/string.h>
#include <cherry/stdio.h>
#include <cherry/stdlib.h>
#include <smartfox/data.h>
#include <common/key.h>
#include <common/command.h>
#include <common/error.h>

#include <pthread.h>
#include <cherry/lock.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <cherry/unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <cherry/server/file_descriptor.h>

static struct smart_object *__build_request_data(struct smart_object *obj,
        char *version, size_t version_len,
        char *pass, size_t pass_len)
{
        /*
         * get map command, path, data
         */
        struct string *request = smart_object_get_string(obj, qlkey("request"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct string *path = smart_object_get_string(obj, qlkey("path"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        struct smart_object *objdata = smart_object_get_object(obj, qlkey("data"), SMART_GET_REPLACE_IF_WRONG_TYPE);
        /*
         * create request
         */
        struct smart_object *data = smart_object_alloc();
        smart_object_set_string(data, qskey(&__key_version__), version, version_len);
        if(strcmp(request->ptr, "post") == 0) {
                smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_post__));
        } else if(strcmp(request->ptr, "get") == 0) {
                smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_get__));
        } else if(strcmp(request->ptr, "put") == 0) {
                smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_put__));
        } else if(strcmp(request->ptr, "delete") == 0) {
                smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_delete__));
        }
        smart_object_set_string(data, qskey(&__key_pass__), pass, pass_len);
        smart_object_set_string(data, qskey(&__key_path__), qskey(path));
        /*
         * set data
         */
        struct string *json = smart_object_to_json(objdata);
        int counter = 0;
        struct smart_object *d = smart_object_from_json(json->ptr, json->len, &counter);
        string_free(json);
        smart_object_set_object(data, qskey(&__key_data__), d);

        return data;
}

struct smart_object *cs_request_data_from_string(char *content, size_t len,
        char *version, size_t version_len,
        char *pass, size_t pass_len)
{
        int counter = 0;
        struct smart_object *obj = smart_object_from_json(content, len, &counter);

        struct smart_object *data = __build_request_data(obj, version, version_len,
                pass, pass_len);

        smart_object_free(obj);

        return data;
}

struct smart_object *cs_request_data_from_file(char *file, int file_type,
        char *version, size_t version_len,
        char *pass, size_t pass_len)
{
        /*
         * read map data
         */
        struct smart_object *obj = smart_object_from_json_file(file, file_type);

        struct smart_object *data = __build_request_data(obj, version, version_len,
                pass, pass_len);

        smart_object_free(obj);

        return data;
}

struct string *cs_request_string_from_file(char *file, int file_type)
{
        return file_read_string(file, file_type);
}

static struct cs_response *__cs_response_alloc(cs_request_callback callback, void *ctx)
{
        struct cs_response *p   = smalloc(sizeof(struct cs_response));
        p->callback             = callback;
        p->ctx                  = ctx;
        p->data                 = NULL;
        return p;
}

static void __cs_response_free(struct cs_response *p)
{
        if(p->data) {
                smart_object_free(p->data);
        }
        sfree(p);
}

/*
 * cs request
 */
struct cs_request *cs_request_alloc(struct cs_requester *p, struct smart_object *data, cs_request_callback callback, void *ctx)
{
        pthread_mutex_lock(&p->run_mutex);

        struct cs_request *r    = smalloc(sizeof(struct cs_request));
        r->data                 = data;
        r->host                 = string_alloc(0);
        r->port                 = 0;
        r->timeout              = -1;

        struct smart_number num;
        num._generic_integer    = p->total++;
        smart_object_set(r->data, qskey(&__key_request_id__), SMART_LONG, qpkey(num));

        list_add_tail(&r->head, &p->list);

        struct cs_response *res = __cs_response_alloc(callback, ctx);
        res->num = num._int;

        pthread_mutex_lock(&p->wait_lock);
        map_set(p->waits, qpkey(num), &res);
        pthread_mutex_unlock(&p->wait_lock);

        pthread_mutex_unlock(&p->run_mutex);
        pthread_cond_signal(&p->run_cond);

        return r;
}

struct cs_request *cs_request_alloc_with_host(struct cs_requester *p, struct smart_object *data,
        cs_request_callback callback, void *ctx, char *host, size_t host_len, u16 port)
{
        pthread_mutex_lock(&p->run_mutex);

        struct cs_request *r    = smalloc(sizeof(struct cs_request));
        r->data                 = data;
        r->host                 = string_alloc_chars(host, host_len);
        r->port                 = port;
        r->timeout              = -1;

        struct smart_number num;
        num._generic_integer    = p->total++;
        smart_object_set(r->data, qskey(&__key_request_id__), SMART_LONG, qpkey(num));

        list_add_tail(&r->head, &p->list);

        struct cs_response *res = __cs_response_alloc(callback, ctx);
        res->num = num._int;

        pthread_mutex_lock(&p->wait_lock);
        map_set(p->waits, qpkey(num), &res);
        pthread_mutex_unlock(&p->wait_lock);

        pthread_mutex_unlock(&p->run_mutex);
        pthread_cond_signal(&p->run_cond);

        return r;
}

struct cs_request *cs_request_alloc_with_param(struct cs_requester *p, struct smart_object *data,
        cs_request_callback callback, void *ctx, struct cs_request_param param)
{
        pthread_mutex_lock(&p->run_mutex);

        struct cs_request *r    = smalloc(sizeof(struct cs_request));
        r->data                 = data;

        r->host                 = param.host ?
                string_alloc_chars(param.host, strlen(param.host)) : string_alloc(0);
        r->port                 = param.port;
        r->timeout              = param.timeout;

        struct smart_number num;
        num._generic_integer    = p->total++;
        smart_object_set(r->data, qskey(&__key_request_id__), SMART_LONG, qpkey(num));

        list_add_tail(&r->head, &p->list);

        struct cs_response *res = __cs_response_alloc(callback, ctx);
        res->num = num._int;

        pthread_mutex_lock(&p->wait_lock);
        map_set(p->waits, qpkey(num), &res);
        pthread_mutex_unlock(&p->wait_lock);

        pthread_mutex_unlock(&p->run_mutex);
        pthread_cond_signal(&p->run_cond);

        return r;
}

void cs_request_free(struct cs_request *p)
{
        list_del(&p->head);
        if(p->data) smart_object_free(p->data);
        string_free(p->host);
        sfree(p);
}

static int __try_write(struct cs_requester *p, char *msg, size_t slen)
{
        pthread_mutex_lock(&p->write_mutex);

        int bytes_send          = 0;
        int len                 = slen;
        char *ptr               = msg;
        u32 num                 = htonl((u32)len);
        int first = send(p->listener, &num, sizeof(num), MSG_NOSIGNAL);
        app_log("sended first %d\n", first);
        if(first <= 0) {
                p->valid = 0;
                pthread_mutex_unlock(&p->write_mutex);
                return 0;
        }

        while(bytes_send < slen) {
                int sent = send(p->listener, ptr, len, MSG_NOSIGNAL);
                app_log("sended %d\n", sent);
                bytes_send += sent;
                if(bytes_send <= 0 || sent <= 0) {
                        p->valid = 0;
                        pthread_mutex_unlock(&p->write_mutex);
                        return -1;
                }
                len -= bytes_send;
                ptr += bytes_send;
        }
        pthread_mutex_unlock(&p->write_mutex);

        return 1;
}

static void __response_timeout(struct cs_requester *p, struct cs_request *r)
{
        struct smart_object *data = smart_object_alloc();
        smart_object_set_long(data, qskey(&__key_request_id__), smart_object_get_long(r->data, qskey(&__key_request_id__), SMART_GET_REPLACE_IF_WRONG_TYPE));
        smart_object_set_bool(data, qskey(&__key_result__), 0);
        smart_object_set_string(data, qskey(&__key_message__), qlkey("timeout"));
        smart_object_set_long(data, qskey(&__key_error__), ERROR_TIMEOUT);

        struct smart_data *id     = map_get(data->data, struct smart_data *, qskey(&__key_request_id__));
        pthread_mutex_lock(&p->wait_lock);
        struct cs_response *res = map_get(p->waits, struct cs_response *, qpkey(id->_number));
        if(res) {
                map_remove_key(p->waits, qpkey(id->_number));
                res->data       = data;
        }
        pthread_mutex_unlock(&p->wait_lock);
        if(res) {
                if(res->callback) res->callback(res->ctx, res->data);
                __cs_response_free(res);
        }
}


/*
 * cs requester
 */
static int count = 0;
static void *cs_requester_write(struct cs_requester *p)
{
        int *send_valid = p->send_valid;
        struct list_head *head = NULL;

process_request:;
        if((*send_valid) == 0)
                goto finish;

        head = NULL;
        pthread_mutex_lock(&p->run_mutex);

get_head:;
        if(!list_singular(&p->list)) {
                head = p->list.next;
                list_del_init(head);
        }

        while(!head) {
                pthread_cond_wait(&p->run_cond, &p->run_mutex);

                if((*send_valid) == 0) {
                        pthread_mutex_unlock(&p->run_mutex);
                        goto finish;
                }

                goto get_head;
        }

        pthread_mutex_lock(&p->read_mutex);
        struct cs_request *r    = (struct cs_request *)
                ((char *)head - offsetof(struct cs_request, head));
        struct string *d        = smart_object_to_json(r->data);

        while(!cs_requester_reconnect(p, r->host->ptr, r->host->len,
                        r->port, r->timeout <= 0 ? 5 : r->timeout)) {
                /*
                 * treat all reconnection fails as timeout
                 */
                if((*send_valid) == 0) {
                        pthread_mutex_unlock(&p->run_mutex);
                        pthread_mutex_unlock(&p->read_mutex);
                        goto finish;
                } else {
                        __response_timeout(p, r);
                        pthread_mutex_unlock(&p->run_mutex);
                        pthread_mutex_unlock(&p->read_mutex);
                        goto head_done;
                }
        }
        p->valid = 1;

        debug("start write\n");
        __try_write(p, qskey(d));

        /*
         * enable read
         */
        pthread_mutex_unlock(&p->read_mutex);
        pthread_cond_signal(&p->read_cond);

head_done:;
        string_free(d);
        cs_request_free(r);
        pthread_mutex_unlock(&p->run_mutex);
        debug("end write\n");
check_life_time:;
        /*
         * wait until server disconnect this socket
         */
        if(p->listener >= 0) {
                pthread_mutex_lock(&p->write_mutex);
                pthread_cond_wait(&p->write_cond, &p->write_mutex);

                if((*send_valid) == 0) {
                        pthread_mutex_unlock(&p->write_mutex);
                        goto finish;
                }
                pthread_mutex_unlock(&p->write_mutex);
        }

        goto process_request;

finish:
        free(send_valid);
        pthread_exit(NULL);
}

static void *cs_requester_read(struct cs_requester *p)
{
#define MAX_RECV_BUF_LEN 99999
        struct timeval t1, t2;
        double elapsedTime;
        int *read_valid         = p->read_valid;

        char *recvbuf           = smalloc(sizeof(char) * MAX_RECV_BUF_LEN);
        int nbytes              = 0;

        pthread_mutex_lock(&p->read_mutex);
        pthread_cond_wait(&p->read_cond, &p->read_mutex);

        if((*read_valid) == 0) {
                pthread_mutex_unlock(&p->read_mutex);
                goto finish;
        }
        pthread_mutex_unlock(&p->read_mutex);

process_request:;
        if((*read_valid) == 0)
                goto finish;

        int msg_len;
        char *msg;
        debug("start read\n");
        if(p->valid) {
                nbytes = recv(p->listener, recvbuf, MAX_RECV_BUF_LEN, 0);
        } else {
                nbytes = 0;
        }
        msg = recvbuf;
        msg_len = nbytes;

check_bytes:;
        if(nbytes <= 0) {
                pthread_mutex_lock(&p->read_mutex);

                shutdown(p->listener, SHUT_RDWR);
                close(p->listener);
                p->listener = -1;
                debug("client closed!\n");

                pthread_cond_signal(&p->write_cond);
                if(p->listener < 0) {
                        pthread_cond_wait(&p->read_cond, &p->read_mutex);
                        if((*read_valid) == 0) {
                                pthread_mutex_unlock(&p->read_mutex);
                                goto finish;
                        }
                }
                pthread_mutex_unlock(&p->read_mutex);
                goto check_life_time;
        }

check:;
        if(msg_len <= 0) {
                __try_write(p, qlkey("0"));
                goto check_life_time;
        }

        int amount      = 0;
        if(p->requested_len == 0) {
                if(msg_len < sizeof(u32)) {
                        amount = msg_len;
                        string_cat(p->buff, msg, amount);
                        goto check_life_time;
                } else {
                        if(p->buff->len) {
                                amount = sizeof(u32) - p->buff->len;
                                string_cat(p->buff, msg, amount);
                                u32 *num                = (u32 *)p->buff->ptr;
                                p->requested_len        = ntohl(*num);
                                msg                     += amount;
                                msg_len                 -= amount;
                                p->buff->len = 0;
                        } else {
                                u32 *num                = (u32 *)msg;
                                p->requested_len        = ntohl(*num);
                                msg                     += sizeof(u32);
                                msg_len                 -= sizeof(u32);
                        }
                }
        }
        if(msg_len + p->buff->len >= p->requested_len) {
                goto receive_full_packet;
        } else {
                string_cat(p->buff, msg, msg_len);
                goto check_life_time;
        }


receive_full_packet:;
        amount = p->requested_len - p->buff->len;
        string_cat(p->buff, msg, amount);
        msg += amount;
        msg_len -= amount;

        int counter     = 0;
        struct smart_object *data = smart_object_from_json(p->buff->ptr, p->buff->len, &counter);
        struct smart_data *id     = map_get(data->data, struct smart_data *, qskey(&__key_request_id__));
        pthread_mutex_lock(&p->wait_lock);
        struct cs_response *res = map_get(p->waits, struct cs_response *, qpkey(id->_number));
        if(res) {
                map_remove_key(p->waits, qpkey(id->_number));
                res->data       = data;
        }
        pthread_mutex_unlock(&p->wait_lock);
        if(res) {
                if(res->callback) res->callback(res->ctx, res->data);
                __cs_response_free(res);
        }
        p->requested_len       = 0;
        p->buff->len           = 0;
        /*
         * send temporary signal to server to clear previous session
         */
        goto check;

check_life_time:;
        goto process_request;

finish:
        debug("free buf\n");
        sfree(recvbuf);
        free(read_valid);
        pthread_exit(NULL);
#undef MAX_RECV_BUF_LEN
}

struct cs_requester *cs_requester_alloc()
{
        struct cs_requester *p  = smalloc(sizeof(struct cs_requester));
        p->total                = 0;
        p->life_time            = 60;
        p->listener             = -1;
        p->buff                 = string_alloc(0);
        p->requested_len        = 0;
        p->host                 = string_alloc(0);
        p->port                 = 0;
        INIT_LIST_HEAD(&p->list);

        p->wset                 = file_descriptor_set_alloc();
        p->eset                 = file_descriptor_set_alloc();
        p->actives              = array_alloc(sizeof(u32), ORDERED);
        p->e_actives              = array_alloc(sizeof(u32), ORDERED);

        p->valid                = 1;

        pthread_mutex_init(&p->run_mutex, NULL);
        pthread_mutex_init(&p->lock, NULL);

        pthread_mutex_init(&p->wait_lock, NULL);
        pthread_cond_init (&p->run_cond, NULL);

        pthread_mutex_init(&p->write_mutex, NULL);
        pthread_cond_init (&p->write_cond, NULL);

        pthread_mutex_init(&p->read_mutex, NULL);
        pthread_cond_init (&p->read_cond, NULL);

        p->waits                = map_alloc(sizeof(struct cs_response *));

        p->read_valid           = malloc(sizeof(int));
        p->send_valid           = malloc(sizeof(int));
        (*p->send_valid)        = 1;
        (*p->read_valid)        = 1;

        gettimeofday(&p->t1, NULL);

        return p;
}

static void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static int __set_blocking(int fd, int blocking)
{
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0) return 0;
        flags = blocking == 1 ? (flags &~ O_NONBLOCK) : (flags | O_NONBLOCK);
        return (fcntl(fd, F_SETFL, flags) == 0) ? 1 : 0;
}

int cs_requester_reconnect(struct cs_requester *requester, char *host, size_t host_len, u16 port, int timeout)
{
        debug("start reconnect\n");

        if(host_len == 0) {
                host = requester->host->ptr;
                port = requester->port;
        }

        int numbytes;
        struct addrinfo hints, *servinfo, *p;

        int rv;
        char s[INET6_ADDRSTRLEN];

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        struct string *ports    = string_alloc(0);
        string_cat_int(ports, port);

        if ((rv = getaddrinfo(host, ports->ptr, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                string_free(ports);
                requester->listener = -1;
                return 0;
        }
        string_free(ports);
        debug("finish get addr\n");

        for(p = servinfo; p != NULL; p = p->ai_next) {
                debug("try listen 1\n");
                if ((requester->listener = socket(p->ai_family, p->ai_socktype,
                                p->ai_protocol)) == -1) {
                        perror("client: socket");
                        continue;
                }
                debug("try listen 2\n");

                __set_blocking(requester->listener, 0);
                file_descriptor_set_clean(requester->wset);
                file_descriptor_set_clean(requester->eset);
                file_descriptor_set_add(requester->wset, requester->listener);
                file_descriptor_set_add(requester->eset, requester->listener);

                struct timeval select_timeout;
                select_timeout.tv_sec   = timeout;
                select_timeout.tv_usec  = 0;
                int connect_result = connect(requester->listener, p->ai_addr, p->ai_addrlen);
                debug("native ui : connect %d\n", connect_result);
                debug("native ui : listener %d\n", requester->listener);
                if (connect_result == -1) {
                        if(select(requester->listener + 1, requester->eset->set->ptr, requester->wset->set->ptr, NULL, &select_timeout) == -1) {
                                goto connect_failed;
                        }

                        array_force_len(requester->actives, 0);
                        array_force_len(requester->e_actives, 0);
                        file_descriptor_set_get_active(requester->wset, requester->actives);
                        file_descriptor_set_get_active(requester->eset, requester->e_actives);

                        app_log("native ui w : %d | e : %d\n", requester->actives->len, requester->e_actives->len);

                        int i;
                        for_i(i, requester->actives->len) {
                                u32 ls = array_get(requester->actives, u32, i);
                                debug("native ui : active %u\n", ls);
                        }

                        if(requester->actives->len) {
                                __set_blocking(requester->listener, 1);
                                goto end_try_connect;
                        }

                connect_failed:;
                        __set_blocking(requester->listener, 1);
                        close(requester->listener);
                        requester->listener = -1;
                        perror("client: connect");
                        continue;
                }

        end_try_connect:;
                debug("try listen 3\n");

                break;
        }
        debug("finish listen\n");

        if (p == NULL) {
                fprintf(stderr, "client: failed to connect\n");
                return 0;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
                s, sizeof s);
        debug("requester client: connecting to %s\n", s);

        freeaddrinfo(servinfo);
        debug("finish reconnect\n");

        return 1;
}

int cs_requester_connect(struct cs_requester *requester, char *host, u16 port)
{
        requester->host->len = 0;
        string_cat(requester->host, host, strlen(host));
        requester->port = port;

        pthread_t tid[2];
        pthread_create(&tid[0], NULL, (void*(*)(void*))cs_requester_write, (void*)requester);
        pthread_create(&tid[1], NULL, (void*(*)(void*))cs_requester_read, (void*)requester);

        return 1;
}

void cs_requester_free(struct cs_requester *p)
{
        pthread_mutex_lock(&p->lock);
        (*p->send_valid) = 0;
        (*p->read_valid) = 0;
        pthread_mutex_unlock(&p->lock);

        pthread_mutex_lock(&p->run_mutex);
        pthread_cond_signal(&p->run_cond);
        pthread_mutex_unlock(&p->run_mutex);

        pthread_mutex_lock(&p->write_mutex);
        pthread_cond_signal(&p->write_cond);
        pthread_mutex_unlock(&p->write_mutex);

        pthread_mutex_lock(&p->read_mutex);
        pthread_cond_signal(&p->read_cond);
        pthread_mutex_unlock(&p->read_mutex);

        struct list_head *head;
        list_for_each_secure_mutex_lock(head, &p->list, &p->lock, {
                struct cs_request *r = (struct cs_request *)
                        ((char *)head - offsetof(struct cs_request, head));
                cs_request_free(r);
        });
        map_deep_free(p->waits, struct cs_response *, __cs_response_free);

        // spin_lock_destroy(&p->lock);
        pthread_mutex_destroy(&p->run_mutex);
        pthread_mutex_destroy(&p->lock);

        pthread_mutex_destroy(&p->wait_lock);
        pthread_cond_destroy(&p->run_cond);

        pthread_mutex_destroy(&p->write_mutex);
        pthread_cond_destroy(&p->write_cond);

        pthread_mutex_destroy(&p->read_mutex);
        pthread_cond_destroy(&p->read_cond);

        string_free(p->buff);
        shutdown(p->listener, SHUT_RDWR);
        close(p->listener);
        string_free(p->host);

        file_descriptor_set_free(p->wset);
        file_descriptor_set_free(p->eset);
        array_free(p->actives);
        array_free(p->e_actives);

        sfree(p);
}
