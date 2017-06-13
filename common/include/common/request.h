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
#include <common/types.h>

struct sfs_object *cs_request_data_from_file(char *file, int file_type,
        char *version, size_t version_len,
        char *pass, size_t pass_len);
/*
 * cs request
 */
struct cs_request *cs_request_alloc(struct cs_requester *p, struct sfs_object *data, cs_request_callback callback, void *ctx);

void cs_request_free(struct cs_request *p);

/*
 * cs requester
 */
struct cs_requester *cs_requester_alloc();

int cs_requester_connect(struct cs_requester *p, char *host, u16 port);

int cs_requester_reconnect(struct cs_requester *p);

void cs_requester_free(struct cs_requester *p);
