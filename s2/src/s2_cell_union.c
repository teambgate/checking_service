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
#include <s2/s2_cell_union.h>
#include <s2/s2_lat_lng.h>
#include <s2/s2_point.h>
#include <cherry/memory.h>

static jclass           __class = NULL;
static jmethodID        __method_contains = NULL;

static void __clear()
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        if(__class) {
                (*__jni_env)->DeleteGlobalRef(__jni_env, __class);
                __class = NULL;
        }
}

static void __setup()
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        if(__class == NULL) {
                cache_add(__clear);

                __class                         = (*__jni_env)->NewGlobalRef(__jni_env, (*__jni_env)->FindClass(__jni_env,
                        "com/google/common/geometry/S2CellUnion"));

                __method_contains               =  (*__jni_env)->GetMethodID(__jni_env,
                        __class,
                        "contains",
                        "(Lcom/google/common/geometry/S2Point;)Z");
        }
}

struct s2_cell_union *s2_cell_union_alloc(jobject obj)
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        __setup();

        struct s2_cell_union *p = smalloc(sizeof(struct s2_cell_union));
        p->obj                  = (*__jni_env)->NewGlobalRef(__jni_env, obj);

        return p;
}

void s2_cell_union_free(struct s2_cell_union *p)
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        __setup();

        (*__jni_env)->DeleteGlobalRef(__jni_env, p->obj);
        sfree(p);
}

int s2_cell_union_contain(struct s2_cell_union *p, double lat, double lng)
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        struct s2_lat_lng *latlng       = s2_lat_lng_alloc(lat, lng);
        struct s2_point *point          = s2_lat_lng_to_point(latlng);

        int result                      = (*__jni_env)->CallBooleanMethod(__jni_env,
                p->obj, __method_contains, point->obj);

        s2_lat_lng_free(latlng);
        s2_point_free(point);

        return result;
}
