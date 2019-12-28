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


bool AudioController::performRender(double distance, int audioSource) {

    JNIEnv *env;
    if ((*gs_jvm).AttachCurrentThread(&env, NULL) < 0) {
    } else {
        (*env).CallVoidMethod(mObject, gOnNativeID, int(distance), audioSource);
    }

    return true;
}


void AudioController::init() {
    DebugLog("init()");
    setUpAudio();
}

void AudioController::setUpAudio() {
    DebugLog("setUpAudio");
    RangeFinder *rangeFinder_mic = new RangeFinder(MAX_FRAME_SIZE, NUM_FREQ, START_FREQ,
                                                   FREQ_INTERVAL);
    RangeFinder *rangeFinder_cam = new RangeFinder(MAX_FRAME_SIZE, NUM_FREQ, START_FREQ,
                                                   FREQ_INTERVAL);
    SuperpoweredCPU::setSustainedPerformanceMode(true);

    //*
    new SuperpoweredAndroidAudioIO(AUDIO_SAMPLE_RATE, MAX_FRAME_SIZE, true, true,
                                   (AudioContrllerPerformRender) performRender,
                                   rangeFinder_mic, SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION,
                                   SL_ANDROID_STREAM_MEDIA);
    //*/

    /*
    new SuperpoweredAndroidAudioIO(AUDIO_SAMPLE_RATE, MAX_FRAME_SIZE, true, true,
                                   (AudioContrllerPerformRender) performRender,
                                   rangeFinder_cam, SL_ANDROID_RECORDING_PRESET_CAMCORDER,
                                   SL_ANDROID_STREAM_MEDIA);
    //*/

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
    gOnNativeID = (*env).GetMethodID(clazz, "refresh", "(II)V");
    if (gOnNativeID == NULL) {
        DebugLog("gOnNativeID IS NULL................");
        return;
    }

    (*env).GetJavaVM(&gs_jvm);
    genv = env;
    AudioController audioController;

    audioController.init();

}



