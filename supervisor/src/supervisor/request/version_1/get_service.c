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

struct company_search {
        struct array *cell_ids;
        struct array *companies_id;
};

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

static void __response_invalid_server(struct supervisor *p, int fd, struct sfs_object *obj, char *msg, size_t msg_len)
{
        struct sfs_object *res = sfs_object_alloc();
        sfs_object_set_long(res, qskey(&__key_request_id__), sfs_object_get_long(obj, qskey(&__key_request_id__), 0));
        sfs_object_set_bool(res, qskey(&__key_result__), 0);
        sfs_object_set_string(res, qskey(&__key_message__), msg, msg_len);
        sfs_object_set_long(res, qskey(&__key_error__), ERROR_SERVER_INVALID);

        struct string *d        = sfs_object_to_json(res);
        supervisor_send_to_client(p, fd, d->ptr, d->len);
        string_free(d);
        sfs_object_free(res);
}

static void __response_success(struct supervisor *p, int fd, struct sfs_object *obj, struct array *objs, struct company_search companies)
{
        struct sfs_object *res = sfs_object_alloc();
        sfs_object_set_long(res, qskey(&__key_request_id__), sfs_object_get_long(obj, qskey(&__key_request_id__), SFS_GET_REPLACE_IF_WRONG_TYPE));
        sfs_object_set_bool(res, qskey(&__key_result__), 1);
        sfs_object_set_string(res, qskey(&__key_message__), qlkey("success"));

        struct sfs_array *sfscompanies = sfs_array_alloc();
        sfs_object_set_array(res, qskey(&__key_companies__), sfscompanies);

        struct sfs_object **com;
        array_for_each(com, objs) {
                struct sfs_object *company = sfs_object_alloc();
                sfs_array_add_object(sfscompanies, company);

                struct string *name = sfs_object_get_string(*com, qskey(&__key_name__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                sfs_object_set_string(company, qskey(&__key_name__), qskey(name));

                struct sfs_array *services = sfs_array_alloc();
                sfs_object_set_array(company, qskey(&__key_services__), services);

                struct sfs_array *com_cell = sfs_object_get_array(*com, qskey(&__key_cell__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                int i, j;
                for_i(i, com_cell->data->len) {
                        struct sfs_object *cell = sfs_array_get_object(com_cell, i, SFS_GET_REPLACE_IF_WRONG_TYPE);
                        u64 cell_id = sfs_object_get_unsigned_long(cell, qskey(&__key_id__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                        u64 *ccid;
                        array_for_each(ccid, companies.cell_ids) {
                                if(*ccid == cell_id) {
                                        goto found;
                                }
                        }
                not_found:;
                        continue;

                found:;
                        struct sfs_array *cell_services = sfs_object_get_array(cell, qskey(&__key_services__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                        for_i(j, cell_services->data->len) {
                                struct sfs_object *cell_service = sfs_array_get_object(cell_services, j, SFS_GET_REPLACE_IF_WRONG_TYPE);
                                struct string *ip = sfs_object_get_string(cell_service, qskey(&__key_ip__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                                i16 port        = sfs_object_get_short(cell_service, qskey(&__key_port__), SFS_GET_REPLACE_IF_WRONG_TYPE);
                                struct sfs_object *target = sfs_object_alloc();
                                sfs_object_set_string(target, qskey(&__key_ip__), qskey(ip));
                                sfs_object_set_short(target, qskey(&__key_port__), port);
                                sfs_array_add_object(services, target);
                        }
                }
        }

        struct string *d        = sfs_object_to_json(res);
        supervisor_send_to_client(p, fd, d->ptr, d->len);
        string_free(d);
        sfs_object_free(res);
}


static struct company_search __get_companies_id(struct supervisor *p, double lat, double lng)
{
        struct array *cell_results      = array_alloc(sizeof(u64), ORDERED);
        struct array *companies_result  = array_alloc(sizeof(u32), ORDERED);
        struct s2_cell_union * scu      = s2_helper_get_cell_union(p->s2_helper, lat, lng, 100);
        struct array * cids             = s2_helper_get_cellids(p->s2_helper, scu);

        if(cids->len == 0) goto finish;

        struct s2_cell_id **cell_id_it;
        struct array *query_results = array_alloc(sizeof(struct sfs_object *), ORDERED);
        struct string *query = string_alloc_chars(qlkey("SELECT * FROM cell WHERE"));
        int i, j;
        array_for_each_index(cell_id_it, i, cids) {
                if(i == 0) {
                        string_cat(query, qlkey(" (cellid = "));
                } else {
                        string_cat(query, qlkey(" OR (cellid = "));
                }
                string_cat_unsigned_long(query, (*cell_id_it)->id);
                string_cat(query, qlkey(")"));

                array_push(cell_results, &(*cell_id_it)->id);
        }
        string_cat(query, qlkey(";"));

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
                                struct sfs_object *query_obj = sfs_object_from_json(content, strlen(content), &counter);
                                array_push(query_results, &query_obj);
                        }
                }
        }
        mysql_free_result(result);

        struct sfs_object **query_obj;
        array_for_each(query_obj, query_results) {
                struct sfs_data *companies = sfs_object_get_number_array(*query_obj, qskey(&__key_companies__), SFS_INT_ARRAY, SFS_GET_EXPAND);
                for_i(i, companies->_number_array->len) {
                        u32 cid = (u32)sfs_number_array_get_int(companies, i);
                        u32 *c;
                        array_for_each(c, companies_result) {
                                if((*c) == cid)
                                        goto duplicated;
                        }
                can_push:;
                        array_push(companies_result, &cid);

                duplicated:;
                }
        }

end:;
        string_free(query);
        array_deep_free(query_results, struct sfs_object *, sfs_object_free);

finish:;
        s2_cell_union_free(scu);
        array_deep_free(cids, struct s2_cell_id *, s2_cell_id_free);

        struct company_search cs;
        cs.companies_id = companies_result;
        cs.cell_ids = cell_results;

        return cs;
}

void supervisor_process_get_service_v1(struct supervisor *p, int fd, struct sfs_object *obj)
{
        double lat                      = sfs_object_get_double(obj, qskey(&__key_lat__), SFS_GET_REPLACE_IF_WRONG_TYPE);
        double lng                      = sfs_object_get_double(obj, qskey(&__key_lng__), SFS_GET_REPLACE_IF_WRONG_TYPE);

        struct company_search companies = __get_companies_id(p, lat, lng);

        u32 *cid;
        struct array *query_results = array_alloc(sizeof(struct sfs_object *), ORDERED);
        struct string *query = string_alloc_chars(qlkey("SELECT * FROM service WHERE"));

        if(companies.companies_id->len == 0) {
                goto return_success;
        }

        int i, j;
        array_for_each_index(cid, i, companies.companies_id) {
                if(i == 0) {
                        string_cat(query, qlkey(" (id = "));
                } else {
                        string_cat(query, qlkey(" OR (id = "));
                }
                string_cat_int(query, *cid);
                string_cat(query, qlkey(")"));
        }
        string_cat(query, qlkey(";"));

        if (mysql_query(p->db_connection, query->ptr)) {
                fprintf(stderr, "%s\n", mysql_error(p->db_connection));
                __response_invalid_server(p, fd, obj, qlkey("server error"));
                goto end;
        }

        MYSQL_RES *result = mysql_store_result(p->db_connection);
        if (result == NULL) {
                fprintf(stderr, "%s\n", mysql_error(p->db_connection));
                __response_invalid_server(p, fd, obj, qlkey("server error"));
                goto end;
        }

        int num_fields = mysql_num_fields(result);
        MYSQL_ROW row;
        MYSQL_FIELD *field;
        int attributes_field_index = -1;
        int counter = 0;

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
                                struct sfs_object *query_obj = sfs_object_from_json(content, strlen(content), &counter);
                                array_push(query_results, &query_obj);
                        }
                }
        }
        mysql_free_result(result);

return_success:;

        __response_success(p, fd, obj, query_results, companies);

end:;
        string_free(query);
        array_deep_free(query_results, struct sfs_object *, sfs_object_free);
        array_free(companies.companies_id);
        array_free(companies.cell_ids);
}
