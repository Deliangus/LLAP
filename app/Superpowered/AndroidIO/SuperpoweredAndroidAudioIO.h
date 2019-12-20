#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <clocale>
#include "../../src/main/cpp/RangeFinder.h"

#ifndef Header_SuperpoweredAndroidAudioIO

#define Header_SuperpoweredAndroidAudioIO

#define LOG_TAG "System.out"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct SuperpoweredAndroidAudioIOInternals;

struct CallbackData {
    RangeFinder *rangeFinder;
    uint64_t mtime;
    uint64_t mUIUpdateTime;
    float distance;
    float distancechange;

    CallbackData() : rangeFinder(NULL), mtime(0), mUIUpdateTime(0), distance(0),
                     distancechange(0) {}
};

typedef bool (*AudioContrllerPerformRender)(void *clientdata, short int *audioIO,
                                            int numberOfSamples,
                                            int samplerate);

/**
 @brief Easy handling of OpenSL ES audio input and/or output.
 */
class SuperpoweredAndroidAudioIO {
public:
/**
 @brief Creates an audio I/O instance. Audio input and/or output immediately starts after calling this.

 @param samplerate The requested sample rate in Hz.
 @param buffersize The requested buffer size (number of samples).
 @param enableInput Enable audio input.
 @param enableOutput Enable audio output.
 @param callback The audio processing callback function to call periodically.
 @param clientdata A custom pointer the callback receives.
 @param inputStreamType OpenSL ES stream type, such as SL_ANDROID_RECORDING_PRESET_GENERIC. -1 means default. SLES/OpenSLES_AndroidConfiguration.h has them.
 @param outputStreamType OpenSL ES stream type, such as SL_ANDROID_STREAM_MEDIA or SL_ANDROID_STREAM_VOICE. -1 means default. SLES/OpenSLES_AndroidConfiguration.h has them.
 @param latencySamples How many samples to have in the internal fifo buffer minimum. Works only when both input and output are enabled. Might help if you have many dropouts.
 */
    SuperpoweredAndroidAudioIO(int samplerate, int buffersize, bool enableInput,
                               bool enableOutput, AudioContrllerPerformRender performRender,
                               CallbackData *callbackData, int inputStreamType = -1,
                               int outputStreamType = -1, int latencySamples = 0);

    ~SuperpoweredAndroidAudioIO();

/*
 @brief Call this in the main activity's onResume() method.
 
  Calling this is important if you'd like to save battery. When there is no audio playing and the app goes to the background, it will automatically stop audio input and/or output.
*/
    void onForeground();

/*
 @brief Call this in the main activity's onPause() method.
 
 Calling this is important if you'd like to save battery. When there is no audio playing and the app goes to the background, it will automatically stop audio input and/or output.
*/
    void onBackground();

/*
 @brief Starts audio input and/or output.
*/
    void start();

/*
 @brief Stops audio input and/or output.
*/
    void stop();

public:
    int readBufferIndex;
    int writeBufferIndex;
    int silenceSamples;
    int samplerate;
    int buffersize;
    CallbackData *callbackData;
    bool hasInput;
    bool hasOutput;
    bool foreground;
    AudioContrllerPerformRender performRender;
    bool started;
    short int *silence;
    int latencySamples;
    int numBuffers;
    int bufferStep;
    short int *fifobuffer;
    SLObjectItf openSLEngine;
    SLObjectItf outputMix;
    SLObjectItf outputBufferQueue;
    SLObjectItf inputBufferQueue;
    SLAndroidSimpleBufferQueueItf outputBufferQueueInterface, inputBufferQueueInterface;


    void stopQueues();

    short int *InputCallBack();

    short int *OutputCallBack();

    void startQueues();

private:
    SuperpoweredAndroidAudioIO(const SuperpoweredAndroidAudioIO &);

    SuperpoweredAndroidAudioIO &operator=(const SuperpoweredAndroidAudioIO &);
};

#endif
