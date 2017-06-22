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
#include <s2/s2_point.h>
#include <cherry/memory.h>

struct s2_point *s2_point_alloc(jobject obj)
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        struct s2_point *p      = smalloc(sizeof(struct s2_point));
        p->obj                  = (*__jni_env)->NewGlobalRef(__jni_env, obj);
        return p;
}

void s2_point_free(struct s2_point *p)
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        (*__jni_env)->DeleteGlobalRef(__jni_env, p->obj);
        sfree(p);
}
