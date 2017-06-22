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
#include <s2/s2_lat_lng.h>
#include <s2/s2_point.h>
#include <cherry/memory.h>
#include <cherry/stdio.h>

static jclass           __class = NULL;
static jmethodID        __static_method_from_degrees = NULL;
static jmethodID        __method_to_point = NULL;

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
                        "com/google/common/geometry/S2LatLng"));
                __static_method_from_degrees    = (*__jni_env)->GetStaticMethodID(__jni_env,
                        __class,
                        "fromDegrees",
                        "(DD)Lcom/google/common/geometry/S2LatLng;"
                );
                __method_to_point               = (*__jni_env)->GetMethodID(__jni_env,
                        __class,
                        "toPoint",
                        "()Lcom/google/common/geometry/S2Point;");
        }
}

struct s2_lat_lng *s2_lat_lng_alloc(double lat, double lng)
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        __setup();
        struct s2_lat_lng *p    = smalloc(sizeof(struct s2_lat_lng));

        jobject obj             = (*__jni_env)->CallStaticObjectMethod(__jni_env,
                __class, __static_method_from_degrees, lat, lng);

        p->obj                  = (*__jni_env)->NewGlobalRef(__jni_env, obj);

        (*__jni_env)->DeleteLocalRef(__jni_env, obj);
        return p;
}

void s2_lat_lng_free(struct s2_lat_lng *p)
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        __setup();

        (*__jni_env)->DeleteGlobalRef(__jni_env, p->obj);
        sfree(p);
}

struct s2_point *s2_lat_lng_to_point(struct s2_lat_lng *p)
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        __setup();

        jobject point =  (*__jni_env)->CallObjectMethod(__jni_env, p->obj, __method_to_point);
        return s2_point_alloc(point);
}
