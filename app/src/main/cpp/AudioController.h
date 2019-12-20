//
// Created by sensj on 2017/12/18.
//

#ifndef MYAPPLICATION_AUDIOCONTROLLER_H
#define MYAPPLICATION_AUDIOCONTROLLER_H

#include <AndroidIO/SuperpoweredAndroidAudioIO.h>
#include <SuperpoweredSimple.h>
#include <SuperpoweredCPU.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <android/log.h>

#include <android/log.h>
#include <jni.h>
#include <clocale>
//Record sample rate
#define AUDIO_SAMPLE_RATE   48000
//Start audio frequency
#define START_FREQ          17500.0
//Frequency interval
#define FREQ_INTERVAL       350.0
//Number of frequency
#define NUM_FREQ            10

//Number of frame size
#define MAX_FRAME_SIZE      1920

//Speed adjust
#define SPEED_ADJ           1.1
#define DebugLog(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)



class AudioController {
    CallbackData callbackData;
public:
    RangeFinder *_myRangeFinder;

    void init();

    void setUpAudio();

    static bool
    performRender(CallbackData *__unused, short int *audioInputOutput, int numberOfSamples,
                  int);
};


#endif //MYAPPLICATION_AUDIOCONTROLLER_H
