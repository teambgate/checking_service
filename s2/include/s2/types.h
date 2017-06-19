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
#ifndef __CHECKING_SERVICE_S2_TYPES_H__
#define __CHECKING_SERVICE_S2_TYPES_H__

#include <cherry/types.h>
#include <jni.h>

extern JNIEnv* __jni_env;

struct s2_helper {
        jobject         obj;

        int             level;
};

struct s2_cell_id {
        jobject         obj;

        u64             id;
};

struct s2_cell_union {
        jobject         obj;
        struct array    *cellids;
};

struct s2_lat_lng {
        jobject         obj;
};

struct s2_point {
        jobject         obj;
};

#endif
