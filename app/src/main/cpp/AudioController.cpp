//
// Created by sensj on 2017/12/18.
//

#include "AudioController.h"
#include <sys/time.h>
#include <cstring>
#include <cmath>
#include <time.h>
#include <cstdlib>
#include <unistd.h>
#include <thread>

jmethodID gOnNativeID;
JNIEnv *genv;
jobject mObject;
jclass mClass;
JavaVM *gs_jvm;


bool AudioController::performRender(double distance) {

    if (distance < 0) {
        distance = 0;
    }
    if (distance > 500) {
        distance = 500;
    }

    JNIEnv *env;
    if ((*gs_jvm).AttachCurrentThread(&env, NULL) < 0) {
    } else {
        (*env).CallVoidMethod(mObject, gOnNativeID, int(distance));
    }

    return true;
}


void AudioController::init() {
    DebugLog("init()");
    setUpAudio();
}

void AudioController::setUpAudio() {
    DebugLog("setUpAudio");
    RangeFinder *rangeFinder = new RangeFinder(MAX_FRAME_SIZE, NUM_FREQ, START_FREQ, FREQ_INTERVAL);

    SuperpoweredCPU::setSustainedPerformanceMode(true);

    new SuperpoweredAndroidAudioIO(AUDIO_SAMPLE_RATE, MAX_FRAME_SIZE, true, true,
                                   (AudioContrllerPerformRender) performRender,
                                   rangeFinder, SL_ANDROID_RECORDING_PRESET_CAMCORDER,
                                   SL_ANDROID_STREAM_MEDIA, MAX_FRAME_SIZE * 2);

    new SuperpoweredAndroidAudioIO(AUDIO_SAMPLE_RATE, MAX_FRAME_SIZE, true, false,
                                   (AudioContrllerPerformRender) performRender,
                                   rangeFinder, SL_ANDROID_RECORDING_PRESET_CAMCORDER,
                                   SL_ANDROID_STREAM_MEDIA, MAX_FRAME_SIZE * 2);

}

extern "C"
JNIEXPORT void JNICALL
Java_cn_sencs_llap_MainActivity_Begin(JNIEnv *env, jobject instance) {
    jclass clazz = (*env).FindClass("cn/sencs/llap/MainActivity");
    if (clazz == NULL) {
        DebugLog("clazz IS NULL................");
        return;
    }

    mObject = (jobject) (*env).NewGlobalRef(instance);
    gOnNativeID = (*env).GetMethodID(clazz, "refresh", "(I)V");
    if (gOnNativeID == NULL) {
        DebugLog("gOnNativeID IS NULL................");
        return;
    }

    (*env).GetJavaVM(&gs_jvm);
    genv = env;
    AudioController audioController;

    audioController.init();

}



