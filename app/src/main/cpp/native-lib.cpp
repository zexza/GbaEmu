#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <string>
#include <mutex>
#include <fcntl.h>

extern "C" {
#include <mgba/core/core.h>
#include <mgba-util/vfs.h>
}

#define LOG_TAG "GbaEmuNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static mCore* core = nullptr;
static std::mutex coreMutex;
static uint32_t* videoBuffer = nullptr;
static unsigned videoWidth = 240;
static unsigned videoHeight = 160;

extern "C" JNIEXPORT jboolean JNICALL
Java_com_maxi_gbaemu_EmulatorBridge_loadRom(JNIEnv* env, jobject /* this */, jstring romPath) {
    std::lock_guard<std::mutex> lock(coreMutex);

    const char* path = env->GetStringUTFChars(romPath, nullptr);

    if (core) {
        core->deinit(core);
        core = nullptr;
    }

    struct VFile* vf = VFileOpen(path, O_RDONLY);
    if (!vf) {
        LOGE("No se pudo abrir el archivo: %s", path);
        env->ReleaseStringUTFChars(romPath, path);
        return JNI_FALSE;
    }

    core = mCoreFindVF(vf);
    if (!core) {
        LOGE("No se pudo detectar el tipo de core para: %s", path);
        vf->close(vf);
        env->ReleaseStringUTFChars(romPath, path);
        return JNI_FALSE;
    }

    core->init(core);
    mCoreInitConfig(core, nullptr);

    core->baseVideoSize(core, &videoWidth, &videoHeight);
    if (videoBuffer) {
        free(videoBuffer);
    }
    videoBuffer = (uint32_t*)malloc(videoWidth * videoHeight * sizeof(uint32_t));
    core->setVideoBuffer(core, videoBuffer, videoWidth);

    if (!core->loadROM(core, vf)) {
        LOGE("Fallo al cargar la ROM: %s", path);
        env->ReleaseStringUTFChars(romPath, path);
        return JNI_FALSE;
    }

    core->reset(core);

    LOGI("ROM cargada correctamente: %s (%dx%d)", path, videoWidth, videoHeight);
    env->ReleaseStringUTFChars(romPath, path);
    return JNI_TRUE;
}


extern "C" JNIEXPORT void JNICALL
Java_com_maxi_gbaemu_EmulatorBridge_runFrame(JNIEnv* env, jobject /* this */) {
    std::lock_guard<std::mutex> lock(coreMutex);
    if (core) {
        core->runFrame(core);
    }
}

extern "C" JNIEXPORT jobject JNICALL
Java_com_maxi_gbaemu_EmulatorBridge_getVideoBuffer(JNIEnv* env, jobject /* this */, jobject bitmap) {
    std::lock_guard<std::mutex> lock(coreMutex);
    if (!core || !videoBuffer) return bitmap;

    void* bitmapPixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &bitmapPixels) < 0) {
        return bitmap;
    }

    memcpy(bitmapPixels, videoBuffer, videoWidth * videoHeight * sizeof(uint32_t));

    AndroidBitmap_unlockPixels(env, bitmap);
    return bitmap;
}

extern "C" JNIEXPORT void JNICALL
Java_com_maxi_gbaemu_EmulatorBridge_setKeyState(JNIEnv* env, jobject /* this */, jint key, jboolean pressed) {
    std::lock_guard<std::mutex> lock(coreMutex);
    if (!core) return;

    if (pressed) {
        core->addKeys(core, 1 << key);
    } else {
        core->clearKeys(core, 1 << key);
    }
}

extern "C" JNIEXPORT jint JNICALL
Java_com_maxi_gbaemu_EmulatorBridge_getVideoWidth(JNIEnv* env, jobject /* this */) {
    return videoWidth;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_maxi_gbaemu_EmulatorBridge_getVideoHeight(JNIEnv* env, jobject /* this */) {
    return videoHeight;
}