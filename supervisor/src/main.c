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
#include <s2/s2_cell_union.h>
#include <s2/s2_lat_lng.h>
#include <s2/s2_cell_id.h>
#include <s2/s2_point.h>

#include <cherry/array.h>
#include <cherry/map.h>
#include <cherry/stdio.h>
#include <cherry/memory.h>
#include <cherry/string.h>
#include <cherry/stdlib.h>
#include <cherry/unistd.h>
#include <cherry/ctype.h>
#include <cherry/time.h>

#include <smartfox/data.h>
#include <supervisor/supervisor.h>

#include <pthread.h>

static JavaVM*  jvm;
JNIEnv*         __jni_env;

static struct supervisor *s = NULL;

static void __setup_jni()
{
        JNIEnv*                 env;
        JavaVMInitArgs          vm_args;
        JavaVMOption            options[10];

        int oi = 0;
        options[oi++].optionString = "-Djava.class.path=./s2.jar";

        vm_args.version                 = JNI_VERSION_1_4;
        vm_args.options                 = options;
        vm_args.nOptions                = oi;
        vm_args.ignoreUnrecognized      = 1;

        JNI_CreateJavaVM( &jvm, (void**)&env, &vm_args );

        __jni_env = env;
}

static struct sfs_object *__parse_input(char *line, int len)
{
        int counter             = 0;
        int start               = 0;
        int end                 = 0;
        struct sfs_object *p    = sfs_object_alloc();
        struct string *key      = string_alloc(0);
        struct string *val      = string_alloc(0);

#define increase_count() \
        counter++;      \
        if(counter == len - 1) goto end;

find_cmd:;
        if(isspace(line[counter])) {
                increase_count()
                goto find_cmd;
        }
        start   = counter;
        end     = start;
        increase_count()
read_cmd:;
        if( ! isspace(line[counter])) {
                end = counter;
                increase_count()
                goto read_cmd;
        }
        sfs_object_set_string(p, qlkey("cmd"), line + start, end - start + 1);

find_argument:;
        if(line[counter] != '-') {
                increase_count();
                goto find_argument;
        }
        increase_count()
        start   = counter;
        end     = start;
        increase_count()
read_argument:;
        if( ! isspace(line[counter])) {
                end = counter;
                increase_count()
                goto read_argument;
        }
        key->len = 0;
        string_cat(key, line + start, end - start + 1);

find_value:;
        if(isspace(line[counter])) {
                increase_count();
                goto find_value;
        }
        start   = counter;
        end     = start;
        increase_count()
read_value:;
        if(line[counter] != '-') {
                end = counter;
                increase_count()
                goto read_value;
        }
        val->len = 0;
        string_cat(val, line + start, end - start + 1);
        sfs_object_set_string(p, key->ptr, key->len, val->ptr, val->len);

        goto read_argument;

end:;
        string_free(val);
        string_free(key);
        return p;
}

static void __read_input(void *d)
{
#define BUFFER_LEN 1024
get_line:;
        char line[BUFFER_LEN];
        int counter = 0;

        if(fgets(line, BUFFER_LEN, stdin) != NULL) {
                struct sfs_object *com = __parse_input(line, BUFFER_LEN);

                struct string *cmd = sfs_object_get_string(com, qlkey("cmd"), SFS_GET_REPLACE_IF_WRONG_TYPE);

                sfs_object_free(com);
        }

        goto get_line;
}

int main( int argc, char** argv )
{
        srand ( time(NULL) );

        __setup_jni();

        pthread_t tid[1];
        pthread_create(&tid[0], NULL, (void*(*)(void*))__read_input, (void*)NULL);

// begin:;
        s = supervisor_alloc();

        supervisor_start(s);

        supervisor_free(s);
        cache_free();
        dim_memory();

        // goto begin;

        (*jvm)->DestroyJavaVM( jvm );
        return 0;
}
