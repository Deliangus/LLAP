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

jmethodID func_refresh;
JNIEnv *genv;
jobject mObject;
jclass mClass;
JavaVM *gs_jvm;

AudioController audioController_recognition;
AudioController audioController_communication;
AudioController audioController_unprocessed;
AudioController audioController_cameraorder;
AudioController audioController_output;

bool AudioController::output_enabled = false;
int AudioController::activate_controller = 0;

struct CallbackData {
    RangeFinder *rangeFinder;
    uint64_t mtime;
    uint64_t mUIUpdateTime;
    float distance;
    float distancechange;

    CallbackData() : rangeFinder(NULL), mtime(0), mUIUpdateTime(0), distance(0),
                     distancechange(0) {}
} cd;

int64_t getTimeNsec() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t) now.tv_sec * 1000000000LL + now.tv_nsec;
}


bool AudioController::performRender(void *__unused clientdata, short int *audioInputOutput,
                                    int inNumberFrames, int __unused samplerate, int voice_source) {
    CallbackData *cd = (CallbackData *) clientdata;

//    float distancechange;
    uint64_t startTime = (uint64_t) getTimeNsec();
//    DebugLog("start   %d",int(startTime/1e6));
//    DebugLog("%d %d %d", int(audioInputOutput[0]),int(audioInputOutput[1]),int(audioInputOutput[2]));

    memcpy((void *) cd->rangeFinder->GetRecDataBuffer(inNumberFrames), (void *) audioInputOutput,
           sizeof(int16_t) * inNumberFrames);

    cd->distancechange = cd->rangeFinder->GetDistanceChange();

    cd->distance = (float) (cd->distance + cd->distancechange * SPEED_ADJ);

    if (cd->distance < 0) {
        cd->distance = 0;
    }
    if (cd->distance > 500) {
        cd->distance = 500;
    }


    memcpy((void *) audioInputOutput, (void *) cd->rangeFinder->GetPlayBuffer(inNumberFrames),
           sizeof(int16_t) * inNumberFrames);

    cd->mtime = startTime;

    if (fabs(cd->distancechange) > 0.06 && (startTime - cd->mUIUpdateTime) / 1.0e6 > 10) {
//        DebugLog("distance: %f", cd->distance* SPEED_ADJ);
//        env->CallVoidMethod(instance,method,distancechange);
//        cd->audioController->env->CallVoidMethod(cd->audioController->instance,cd->audioController->method,int(cd->distance));
//            [[NSNotificationCenter defaultCenter] postNotificationName:@"AudioDisUpdate" object:nil];
//        cd->audioController->audiodistance=cd->distance;
//        DebugLog("Distance: %f", cd->distance);
//        genv->CallVoidMethod(mObject,func_refresh,400);
        JNIEnv *env;
        if ((*gs_jvm).AttachCurrentThread(&env, NULL) < 0) {
        } else {
            (*env).CallVoidMethod(mObject, func_refresh, voice_source, int(cd->distance));
        }
        cd->mUIUpdateTime = startTime;
    }

    return true;
}


void AudioController::init(int voice_source, bool enable_input, bool enable_output) {
    DebugLog("init()");
    setUpAudio(voice_source, enable_input, enable_output);
}

void AudioController::stop() {
    superpoweredAndroidAudioIO->stop();
}

void AudioController::setUpAudio(int voice_source, bool enable_input, bool enable_output) {
    DebugLog("setUpAudio");
    _myRangeFinder = new RangeFinder(MAX_FRAME_SIZE, NUM_FREQ, START_FREQ, FREQ_INTERVAL);
    cd.rangeFinder = _myRangeFinder;
    SuperpoweredCPU::setSustainedPerformanceMode(true);
    superpoweredAndroidAudioIO = new SuperpoweredAndroidAudioIO(AUDIO_SAMPLE_RATE, MAX_FRAME_SIZE,
                                                                enable_input, enable_output,
                                                                performRender,
                                                                &cd, -1, SL_ANDROID_STREAM_MEDIA,
                                                                MAX_FRAME_SIZE * 2,
                                                                voice_source);
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_sencs_llap_MainActivity_Begin(JNIEnv *env, jobject instance, jint voice_source) {
    jclass clazz = (*env).FindClass("cn/sencs/llap/MainActivity");
    if (clazz == NULL) {
        DebugLog("clazz IS NULL................");
        return;
    }

    mObject = (jobject) (*env).NewGlobalRef(instance);
    func_refresh = (*env).GetMethodID(clazz, "refresh", "(II)V");
    if (func_refresh == NULL) {
        DebugLog("func_refresh IS NULL................");
        return;
    }

    (*env).GetJavaVM(&gs_jvm);
    genv = env;

    if (!AudioController::output_enabled) {
        audioController_output.init(-1, false, true);
        AudioController::output_enabled = true;
    }

    ///*
    if (voice_source == AudioController::VOICE_RECOGNITION) {
        audioController_recognition.init(AudioController::VOICE_RECOGNITION, true, false);
    } else if (voice_source == AudioController::VOICE_COMMUNICATION) {
        audioController_communication.init(AudioController::VOICE_COMMUNICATION, true, false);
    } else if (voice_source == AudioController::VOICE_CAMCORDER) {
        audioController_cameraorder.init(AudioController::VOICE_CAMCORDER, true, false);
    } else if (voice_source == AudioController::VOICE_UNPROCESSED) {
        audioController_unprocessed.init(AudioController::VOICE_UNPROCESSED, true, false);
    }

    AudioController::activate_controller += 1;
    //*/
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_sencs_llap_MainActivity_Stop(JNIEnv *env, jobject instance, jint voice_source) {
    if (voice_source == AudioController::VOICE_RECOGNITION) {
        audioController_recognition.stop();
    } else if (voice_source == AudioController::VOICE_COMMUNICATION) {
        audioController_communication.stop();
    } else if (voice_source == AudioController::VOICE_CAMCORDER) {
        audioController_cameraorder.stop();
    } else if (voice_source == AudioController::VOICE_UNPROCESSED) {
        audioController_unprocessed.stop();
    }

    AudioController::activate_controller -= 1;

    if (AudioController::activate_controller <= 0) {
        audioController_output.stop();
        AudioController::output_enabled = false;
    }

}

extern "C"
JNIEXPORT int JNICALL
Java_cn_sencs_llap_MainActivity_getVOICE_1RECOGNITION(JNIEnv *env, jobject instance) {
    return AudioController::VOICE_RECOGNITION;
}

extern "C"
JNIEXPORT int JNICALL
Java_cn_sencs_llap_MainActivity_getVOICE_1COMMUNICATION(JNIEnv *env, jobject instance) {
    return AudioController::VOICE_COMMUNICATION;
}

extern "C"
JNIEXPORT int JNICALL
Java_cn_sencs_llap_MainActivity_getVOICE_1UNPROCESSED(JNIEnv *env, jobject instance) {
    return AudioController::VOICE_UNPROCESSED;
}

extern "C"
JNIEXPORT int JNICALL
Java_cn_sencs_llap_MainActivity_getVOICE_1CAMCORDER(JNIEnv *env, jobject instance) {
    return AudioController::VOICE_CAMCORDER;
}