#include "utils/init.h"
#include "utils/dialog.h"
#include "utils/glutil.h"
#include "utils/logger.h"

#include <psp2/kernel/threadmgr.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>

#include "reimpl/asset_manager.h"
#include "reimpl/controls.h"

#define SCREEN_WIDTH  960
#define SCREEN_HEIGHT 544
#define DISPLAY_DPI   220

int _newlib_heap_size_user = 256 * 1024 * 1024;

#ifdef USE_SCELIBC_IO
int sceLibcHeapSize = 4 * 1024 * 1024;
#endif

so_module so_mod;
so_module so_fmodex_mod;
so_module so_fmodevent_mod;

typedef void (*native_void_fn)(JNIEnv *, jobject);
typedef void (*native_init_fn)(JNIEnv *, jobject, jobject, jint, jint);
typedef void (*native_resize_fn)(JNIEnv *, jobject, jint, jint);
typedef void (*native_touch_fn)(JNIEnv *, jobject, jint, jint, jfloat, jfloat);
typedef void (*native_path_fn)(JNIEnv *, jobject, jstring);
typedef void (*native_dpi_fn)(JNIEnv *, jobject, jint);
typedef jboolean (*native_back_fn)(JNIEnv *, jobject);
typedef void (*game_alloc_renderer_fn)(void);

static native_void_fn native_render;
static native_void_fn native_pause;
static native_void_fn native_resume;
static native_void_fn native_button_1;
static native_void_fn native_button_2;
static native_void_fn native_button_3;
static native_touch_fn native_touch;
static native_back_fn native_back;
static int app_paused;

static uintptr_t required_symbol(const char *name) {
    uintptr_t address = so_symbol(&so_mod, name);
    if (!address) {
        fatal_error("Required libcgame.so symbol is missing: %s", name);
    }
    return address;
}

static void bind_game_api(void) {
    native_render = (native_void_fn)required_symbol(
        "Java_net_mobigame_artemis_GameViewRenderer_nativeRender");
    native_touch = (native_touch_fn)required_symbol(
        "Java_net_mobigame_artemis_GameGLSurfaceView_nativeTouch");
    native_pause = (native_void_fn)required_symbol(
        "Java_net_mobigame_artemis_MobiActivity_nativeOnPause");
    native_resume = (native_void_fn)required_symbol(
        "Java_net_mobigame_artemis_MobiActivity_nativeOnResume");
    native_back = (native_back_fn)required_symbol(
        "Java_net_mobigame_artemis_MobiActivity_nativeOnBackPressed");
    native_button_1 = (native_void_fn)required_symbol(
        "Java_net_mobigame_artemis_MobiActivity_nativeOnButton1Pressed");
    native_button_2 = (native_void_fn)required_symbol(
        "Java_net_mobigame_artemis_MobiActivity_nativeOnButton2Pressed");
    native_button_3 = (native_void_fn)required_symbol(
        "Java_net_mobigame_artemis_MobiActivity_nativeOnButton3Pressed");
}

static void start_game(void) {
    game_alloc_renderer_fn game_alloc_renderer = (game_alloc_renderer_fn)required_symbol(
        "_Z17GameAllocRendererv");
    native_init_fn native_init = (native_init_fn)required_symbol(
        "Java_net_mobigame_artemis_GameViewRenderer_nativeInit");
    native_resize_fn native_resize = (native_resize_fn)required_symbol(
        "Java_net_mobigame_artemis_GameViewRenderer_nativeResize");
    native_path_fn native_set_files = (native_path_fn)required_symbol(
        "Java_net_mobigame_artemis_MobiActivity_nativeSetFilesDir");
    native_path_fn native_set_external_files = (native_path_fn)required_symbol(
        "Java_net_mobigame_artemis_MobiActivity_nativeSetExternalFilesDir");
    native_dpi_fn native_set_dpi = (native_dpi_fn)required_symbol(
        "Java_net_mobigame_artemis_MobiActivity_nativeSetDisplayDpi");

    jstring data_path = jni->NewStringUTF(&jni, DATA_PATH);
    native_set_files(&jni, NULL, data_path);
    native_set_external_files(&jni, NULL, data_path);
    native_set_dpi(&jni, NULL, DISPLAY_DPI);

    // AAssetManager_open ignores the identity of this Java-side token and
    // reads from DATA_PATH/assets, but a stable non-null object is still
    // supplied to match the original Java renderer call.
    jobject assets = (jobject)AAssetManager_create();
    // The Android Java bootstrap allocates the engine renderer before it calls
    // nativeInit. Without this step, GameEngineInit reaches
    // CRenderer::GetInstance(), receives NULL and aborts at libcgame+0xDC276.
    game_alloc_renderer();
    native_init(&jni, NULL, assets, SCREEN_WIDTH, SCREEN_HEIGHT);
    native_resize(&jni, NULL, SCREEN_WIDTH, SCREEN_HEIGHT);
    native_resume(&jni, NULL);
    l_success("Zombie Tsunami native renderer initialized.");
}

int main(void) {
    soloader_init_all();
    bind_game_api();

    // libcgame.so does not export JNI_OnLoad in version 1.7.0. Calling the
    // boilerplate's sample JNI_OnLoad path would therefore jump to NULL.
    gl_init();
    start_game();

    while (1) {
        controls_poll();
        if (!app_paused) {
            native_render(&jni, NULL);
            gl_swap();
        } else {
            sceKernelDelayThread(10000);
        }
    }

    return 0;
}

void controls_handler_key(int32_t keycode, ControlsAction action) {
    if (action != CONTROLS_ACTION_DOWN) return;

    switch (keycode) {
        case AKEYCODE_BUTTON_A:
            native_button_1(&jni, NULL);
            break;
        case AKEYCODE_BUTTON_X:
            native_button_2(&jni, NULL);
            break;
        case AKEYCODE_BUTTON_Y:
            native_button_3(&jni, NULL);
            break;
        case AKEYCODE_BUTTON_B:
        case AKEYCODE_BACK:
            native_back(&jni, NULL);
            break;
        case AKEYCODE_BUTTON_START:
            app_paused = !app_paused;
            if (app_paused) native_pause(&jni, NULL);
            else native_resume(&jni, NULL);
            break;
        default:
            break;
    }
}

void controls_handler_touch(int32_t id, float x, float y, ControlsAction action) {
    // Android MotionEvent constants: DOWN=0, UP=1, MOVE=2. The Java wrapper
    // calls nativeTouch(pointerId, action, x, y). The native routine stores
    // r2 as the pointer id and dispatches on r3 (0..3) as the action.
    jint android_action = action == CONTROLS_ACTION_DOWN ? 0 :
                          action == CONTROLS_ACTION_UP ? 1 : 2;
    native_touch(&jni, NULL, id, android_action, x, y);
}

void controls_handler_analog(ControlsStickId which, float x, float y, ControlsAction action) {
    (void)which;
    (void)x;
    (void)y;
    (void)action;
}
