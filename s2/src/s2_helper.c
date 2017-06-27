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
#include <s2/s2_helper.h>
#include <s2/s2_lat_lng.h>
#include <s2/s2_cell_union.h>
#include <s2/s2_cell_id.h>

#include <cherry/array.h>
#include <cherry/memory.h>
#include <cherry/stdio.h>

static jclass           __class = NULL;
static jmethodID        __static_method_create  = NULL;
static jmethodID        __method_get_cell_union = NULL;
static jmethodID        __method_get_cell_ids   = NULL;
static jmethodID        __method_get_cell_id    = NULL;

static jclass           __array_list_class = NULL;
static jmethodID        __array_list_method_size = NULL;
static jmethodID        __array_list_method_get = NULL;

static void __clear()
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        if(__class) {
                (*__jni_env)->DeleteGlobalRef(__jni_env, __class);
                (*__jni_env)->DeleteGlobalRef(__jni_env, __array_list_class);
                __class = NULL;
        }
}

static void __setup()
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        if(__class == NULL) {
                cache_add(__clear);

                __class = (*__jni_env)->NewGlobalRef(__jni_env, (*__jni_env)->FindClass(__jni_env, "com/bgate/s2/S2Helper"));

                __static_method_create  = (*__jni_env)->GetStaticMethodID(__jni_env,
                        __class,
                        "create",
                        "(III)Lcom/bgate/s2/S2Helper;"
                );

                __method_get_cell_union = (*__jni_env)->GetMethodID(__jni_env,
                        __class,
                        "getCellUnion",
                        "(Lcom/google/common/geometry/S2LatLng;D)Lcom/google/common/geometry/S2CellUnion;"
                );

                __method_get_cell_ids   = (*__jni_env)->GetMethodID(__jni_env,
                        __class,
                        "getCellIds",
                        "(Lcom/google/common/geometry/S2CellUnion;)Ljava/util/ArrayList;"
                );

                __method_get_cell_id    = (*__jni_env)->GetMethodID(__jni_env,
                        __class,
                        "getCellId",
                        "(DDI)Lcom/google/common/geometry/S2CellId;"
                );

                /*
                 * array list
                 */
                 __array_list_class             = (*__jni_env)->NewGlobalRef(__jni_env, (*__jni_env)->FindClass(__jni_env, "java/util/ArrayList"));

                 __array_list_method_size       = (*__jni_env)->GetMethodID(__jni_env,
                         __array_list_class,
                         "size",
                         "()I"
                 );

                 __array_list_method_get        = (*__jni_env)->GetMethodID(__jni_env,
                         __array_list_class,
                         "get",
                         "(I)Ljava/lang/Object;"
                 );
        }
}

struct s2_helper *s2_helper_alloc(int min_level, int max_level, int max_cells)
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        __setup();

        struct s2_helper *p     = smalloc(sizeof(struct s2_helper), s2_helper_free);
        jobject obj             = (*__jni_env)->CallStaticObjectMethod(__jni_env, __class,
                                        __static_method_create, min_level, max_level, max_cells);

        p->obj                  = (*__jni_env)->NewGlobalRef(__jni_env, obj);
        (*__jni_env)->DeleteLocalRef(__jni_env, obj);

        p->level                = min_level;
        return p;
}

struct s2_cell_union *s2_helper_get_cell_union(struct s2_helper *p, double lat, double lng, double range)
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        __setup();
        struct s2_lat_lng *latlng = s2_lat_lng_alloc(lat, lng);

        jobject obj = (*__jni_env)->CallObjectMethod(__jni_env, p->obj, __method_get_cell_union, latlng->obj, range);

        s2_lat_lng_free(latlng);

        struct s2_cell_union * scu = s2_cell_union_alloc(obj);
        (*__jni_env)->DeleteLocalRef(__jni_env, obj);

        return scu;
}

struct array *s2_helper_get_cellids(struct s2_helper *p, struct s2_cell_union *u)
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        struct array *a         = array_alloc(sizeof(struct s2_cell_id *), ORDERED);

        jobject obj             = (*__jni_env)->CallObjectMethod(__jni_env, p->obj,
                __method_get_cell_ids, u->obj);

        int size                = (*__jni_env)->CallIntMethod(__jni_env, obj,
                __array_list_method_size);

        int i;
        for_i(i, size) {
                jobject cellid  = (*__jni_env)->CallObjectMethod(__jni_env, obj,
                        __array_list_method_get, i);
                struct s2_cell_id *id = s2_cell_id_alloc(cellid);
                array_push(a, &id);
                (*__jni_env)->DeleteLocalRef(__jni_env, cellid);
        }

        (*__jni_env)->DeleteLocalRef(__jni_env, obj);

        return a;
}

struct s2_cell_id *s2_helper_get_cellid(struct s2_helper *p, double lat, double lng, int level)
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        jobject obj     = (*__jni_env)->CallObjectMethod(__jni_env, p->obj, __method_get_cell_id,
                lat, lng, level);

        struct s2_cell_id * sci = s2_cell_id_alloc(obj);

        (*__jni_env)->DeleteLocalRef(__jni_env, obj);
        return sci;
}

void s2_helper_free(struct s2_helper *p)
{
        JNIEnv *__jni_env = __jni_env_current_thread();
        __setup();

        (*__jni_env)->DeleteGlobalRef(__jni_env, p->obj);
        sfree(p);
}
