#include "runner_core.h"
#include <PvZ/Cheat.h>
#include <Homura/JniUtils.h>

#include <android/log.h>
#include <jni.h>
#include <cstring>
#include <string>

#define LOG_TAG "HomuraBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace pvz_tv;

static uint32_t call_guest_symbol_0(const char *name) {
    uint32_t fn = RunnerCore::instance().findSymbol(name);
    if (!fn) {
        LOGE("call_guest_symbol_0: symbol '%s' not found!", name);
        return 0;
    }
    uint32_t args[2] = { 0, 0 };
    return RunnerCore::instance().callGuest(fn, args, 2);
}

static uint32_t call_guest_symbol_1(const char *name, uint32_t a0) {
    uint32_t fn = RunnerCore::instance().findSymbol(name);
    if (!fn) {
        LOGE("call_guest_symbol_1: symbol '%s' not found!", name);
        return 0;
    }
    uint32_t args[3] = { 0, 0, a0 };
    return RunnerCore::instance().callGuest(fn, args, 3);
}

static uint32_t call_guest_symbol_2(const char *name, uint32_t a0, uint32_t a1) {
    uint32_t fn = RunnerCore::instance().findSymbol(name);
    if (!fn) {
        LOGE("call_guest_symbol_2: symbol '%s' not found!", name);
        return 0;
    }
    uint32_t args[4] = { 0, 0, a0, a1 };
    return RunnerCore::instance().callGuest(fn, args, 4);
}

static uint32_t call_guest_symbol_3(const char *name, uint32_t a0, uint32_t a1, uint32_t a2) {
    uint32_t fn = RunnerCore::instance().findSymbol(name);
    if (!fn) {
        LOGE("call_guest_symbol_3: symbol '%s' not found!", name);
        return 0;
    }
    uint32_t args[5] = { 0, 0, a0, a1, a2 };
    return RunnerCore::instance().callGuest(fn, args, 5);
}

extern "C" {

// ============================================================================
// com.android.support.Preferences
// ============================================================================

JNIEXPORT void JNICALL Java_com_android_support_Preferences_Changes(
    JNIEnv *env, jclass clazz, jobject con, jint featNum, jstring featName, jint value, jboolean boolean, jstring str) {
    const char *str_c = str ? env->GetStringUTFChars(str, nullptr) : nullptr;
    RunnerCore::instance().setFeature((int)featNum, (int)value, (bool)boolean, str_c);
    if (str_c && str) env->ReleaseStringUTFChars(str, str_c);
}

// ============================================================================
// com.android.support.CkHomuraMenu
// ============================================================================

JNIEXPORT jobjectArray JNICALL Java_com_android_support_CkHomuraMenu_GetFeatureList(JNIEnv *env, jobject thiz) {
    const std::string language = homura::GetLocaleLanguage(env);
    const auto &featureList =
        (language == "vi") ? cheat::lang::vi::featureList :
        (language == "zh") ? cheat::lang::zh_Hans::featureList
                           : cheat::lang::en_US::featureList;

    const jsize featuresCount = static_cast<jsize>(featureList.size());
    jclass strClass = env->FindClass("java/lang/String");
    jobjectArray ret = env->NewObjectArray(featuresCount, strClass, nullptr);
    for (jsize i = 0; i < featuresCount; ++i) {
        jstring s = env->NewStringUTF(featureList[i]);
        env->SetObjectArrayElement(ret, i, s);
        env->DeleteLocalRef(s);
    }
    return ret;
}

JNIEXPORT jobjectArray JNICALL Java_com_android_support_CkHomuraMenu_SettingsList(JNIEnv *env, jobject thiz) {
    const std::string language = homura::GetLocaleLanguage(env);
    const auto &settingsList =
        (language == "vi") ? cheat::lang::vi::settingsList :
        (language == "zh") ? cheat::lang::zh_Hans::settingsList
                           : cheat::lang::en_US::settingsList;

    const jsize settingsCount = static_cast<jsize>(settingsList.size());
    jclass strClass = env->FindClass("java/lang/String");
    jobjectArray ret = env->NewObjectArray(settingsCount, strClass, nullptr);
    for (jsize i = 0; i < settingsCount; ++i) {
        jstring s = env->NewStringUTF(settingsList[i]);
        env->SetObjectArrayElement(ret, i, s);
        env->DeleteLocalRef(s);
    }
    return ret;
}

JNIEXPORT jstring JNICALL Java_com_android_support_CkHomuraMenu_GetCurrentFormation(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF("");
}

// ============================================================================
// com.transmension.mobile.EnhanceActivity
// ============================================================================

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeDisableShop(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeDisableShop");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeEnableManualCollect(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeEnableManualCollect");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeSetHeavyWeaponAngle(JNIEnv *env, jclass clazz, jint i) {
    call_guest_symbol_1("Java_com_transmension_mobile_EnhanceActivity_nativeSetHeavyWeaponAngle", (uint32_t)i);
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeEnableNewOptionsDialog(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeEnableNewOptionsDialog");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeGaoJiPause(JNIEnv *env, jclass clazz, jboolean enable) {
    call_guest_symbol_1("Java_com_transmension_mobile_EnhanceActivity_nativeGaoJiPause", (uint32_t)(enable ? 1 : 0));
}

JNIEXPORT jboolean JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeIsGaoJiPaused(JNIEnv *env, jclass clazz) {
    return (jboolean)(call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeIsGaoJiPaused") != 0);
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeHideCoverLayer(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeHideCoverLayer");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeShowCoolDown(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeShowCoolDown");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeEnableNormalLevelMode(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeEnableNormalLevelMode");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeEnableImitater(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeEnableImitater");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeSendSecondTouch(JNIEnv *env, jclass clazz, jint x, jint y, jint action) {
    call_guest_symbol_3("Java_com_transmension_mobile_EnhanceActivity_nativeSendSecondTouch", (uint32_t)x, (uint32_t)y, (uint32_t)action);
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeEnableNewShovel(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeEnableNewShovel");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeDisableTrashBinZombie(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeDisableTrashBinZombie");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeShowHouse(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeShowHouse");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeUseNewCobCannon(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeUseNewCobCannon");
}

JNIEXPORT jboolean JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeIsOnlineMode(JNIEnv *env, jclass clazz) {
    return (jboolean)(call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeIsOnlineMode") != 0);
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_native1PButtonDown(JNIEnv *env, jclass clazz, jint code) {
    call_guest_symbol_1("Java_com_transmension_mobile_EnhanceActivity_native1PButtonDown", (uint32_t)code);
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_native2PButtonDown(JNIEnv *env, jclass clazz, jint code) {
    call_guest_symbol_1("Java_com_transmension_mobile_EnhanceActivity_native2PButtonDown", (uint32_t)code);
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeSwitchTwoPlayerMode(JNIEnv *env, jclass clazz, jboolean isOn) {
    call_guest_symbol_1("Java_com_transmension_mobile_EnhanceActivity_nativeSwitchTwoPlayerMode", (uint32_t)(isOn ? 1 : 0));
}

JNIEXPORT jboolean JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeIsInGame(JNIEnv *env, jclass clazz) {
    return (jboolean)(call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeIsInGame") != 0);
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeSendButtonEvent(JNIEnv *env, jclass clazz, jboolean isButtonDown, jint buttonCode) {
    call_guest_symbol_2("Java_com_transmension_mobile_EnhanceActivity_nativeSendButtonEvent", (uint32_t)(isButtonDown ? 1 : 0), (uint32_t)buttonCode);
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeAutoFixPosition(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeAutoFixPosition");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeUseXboxMusics(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeUseXboxMusics");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeSeedBankPin(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeSeedBankPin");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeDynamicPreview(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeDynamicPreview");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeEnableOpenSL(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeEnableOpenSL");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeJumpLogo(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeJumpLogo");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativePlayVideo(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativePlayVideo");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeHeavyWeaponAccel(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeHeavyWeaponAccel");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeIntroVideoCompleted(JNIEnv *env, jclass clazz) {
    call_guest_symbol_0("Java_com_transmension_mobile_EnhanceActivity_nativeIntroVideoCompleted");
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeOnReplayImportFinished(
    JNIEnv *env, jclass clazz, jboolean success, jstring message) {
}

JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeOnReplayExportFinished(
    JNIEnv *env, jclass clazz, jboolean success, jstring message) {
}

} // extern "C"
