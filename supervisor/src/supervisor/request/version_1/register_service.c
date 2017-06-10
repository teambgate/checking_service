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
#include "version.h"

#include <cherry/memory.h>
#include <cherry/string.h>
#include <cherry/server/file_descriptor.h>
#include <cherry/stdio.h>
#include <cherry/array.h>
#include <cherry/map.h>
#include <cherry/bytes.h>
#include <cherry/math/math.h>
#include <cherry/server/socket.h>

#include <s2/s2_helper.h>
#include <s2/s2_cell_union.h>
#include <s2/s2_lat_lng.h>
#include <s2/s2_cell_id.h>
#include <s2/s2_point.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <netinet/in.h>

#include <cherry/unistd.h>
#include <smartfox/data.h>

#include <common/command.h>
#include <common/key.h>
#include <common/error.h>

static void __response_invalid_data(struct supervisor *p, int fd, struct sfs_object *obj, char *msg, size_t msg_len)
{
        struct sfs_object *res = sfs_object_alloc();
        sfs_object_set_long(res, qskey(&__key_request_id__), sfs_object_get_long(obj, qskey(&__key_request_id__), 0));
        sfs_object_set_bool(res, qskey(&__key_result__), 0);
        sfs_object_set_string(res, qskey(&__key_message__), msg, msg_len);
        sfs_object_set_long(res, qskey(&__key_error__), ERROR_DATA_INVALID);

        struct string *d        = sfs_object_to_json(res);
        supervisor_send_to_client(p, fd, d->ptr, d->len);
        string_free(d);
        sfs_object_free(res);
}

static void __response_invalid_server(struct supervisor *p, int fd, struct sfs_object *obj)
{
        struct sfs_object *res = sfs_object_alloc();
        sfs_object_set_long(res, qskey(&__key_request_id__), sfs_object_get_long(obj, qskey(&__key_request_id__), 0));
        sfs_object_set_bool(res, qskey(&__key_result__), 0);
        sfs_object_set_string(res, qskey(&__key_message__), qlkey(""));
        sfs_object_set_long(res, qskey(&__key_error__), ERROR_SERVER_INVALID);

        struct string *d        = sfs_object_to_json(res);
        supervisor_send_to_client(p, fd, d->ptr, d->len);
        string_free(d);
        sfs_object_free(res);
}


static void __response_success(struct supervisor *p, int fd, struct sfs_object *obj)
{
        struct sfs_object *res = sfs_object_alloc();
        sfs_object_set_long(res, qskey(&__key_request_id__), sfs_object_get_long(obj, qskey(&__key_request_id__), 0));
        sfs_object_set_bool(res, qskey(&__key_result__), 1);
        sfs_object_set_string(res, qskey(&__key_message__), qlkey("success"));

        struct string *d        = sfs_object_to_json(res);
        supervisor_send_to_client(p, fd, d->ptr, d->len);
        string_free(d);
        sfs_object_free(res);
}

static void __reserve_cell(struct supervisor *p, u64 cell_id)
{
        struct sfs_object *data         = sfs_object_alloc();
        struct string *data_json = sfs_object_to_json(data);

        MYSQL_STMT *stmt;
        stmt = mysql_stmt_init(p->db_connection);
        struct string *statement = string_alloc_chars(qlkey("INSERT IGNORE INTO cell(cellid, attributes) VALUES(__cell_id__, \'__json__\');"));

        struct string *s        = string_alloc(0);
        string_cat_unsigned_long(s, cell_id);
        string_replace(statement, "__cell_id__", s->ptr);
        string_free(s);

        string_replace(statement, "__json__", data_json->ptr);

        mysql_stmt_prepare(stmt, statement->ptr, statement->len);
        mysql_stmt_execute(stmt);
        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);

        sfs_object_free(data);
        string_free(data_json);
        string_free(statement);
}

static void __push_company_to_cell(struct supervisor *p, u64 cell_id, u32 company_id)
{
        struct sfs_object *query_obj = NULL;
        struct string *query = string_alloc_chars(qlkey("SELECT * FROM cell WHERE cellid = "));
        string_cat_unsigned_long(query, cell_id);
        string_cat(query, qlkey(";"));

search:;
        if (mysql_query(p->db_connection, query->ptr)) {
                fprintf(stderr, "%s\n", mysql_error(p->db_connection));
                goto end;
        }

        MYSQL_RES *result = mysql_store_result(p->db_connection);
        if (result == NULL) {
                fprintf(stderr, "%s\n", mysql_error(p->db_connection));
                goto end;
        }

        int num_fields = mysql_num_fields(result);
        MYSQL_ROW row;
        MYSQL_FIELD *field;
        int attributes_field_index = -1;
        int counter = 0;
        int i, j;

        for_i(i, num_fields) {
                field = mysql_fetch_field(result);
                if(strcmp(field->name, "attributes") == 0) {
                        attributes_field_index = i;
                }
        }

        while ((row = mysql_fetch_row(result))) {
                for_i(i, num_fields) {
                        if(i == attributes_field_index) {
                                char *content = row[i];
                                counter = 0;
                                query_obj = sfs_object_from_json(content, strlen(content), &counter);
                                break;
                        }
                }
                if(query_obj) break;
        }
        mysql_free_result(result);

        if(!query_obj) {
                __reserve_cell(p, cell_id);
                goto search;
        }

        struct sfs_data *companies = sfs_object_get_number_array(query_obj, qskey(&__key_companies__), SFS_INT_ARRAY, SFS_GET_EXPAND);
        for_i(i, companies->_number_array->len) {
                u32 cid = (u32)sfs_number_array_get_int(companies, i);
                if(cid == company_id) {
                        goto end;
                }
        }

        struct sfs_number num;
        num._int = company_id;
        array_push(companies->_number_array, &num);

update:;
        struct string *data_json = sfs_object_to_json(query_obj);
        MYSQL_STMT *stmt;
        stmt = mysql_stmt_init(p->db_connection);
        struct string *statement = string_alloc_chars(qlkey("UPDATE cell SET attributes=\'__json__\' WHERE cellid = "));
        string_cat_unsigned_long(statement, cell_id);
        string_cat(statement, qlkey(";"));
        string_replace(statement, "__json__", data_json->ptr);

        mysql_stmt_prepare(stmt, statement->ptr, statement->len);
        mysql_stmt_execute(stmt);
        mysql_stmt_free_result(stmt);
        mysql_stmt_close (stmt);

        string_free(data_json);
        string_free(statement);

end:;
        string_free(query);
        if(query_obj) sfs_object_free(query_obj);
finish:;
}

void supervisor_process_register_service_v1(struct supervisor *p, int fd, struct sfs_object *obj)
{
        /*
         * read service id
         */
        if(!map_has_key(obj->data, qskey(&__key_id__))
                || !map_has_key(obj->data, qskey(&__key_lat__))
                || !map_has_key(obj->data, qskey(&__key_lng__))
                || !map_has_key(obj->data, qskey(&__key_ip__))
                || !map_has_key(obj->data, qskey(&__key_port__))
                || !map_has_key(obj->data, qskey(&__key_pass__))) {
                __response_invalid_data(p, fd, obj, qlkey("not enough param"));
                goto finish;
        }

        i32 id                  = sfs_object_get_int(obj, qskey(&__key_id__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        double lat              = sfs_object_get_double(obj, qskey(&__key_lat__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        double lng              = sfs_object_get_double(obj, qskey(&__key_lng__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct string *ip       = sfs_object_get_string(obj, qskey(&__key_ip__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        u16 port                = sfs_object_get_short(obj, qskey(&__key_port__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        struct string *pass     = sfs_object_get_string(obj, qskey(&__key_pass__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        if(strcmp(ip->ptr, "") == 0) {
                __response_invalid_data(p, fd, obj, qlkey("ip is empty"));
                goto finish;
        }

        /*
         * query service from database
         */
        struct sfs_object *query_obj = NULL;
        struct string *query = string_alloc_chars(qlkey("SELECT * FROM service WHERE id = "));
        string_cat_int(query, id);
        string_cat(query, qlkey(";"));

        if (mysql_query(p->db_connection, query->ptr)) {
                fprintf(stderr, "%s\n", mysql_error(p->db_connection));
                __response_invalid_server(p, fd, obj);
                goto end;
        }

        MYSQL_RES *result = mysql_store_result(p->db_connection);

        if (result == NULL) {
                fprintf(stderr, "%s\n", mysql_error(p->db_connection));
                __response_invalid_server(p, fd, obj);
                goto end;
        }

        int num_fields = mysql_num_fields(result);
        MYSQL_ROW row;
        MYSQL_FIELD *field;
        int i, j;
        int attributes_field_index = -1;

        for_i(i, num_fields) {
                field = mysql_fetch_field(result);
                if(strcmp(field->name, "attributes") == 0) {
                        attributes_field_index = i;
                }
        }

        while ((row = mysql_fetch_row(result))) {
                for_i(i, num_fields) {
                        if(i == attributes_field_index) {
                                char *content = row[i];
                                int counter = 0;
                                query_obj = sfs_object_from_json(content, strlen(content), &counter);
                                break;
                        }
                }
                if(query_obj) break;
        }
        mysql_free_result(result);

        /*
         * process data
         */
        if(!query_obj) {
                __response_invalid_data(p, fd, obj, qlkey("service is not available!\n"));
                goto end;
        }

        struct string *service_pass = sfs_object_get_string(query_obj, qskey(&__key_pass__), SFS_GET_EXPAND);
        if(strcmp(service_pass->ptr, pass->ptr) != 0) {
                __response_invalid_data(p, fd, obj, qlkey("wrong password"));
                goto end;
        }

        struct s2_cell_id *input_cell = s2_helper_get_cellid(p->s2_helper, lat, lng, p->s2_helper->level);
        __push_company_to_cell(p, input_cell->id, (u32)id);

        struct sfs_array *cells     = sfs_object_get_array(query_obj, qskey(&__key_cell__), SFS_GET_EXPAND);

        for_i(i, cells->data->len) {
                struct sfs_object *cell = sfs_array_get_object(cells, i, SFS_GET_EXPAND);
                u64 cid                 = sfs_object_get_unsigned_long(cell, qskey(&__key_id__), SFS_GET_EXPAND);
                if(cid == input_cell->id) {
                        struct sfs_array *services = sfs_object_get_array(cell, qskey(&__key_services__), SFS_GET_EXPAND);
                        for_i(j, services->data->len) {
                                struct sfs_object *service      = sfs_array_get_object(services, j, SFS_GET_EXPAND);
                                struct string *service_ip       = sfs_object_get_string(service, qskey(&__key_ip__), SFS_GET_EXPAND);
                                u16 service_port                = sfs_object_get_short(service, qskey(&__key_port__), SFS_GET_EXPAND);
                                if(strcmp(ip->ptr, service_ip->ptr) == 0) {
                                        if(port != service_port) {
                                                sfs_object_set_long(service, qskey(&__key_port__), port);
                                                goto update_attributes;
                                        } else {
                                                goto return_success;
                                        }
                                }
                        }
                        struct sfs_object *service = sfs_object_alloc();
                        sfs_object_set_string(service, qskey(&__key_ip__), ip->ptr, ip->len);
                        sfs_object_set_long(service, qskey(&__key_port__), port);
                        sfs_array_add_object(services, service);
                        goto update_attributes;
                }
        }
new_cell:; {
        struct sfs_object *cell = sfs_object_alloc();
        sfs_object_set_unsigned_long(cell, qskey(&__key_id__), input_cell->id);
        struct sfs_array *services = sfs_object_get_array(cell, qskey(&__key_services__), SFS_GET_EXPAND);
        struct sfs_object *service = sfs_object_alloc();
        sfs_object_set_string(service, qskey(&__key_ip__), ip->ptr, ip->len);
        sfs_object_set_long(service, qskey(&__key_port__), port);
        sfs_array_add_object(services, service);
        sfs_array_add_object(cells, cell);
}

update_attributes:; {
        struct string *data_json = sfs_object_to_json(query_obj);

        MYSQL_STMT *stmt;
        stmt = mysql_stmt_init(p->db_connection);
        struct string *statement = string_alloc_chars(qlkey("UPDATE service SET attributes=\'__json__\' WHERE id = "));
        string_cat_int(statement, id);
        string_cat(statement, qlkey(";"));
        string_replace(statement, "__json__", data_json->ptr);

        mysql_stmt_prepare(stmt, statement->ptr, statement->len);
        mysql_stmt_execute(stmt);
        mysql_stmt_free_result(stmt);
        mysql_stmt_close (stmt);

        string_free(data_json);
        string_free(statement);
}

return_success:;
        s2_cell_id_free(input_cell);
        __response_success(p, fd, obj);
end:;
        string_free(query);
        if(query_obj) sfs_object_free(query_obj);
finish:;
}
