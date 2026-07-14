#include <falso_jni/FalsoJNI.h>
#include <falso_jni/FalsoJNI_Impl.h>
#include <falso_jni/FalsoJNI_Logger.h>

#include <psp2/kernel/processmgr.h>

/*
 * Java callbacks requested by libcgame.so 1.7.0. Platform services that do
 * not exist on Vita (ads, Facebook, Parse, billing and notifications) are
 * deliberately inert. Core device queries return deterministic values.
 *
 * FalsoJNI identifies methods by name only, matching the API of the selected
 * boilerplate. Zombie Tsunami has one overloaded notification method; both
 * overloads are safely handled by the same no-op callback.
 */

static void method_void_stub(jmethodID id, va_list args) {
    (void)id;
    (void)args;
}

static void method_exit_game(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    sceKernelExitProcess(0);
}

static jobject string_renderer(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, "PowerVR SGX 543MP4+");
}

static jobject string_version(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, "1.7.0");
}

static jobject string_country(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, "US");
}

static jobject string_device(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, "PlayStation Vita");
}

static jobject string_language(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, "en");
}

static jobject string_empty(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, "");
}

static jobject object_null(jmethodID id, va_list args) {
    (void)id; (void)args;
    return NULL;
}

static jboolean boolean_false(jmethodID id, va_list args) {
    (void)id; (void)args;
    return JNI_FALSE;
}

static jint android_sdk(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 19;
}

static jint memory_class(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 256;
}

NameToMethodID nameToMethodId[] = {
    { 1,  "createTextBitmapShadowStroke", METHOD_TYPE_VOID },
    { 2,  "saveRGBABufferToJpegFile", METHOD_TYPE_VOID },
    { 3,  "closeIMEKeyboard", METHOD_TYPE_VOID },
    { 4,  "ExternalRequestRender", METHOD_TYPE_VOID },
    { 5,  "GarbageCollector", METHOD_TYPE_VOID },
    { 6,  "openIMEKeyboard", METHOD_TYPE_VOID },
    { 7,  "SetMultitouchEnabled", METHOD_TYPE_VOID },
    { 8,  "setOpenGLVersion", METHOD_TYPE_VOID },
    { 9,  "getFileDataFromAssets", METHOD_TYPE_VOID },
    { 10, "GetImageInfoFromAsset", METHOD_TYPE_OBJECT },
    { 11, "GetImageInfoFromBuffer", METHOD_TYPE_OBJECT },
    { 12, "getRendererString", METHOD_TYPE_OBJECT },
    { 13, "addproductID", METHOD_TYPE_VOID },
    { 14, "clearLocalNotifications", METHOD_TYPE_VOID },
    { 15, "exitGame", METHOD_TYPE_VOID },
    { 16, "gameAlert", METHOD_TYPE_VOID },
    { 17, "GetAndroidSdkVersion", METHOD_TYPE_INT },
    { 18, "getBundleVersion", METHOD_TYPE_OBJECT },
    { 19, "getCountry", METHOD_TYPE_OBJECT },
    { 20, "getDeviceModel", METHOD_TYPE_OBJECT },
    { 21, "getLanguage", METHOD_TYPE_OBJECT },
    { 22, "getMemoryClass", METHOD_TYPE_INT },
    { 23, "getSkuPrice", METHOD_TYPE_OBJECT },
    { 24, "initFlurry", METHOD_TYPE_VOID },
    { 25, "openUrl", METHOD_TYPE_VOID },
    { 26, "purchase", METHOD_TYPE_VOID },
    { 27, "scheduleLocalNotification", METHOD_TYPE_VOID },
    { 28, "startAccelerometer", METHOD_TYPE_VOID },
    { 29, "stopAccelerometer", METHOD_TYPE_VOID },
    { 30, "threeButtonsAlert", METHOD_TYPE_VOID },
    { 31, "verifyPurchaseAtInit", METHOD_TYPE_VOID },
    { 32, "askPublishPermission", METHOD_TYPE_VOID },
    { 33, "deleteAppRequest", METHOD_TYPE_VOID },
    { 34, "enqueueBlockClearLists", METHOD_TYPE_VOID },
    { 35, "enqueueBlockGetFriendsDetails", METHOD_TYPE_VOID },
    { 36, "enqueueBlockGetPlayerDetails", METHOD_TYPE_VOID },
    { 37, "enqueueGetAppRequests", METHOD_TYPE_VOID },
    { 38, "enqueueSyncDoneNotification", METHOD_TYPE_VOID },
    { 39, "enqueueSyncDoneWaitOneCycle", METHOD_TYPE_VOID },
    { 40, "FBProcessQueueAsync", METHOD_TYPE_VOID },
    { 41, "getUserLikeForID", METHOD_TYPE_VOID },
    { 42, "hasPublishPermission", METHOD_TYPE_BOOLEAN },
    { 43, "isConnected", METHOD_TYPE_BOOLEAN },
    { 44, "login", METHOD_TYPE_VOID },
    { 45, "loginViaCache", METHOD_TYPE_BOOLEAN },
    { 46, "logout", METHOD_TYPE_VOID },
    { 47, "postStatusUpdate", METHOD_TYPE_VOID },
    { 48, "sendRequest", METHOD_TYPE_VOID },
    { 49, "sendRequestSuggestingAndroidFriends", METHOD_TYPE_VOID },
    { 50, "sendScore", METHOD_TYPE_VOID },
    { 51, "sendStory", METHOD_TYPE_VOID },
    { 52, "CloudProcessQueueAsync", METHOD_TYPE_VOID },
    { 53, "EnqueueCleanUp", METHOD_TYPE_VOID },
    { 54, "EnqueueCompareSavedGamesAndPrepareUpload", METHOD_TYPE_VOID },
    { 55, "EnqueueLoadFileFromParse", METHOD_TYPE_VOID },
    { 56, "EnqueueSaveFileToParse", METHOD_TYPE_VOID },
    { 57, "connectTapjoy", METHOD_TYPE_VOID },
    { 58, "getTapjoyPoints", METHOD_TYPE_VOID },
    { 59, "isTapjoyOfferWallVisible", METHOD_TYPE_BOOLEAN },
    { 60, "mopubLoadAd", METHOD_TYPE_VOID },
    { 61, "mopubRefreshAd", METHOD_TYPE_VOID },
    { 62, "mopubShowInterstitial", METHOD_TYPE_VOID },
    { 63, "showTapjoyOfferWall", METHOD_TYPE_VOID },
};

MethodsBoolean methodsBoolean[] = {
    { 42, boolean_false }, { 43, boolean_false }, { 45, boolean_false },
    { 59, boolean_false },
};
MethodsByte methodsByte[] = {};
MethodsChar methodsChar[] = {};
MethodsDouble methodsDouble[] = {};
MethodsFloat methodsFloat[] = {};
MethodsInt methodsInt[] = {
    { 17, android_sdk },
    { 22, memory_class },
};
MethodsLong methodsLong[] = {};
MethodsObject methodsObject[] = {
    { 10, object_null },
    { 11, object_null },
    { 12, string_renderer },
    { 18, string_version },
    { 19, string_country },
    { 20, string_device },
    { 21, string_language },
    { 23, string_empty },
};
MethodsShort methodsShort[] = {};
MethodsVoid methodsVoid[] = {
    { 1, method_void_stub }, { 2, method_void_stub },
    { 3, method_void_stub }, { 4, method_void_stub },
    { 5, method_void_stub }, { 6, method_void_stub },
    { 7, method_void_stub }, { 8, method_void_stub },
    { 9, method_void_stub }, { 13, method_void_stub },
    { 14, method_void_stub }, { 15, method_exit_game },
    { 16, method_void_stub }, { 24, method_void_stub },
    { 25, method_void_stub }, { 26, method_void_stub },
    { 27, method_void_stub }, { 28, method_void_stub },
    { 29, method_void_stub }, { 30, method_void_stub },
    { 31, method_void_stub }, { 32, method_void_stub },
    { 33, method_void_stub }, { 34, method_void_stub },
    { 35, method_void_stub }, { 36, method_void_stub },
    { 37, method_void_stub }, { 38, method_void_stub },
    { 39, method_void_stub }, { 40, method_void_stub },
    { 41, method_void_stub }, { 44, method_void_stub },
    { 46, method_void_stub }, { 47, method_void_stub },
    { 48, method_void_stub }, { 49, method_void_stub },
    { 50, method_void_stub }, { 51, method_void_stub },
    { 52, method_void_stub }, { 53, method_void_stub },
    { 54, method_void_stub }, { 55, method_void_stub },
    { 56, method_void_stub }, { 57, method_void_stub },
    { 58, method_void_stub }, { 60, method_void_stub },
    { 61, method_void_stub }, { 62, method_void_stub },
    { 63, method_void_stub },
};

char WINDOW_SERVICE[] = "window";
const int SDK_INT = 19;

NameToFieldID nameToFieldId[] = {
    { 0, "WINDOW_SERVICE", FIELD_TYPE_OBJECT },
    { 1, "SDK_INT", FIELD_TYPE_INT },
};

FieldsBoolean fieldsBoolean[] = {};
FieldsByte fieldsByte[] = {};
FieldsChar fieldsChar[] = {};
FieldsDouble fieldsDouble[] = {};
FieldsFloat fieldsFloat[] = {};
FieldsInt fieldsInt[] = { { 1, SDK_INT } };
FieldsObject fieldsObject[] = { { 0, WINDOW_SERVICE } };
FieldsLong fieldsLong[] = {};
FieldsShort fieldsShort[] = {};

__FALSOJNI_IMPL_CONTAINER_SIZES
