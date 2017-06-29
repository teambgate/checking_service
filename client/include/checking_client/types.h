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
#ifndef __CHECKING_SERVICE_CHECKING_CLIENT_TYPES_H__
#define __CHECKING_SERVICE_CHECKING_CLIENT_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <native_ui/types.h>
#include <common/types.h>
#include <smartfox/types.h>

typedef void(*nexec_listenf)(struct nexec *, struct sobj *);

#define controller_parse(val, ...) \
        struct nexec *val = nexec_parse(\
                (struct nexec_ppr) {\
                        __VA_ARGS__\
                }\
        )

#ifdef __cplusplus
}
#endif

#endif
