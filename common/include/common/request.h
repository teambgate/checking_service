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

void cs_requester_free(struct cs_requester *p);
