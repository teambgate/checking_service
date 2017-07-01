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
#include <native_ui/manager.h>
#include <native_ui/action.h>
#import <native_ui/view.h>
#import <native_ui/view_controller.h>
#import <native_ui/align.h>
#import <native_ui/action.h>
#import <native_ui/touch_handle.h>

#include <common/request.h>
#include <cherry/unistd.h>
#include <smartfox/data.h>
#include <common/key.h>
#include <common/command.h>
#include <locale.h>
#include <time.h>

#import <checking_client/controller_utils.h>

extern "C" {
    zip *APKArchive;
    struct string *__local_directory__ = NULL;

    JNIEnv  *__jni_env;
    JavaVM*         __jvm;
    struct map *    __jni_env_map;

    jobject __activity;

    static struct nview *root;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_bgate_nativeui_ActivityHelper_initNativeJNI(
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
/*
     * register view controller allocator
     */
    nexec_set_fnf(cl_nexec_alloc);

    /*
     * create root
     */
    root = nview_alloc();
    nview_set_layout_type(root, NATIVE_UI_LAYOUT_RELATIVE);

    nview_set_user_interaction_enabled(root, 1);

    struct nparser *parser = nparser_alloc();
    nparser_parse_file(parser, "res/layout/root.xml", NULL);

    struct nview *view = (struct nview *)
            ((char *)parser->view.next - offsetof(struct nview, parser));

    nview_add_child(root, view);

    return root->ptr;
}

extern "C" {
    static nview *__search_touch_view(struct nview *p, float sx, float sy)
    {
        struct nview *ret = NULL;
        struct list_head *head;
        list_back_each(head, &p->children) {
            struct nview *c = (struct nview *)
                    ((char *)head - offsetof(struct nview, head));

            ret = __search_touch_view(c, sx, sy);
            if(ret) break;
        }
        if(ret) return ret;

        union vec2 tl = nview_get_screen_pos(p, (union vec2){0, 0});
        union vec2 pt = vec2_sub((union vec2){sx, sy}, tl);

        if(pt.x >= 0 && pt.x <= p->size.width
           && pt.y >= 0 && pt.y <= p->size.height
                && p->user_interaction_enabled) {
            struct nview *v = p;
            ret = p;
            while(v) {
                if( ! v->visible) {
                    ret = NULL;
                    break;
                }
                v = v->parent;
            }
        }

        return ret;
    }
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_bgate_nativeui_CustomSharedView_searchTouchViewJNI(
        JNIEnv *env,
        jobject /* this */,
        float sx, float sy) {

    struct nview *p = __search_touch_view(root, sx, sy);
    if(p) return p->ptr;

    return NULL;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_bgate_nativeui_ActivityHelper_onResizeJNI(
        JNIEnv *env,
        jobject /* this */,
        int width, int height) {
    nview_set_size(root, (union vec2){(float)width, (float)height});
    nview_set_position(root, (union vec2){root->size.width / 2, root->size.height/2});
    nview_update_layout(root);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_bgate_nativeui_ActivityHelper_onLoopJNI(
        JNIEnv *env,
        jobject /* this */) {
    nmanager_update(nmanager_shared(), 1.0f / 60);
}


jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    __jvm = vm;
    __jni_env_map = map_alloc(sizeof(JNIEnv *));

    if (vm->GetEnv(reinterpret_cast<void**>(&__jni_env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_6;
}
