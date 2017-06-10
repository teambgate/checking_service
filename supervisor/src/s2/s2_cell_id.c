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
#include <s2/s2_cell_id.h>
#include <cherry/memory.h>

static jclass           __class = NULL;
static jmethodID        __method_id = NULL;

static void __clear()
{
        if(__class) {
                (*__jni_env)->DeleteGlobalRef(__jni_env, __class);
                __class = NULL;
        }
}

static void __setup()
{
        if(__class == NULL) {
                cache_add(__clear);

                __class                         = (*__jni_env)->NewGlobalRef(__jni_env, (*__jni_env)->FindClass(__jni_env,
                        "com/google/common/geometry/S2CellId"));

                __method_id                     = (*__jni_env)->GetMethodID(__jni_env,
                        __class,
                        "id",
                        "()J");
        }
}


struct s2_cell_id *s2_cell_id_alloc(jobject obj)
{
        __setup();
        struct s2_cell_id *p    = smalloc(sizeof(struct s2_cell_id));
        p->obj                  = (*__jni_env)->NewGlobalRef(__jni_env, obj);

        p->id                   = (*__jni_env)->CallLongMethod(__jni_env,
                p->obj, __method_id);

        return p;
}

void s2_cell_id_free(struct s2_cell_id *p)
{
        __setup();
        (*__jni_env)->DeleteGlobalRef(__jni_env, p->obj);
        sfree(p);
}
