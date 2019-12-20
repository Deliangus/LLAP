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


int64_t getTimeNsec() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t) now.tv_sec * 1000000000LL + now.tv_nsec;
}


bool AudioController::performRender(CallbackData *clientdata, short int *audioInputOutput,
                                    int inNumberFrames, int __unused samplerate) {

    LOGD("performRender 31");
    CallbackData *callbackData = clientdata;
    RangeFinder *rangeFinder = callbackData->rangeFinder;
//    float distancechange;
    LOGD("performRender 32");

    uint64_t startTime = (uint64_t) getTimeNsec();
    LOGD("performRender 35");

//    DebugLog("start   %d",int(startTime/1e6));
//    DebugLog("%d %d %d", int(audioInputOutput[0]),int(audioInputOutput[1]),int(audioInputOutput[2]));




    memcpy((void *) rangeFinder->GetRecDataBuffer(inNumberFrames),
           (void *) audioInputOutput,
           sizeof(int16_t) * inNumberFrames);

    LOGD("performRender 47");

    callbackData->distancechange = rangeFinder->GetDistanceChange();

    LOGD("performRender 51");

    callbackData->distance = (float) (callbackData->distance +
                                      callbackData->distancechange * SPEED_ADJ);

    LOGD("performRender 56");

    if (callbackData->distance < 0) {
        callbackData->distance = 0;
    }
    if (callbackData->distance > 500) {
        callbackData->distance = 500;
    }

    LOGD("performRender 65");

    memcpy((void *) audioInputOutput,
           (void *) rangeFinder->GetPlayBuffer(inNumberFrames),
           sizeof(int16_t) * inNumberFrames);

    LOGD("performRender 71");

    callbackData->mtime = startTime;

    if (fabs(callbackData->distancechange) > 0.06 &&
        (startTime - callbackData->mUIUpdateTime) / 1.0e6 > 10) {
//        DebugLog("distance: %f", callbackData->distance* SPEED_ADJ);
//        env->CallVoidMethod(instance,method,distancechange);
//        callbackData->audioController->env->CallVoidMethod(callbackData->audioController->instance,callbackData->audioController->method,int(callbackData->distance));
//            [[NSNotificationCenter defaultCenter] postNotificationName:@"AudioDisUpdate" object:nil];
//        callbackData->audioController->audiodistance=callbackData->distance;
//        DebugLog("Distance: %f", callbackData->distance);
//        genv->CallVoidMethod(mObject,gOnNativeID,400);
        JNIEnv *env;
        if ((*gs_jvm).AttachCurrentThread(&env, NULL) < 0) {
        } else {
            (*env).CallVoidMethod(mObject, gOnNativeID, int(callbackData->distance));
        }
        callbackData->mUIUpdateTime = startTime;
    }

    return true;
}


void AudioController::init() {
    DebugLog("init()");
    setUpAudio();
}

void AudioController::setUpAudio() {
    DebugLog("setUpAudio");
    _myRangeFinder = new RangeFinder(MAX_FRAME_SIZE, NUM_FREQ, START_FREQ, FREQ_INTERVAL);

    callbackData.rangeFinder = _myRangeFinder;
    SuperpoweredCPU::setSustainedPerformanceMode(true);
    new SuperpoweredAndroidAudioIO(AUDIO_SAMPLE_RATE, MAX_FRAME_SIZE, true, true,
                                   (AudioContrllerPerformRender) performRender,
                                   &callbackData, -1, SL_ANDROID_STREAM_MEDIA, MAX_FRAME_SIZE * 2);

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



