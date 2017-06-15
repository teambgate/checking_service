#include <jni.h>
#include <cherry/stdio.h>
#include <cherry/list.h>
#include <cherry/memory.h>
#include <cherry/map.h>
#include <cherry/array.h>
#include <cherry/string.h>
#include <cherry/bytes.h>
#include <cherry/math/math.h>
#include <native_ui/view.h>
#include <native_ui/align.h>
#include <zip.h>
#include <native_ui/parser.h>
#include <native_ui/native_ui_manager.h>
#include <native_ui/action.h>
#include <native_ui/test.h>

#include <common/request.h>
#include <cherry/unistd.h>
#include <smartfox/data.h>
#include <common/key.h>
#include <common/command.h>
#include <locale.h>
#include <time.h>

extern "C" {
zip *APKArchive;
struct string *__local_directory__ = NULL;

JNIEnv  *__jni_env;

jobject __activity;

static struct native_view *root;
}

extern "C" {
    static void __test_animation(struct native_view *view)
    {
        union vec3 r;
        r.x = 0;
        r.y = 0;
        r.z = 360;
        native_view_run_action(view, native_ui_action_sequence(
//                native_view_rotate_by(view, r, 5.0f, NATIVE_UI_EASE_QUARTIC_IN_OUT, 0),
                native_view_move_by(view, (union vec2){300, 0}, 5.0f, NATIVE_UI_EASE_QUADRATIC_IN_OUT, 0),
                NULL
        ), NULL);
    }

    static void callback(void *p, struct smart_object *data)
    {
        struct string *j = smart_object_to_json(data);
        debug("native ui : receive : %s\n",j->ptr);
        string_free(j);
    }

    static void test_search()
    {
        struct cs_requester *p  = cs_requester_alloc();
        int ret = cs_requester_connect(p, "192.168.1.13", 9898);
        debug("native ui init requester : %d\n", ret);

        {
            struct smart_object *obj = smart_object_from_json_file("res/request.json", FILE_INNER);
            struct string *request = smart_object_get_string(obj, (void*)"request", sizeof("request") - 1, SMART_GET_REPLACE_IF_WRONG_TYPE);
            struct string *path = smart_object_get_string(obj, (void*)"path", sizeof("path") - 1, SMART_GET_REPLACE_IF_WRONG_TYPE);
            struct smart_object *objdata = smart_object_get_object(obj, (void*)"data", sizeof("data") - 1, SMART_GET_REPLACE_IF_WRONG_TYPE);

            struct smart_object *data = smart_object_alloc();
            smart_object_set_string(data, qskey(&__key_version__), qlkey("1"));

            if(strcmp(request->ptr, "post") == 0) {
                smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_post__));
            } else if(strcmp(request->ptr, "get") == 0) {
                smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_get__));
            } else if(strcmp(request->ptr, "put") == 0) {
                smart_object_set_string(data, qskey(&__key_cmd__), qskey(&__cmd_put__));
            }


            smart_object_set_string(data, qskey(&__key_pass__), qlkey("123456"));

            smart_object_set_string(data, qskey(&__key_path__), qskey(path));

            struct string *json = smart_object_to_json(objdata);
            int counter = 0;
            struct smart_object *d = smart_object_from_json(json->ptr, json->len, &counter);
            string_free(json);
            // struct smart_object *obj = smart_object_alloc();
            // smart_object_set_string(obj, qskey(&__key_name__), qlkey("Johan"));
            smart_object_set_object(data, qskey(&__key_data__), d);
            cs_request_alloc(p, data, callback, p);
        }
    }
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_apple_myapplication_MainActivity_initNativeJNI(
        JNIEnv *env,
        jobject /* this */,
        jstring apkPath, jstring local_directory, jobject activity) {
    /*
     * register assets
     */
    jboolean isCopy;
    APKArchive = zip_open(env->GetStringUTFChars(apkPath, &isCopy), 0, NULL);
    const char *tmp = env->GetStringUTFChars(local_directory, &isCopy);
    __local_directory__ = string_alloc_chars((char*)tmp, strlen(tmp));

    __activity = __jni_env->NewGlobalRef(activity);

    //native_ui_test();

    /*
     * create root
     */
    root = native_view_alloc();
    native_view_set_layout_type(root, NATIVE_UI_LAYOUT_RELATIVE);

    native_view_set_user_interaction_enabled(root, 1);

    struct native_view_parser *parser = native_view_parser_alloc();
    native_view_parser_parse_file(parser, "res/layout/root.xml");

    struct native_view *view = (struct native_view *)
            ((char *)parser->view.next - offsetof(struct native_view, parser));

    native_view_add_child(root, view);

   // test_search();

    return root->ptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_apple_myapplication_MainActivity_onResizeJNI(
        JNIEnv *env,
        jobject /* this */,
        int width, int height) {
    native_view_set_size(root, (union vec2){(float)width, (float)height});
    native_view_set_position(root, (union vec2){root->size.width / 2, root->size.height/2});
    native_view_update_layout(root);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_apple_myapplication_MainActivity_onLoopJNI(
        JNIEnv *env,
        jobject /* this */) {
    native_ui_manager_update(native_ui_manager_shared(), 1.0f / 60);
}


jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    if (vm->GetEnv(reinterpret_cast<void**>(&__jni_env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_6;
}
