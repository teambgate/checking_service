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
#ifndef __CHECKING_SERVICE_CHECKING_CLIENT_CONTROLLER_UTILS_H__
#define __CHECKING_SERVICE_CHECKING_CLIENT_CONTROLLER_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <checking_client/types.h>

struct nexec *cl_nexec_alloc(char *name, size_t len);

struct nexec_ppr {
        struct nexec   *exec_p;
        struct nexec   *exec_d;
        struct nview   *view_p;
        char           *file;
};

struct nexec *nexec_parse(struct nexec_ppr param);

#ifdef __cplusplus
}
#endif

#endif
