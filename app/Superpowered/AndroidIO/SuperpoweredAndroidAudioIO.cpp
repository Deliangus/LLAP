#include "SuperpoweredAndroidAudioIO.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <android/log.h>
#include <cmath>


#define NUM_CHANNELS 1
#define NUM_CHANNELS_TWO 2

int64_t getTimeNsec() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t) now.tv_sec * 1000000000LL + now.tv_nsec;
}

bool SuperpoweredAndroidAudioIO::performRender(short int *audioInputOutput, int audioSource) {

    uint64_t startTime = (uint64_t) getTimeNsec();

    memcpy((void *) rangeFinder->GetRecDataBuffer(buffersize),
           (void *) audioInputOutput,
           sizeof(int16_t) * buffersize);

    distancechange = rangeFinder->GetDistanceChange();

    distance = (float) (distance +
                        distancechange * SPEED_ADJ);

    if (distance < 0) {
        distance = 0;
    }
    if (distance > 500) {
        distance = 500;
    }

    memcpy((void *) audioInputOutput,
           (void *) rangeFinder->GetPlayBuffer(buffersize),
           sizeof(int16_t) * buffersize);

    mtime = startTime;

    if (fabs(distancechange) > 0.06 &&
        (startTime - mUIUpdateTime) / 1.0e6 > 10) {
        if (audioSource == 0) {
            audioSource = AUDIOSOURCE_CAM;
        } else {
            audioSource = AUDIOSOURCE_MIC;
        }
        audioContrllerPerformRender(distance, audioSource);
        LOGD("Distance Update %d - %d", inputStreamType, (int) distance);
//        DebugLog("distance: %f", distance* SPEED_ADJ);
//        env->CallVoidMethod(instance,method,distancechange);
//        audioController->env->CallVoidMethod(audioController->instance,audioController->method,int(distance));
//            [[NSNotificationCenter defaultCenter] postNotificationName:@"AudioDisUpdate" object:nil];
//        audioController->audiodistance=distance;
//        DebugLog("Distance: %f", distance);
//        genv->CallVoidMethod(mObject,gOnNativeID,400);
        mUIUpdateTime = startTime;
    }

    return true;
}

// This is called periodically by the input audio queue. Audio input is received from the media server at this point.
static void SuperpoweredAndroidAudioIO_InputCallback(
        SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    SuperpoweredAndroidAudioIO *ctrl = (SuperpoweredAndroidAudioIO *) pContext;

    short int *buffer = ctrl->InputCallBack();
    (*caller)->Enqueue(caller, buffer, (SLuint32) ctrl->buffersize * NUM_CHANNELS * 2);
}

short int *SuperpoweredAndroidAudioIO::InputCallBack() {

    short int *buffer = fifobuffer + writeBufferIndex * bufferStep;
    writeBufferIndex = (writeBufferIndex + 1) % numBuffers;

    if (!hasOutput) { // When there is no audio output configured.
        int buffersAvailable = writeBufferIndex - readBufferIndex;
        if (buffersAvailable < 0)
            buffersAvailable = numBuffers - (readBufferIndex - writeBufferIndex);
        if (buffersAvailable * buffersize >=
            latencySamples) { // if we have enough audio input available

            performRender(fifobuffer + readBufferIndex * bufferStep, 0);

            readBufferIndex = (readBufferIndex + 1) % numBuffers;
        };
    }


    return buffer;
}

// The entire operation is based on two Android Simple Buffer Queues, one for the audio input and one for the audio output.
void SuperpoweredAndroidAudioIO::startQueues() {
    if (started) return;
    started = true;
    if (inputBufferQueue) {
        SLRecordItf recordInterface;
        (*inputBufferQueue)->GetInterface(inputBufferQueue, SL_IID_RECORD, &recordInterface);
        (*recordInterface)->SetRecordState(recordInterface, SL_RECORDSTATE_RECORDING);
    };
    if (outputBufferQueue) {
        SLPlayItf outputPlayInterface;
        (*outputBufferQueue)->GetInterface(outputBufferQueue, SL_IID_PLAY, &outputPlayInterface);
        (*outputPlayInterface)->SetPlayState(outputPlayInterface, SL_PLAYSTATE_PLAYING);
    };
}

// Stopping the Simple Buffer Queues.
void SuperpoweredAndroidAudioIO::stopQueues() {
    if (!started) return;
    started = false;
    if (outputBufferQueue) {
        SLPlayItf outputPlayInterface;
        (*outputBufferQueue)->GetInterface(outputBufferQueue, SL_IID_PLAY, &outputPlayInterface);
        (*outputPlayInterface)->SetPlayState(outputPlayInterface, SL_PLAYSTATE_STOPPED);
    };
    if (inputBufferQueue) {
        SLRecordItf recordInterface;
        (*inputBufferQueue)->GetInterface(inputBufferQueue, SL_IID_RECORD, &recordInterface);
        (*recordInterface)->SetRecordState(recordInterface, SL_RECORDSTATE_STOPPED);
    };
}

short int *SuperpoweredAndroidAudioIO::OutputCallBack() {


    int buffersAvailable = writeBufferIndex - readBufferIndex;
    if (buffersAvailable < 0)
        buffersAvailable = numBuffers - readBufferIndex + writeBufferIndex;
    short int *output = fifobuffer + readBufferIndex * bufferStep;


    if (hasInput) { // If audio input is enabled.

        if (buffersAvailable >= 2) { // if we have enough audio input available

            if (!performRender(output, (readBufferIndex % 2))) {
                performRender(output + bufferStep, ((readBufferIndex+1) % 2));
                memset(output, 0, (size_t) buffersize * NUM_CHANNELS * 2);
                silenceSamples += buffersize;
            } else silenceSamples = 0;

        } else output = NULL; // dropout, not enough audio input
    } else { // If audio input is not enabled.
        short int *audioToGenerate = fifobuffer + writeBufferIndex * bufferStep;

        if (!performRender(audioToGenerate, 0)) {
            memset(audioToGenerate, 0, (size_t) buffersize * NUM_CHANNELS * 2);
            silenceSamples += buffersize;
        } else silenceSamples = 0;

        writeBufferIndex = (writeBufferIndex + 1) % numBuffers;

        if ((buffersAvailable + 1) * buffersize < latencySamples)
            output = NULL; // dropout, not enough audio generated
    };

    if (output) {
        readBufferIndex = (readBufferIndex + 1) % numBuffers;
    };

    return output;


}


// This is called periodically by the output audio queue. Audio for the user should be provided here.
static void SuperpoweredAndroidAudioIO_OutputCallback(
        SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    SuperpoweredAndroidAudioIO *ctrl = (SuperpoweredAndroidAudioIO *) pContext;
    //LOGD("IO_OutputCallBack %p", (char *) caller);

    short int *output = ctrl->OutputCallBack();
    (*caller)->Enqueue(caller, output ? output : ctrl->silence,
                       (SLuint32) ctrl->buffersize * NUM_CHANNELS * 2);

    if (!ctrl->foreground && (ctrl->silenceSamples > ctrl->samplerate)) {
        ctrl->silenceSamples = 0;
        ctrl->stopQueues();
    };
}

SuperpoweredAndroidAudioIO::SuperpoweredAndroidAudioIO(int samplerate, int buffersize,
                                                       bool enableInput, bool enableOutput,
                                                       AudioContrllerPerformRender ACTperformRender,
                                                       RangeFinder *rgF, int inputStreamType,
                                                       int outputStreamType) {
    static const SLboolean requireds[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_FALSE};

    mtime = 0;
    mUIUpdateTime = 0;
    distance = 0;
    distancechange = 0;
    readBufferIndex = 0;
    writeBufferIndex = 0;
    silenceSamples = 0;
    rangeFinder = rgF;
    this->inputStreamType = inputStreamType;
    this->samplerate = samplerate;
    this->buffersize = buffersize;
    audioContrllerPerformRender = ACTperformRender;
    hasInput = enableInput;
    hasOutput = enableOutput;
    foreground = true;
    started = false;
    silence = (short int *) malloc((size_t) buffersize * NUM_CHANNELS * 2);
    memset(silence, 0, (size_t) buffersize * NUM_CHANNELS * 2);
    this->latencySamples = buffersize * 2;

    bufferStep = (buffersize + 64) * NUM_CHANNELS;
    //bufferStep = 96

    size_t fifoBufferSizeBytes = numBuffers * bufferStep * sizeof(short int);
    //fifoBufferSizeBytes = 32*96*sizeof(short int);

    fifobuffer = (short int *) malloc(fifoBufferSizeBytes);
    //fifobuffer = short int[32][96]

    memset(fifobuffer, 0, fifoBufferSizeBytes);

    // Create the OpenSL ES engine.
    slCreateEngine(&openSLEngine, 0, NULL, 0, NULL, NULL);
    (*openSLEngine)->Realize(openSLEngine, SL_BOOLEAN_FALSE);
    SLEngineItf openSLEngineInterface = NULL;
    (*openSLEngine)->GetInterface(openSLEngine, SL_IID_ENGINE, &openSLEngineInterface);
    // Create the output mix.
    (*openSLEngineInterface)->CreateOutputMix(openSLEngineInterface, &outputMix, 0, NULL, NULL);
    (*outputMix)->Realize(outputMix, SL_BOOLEAN_FALSE);
    SLDataLocator_OutputMix outputMixLocator = {SL_DATALOCATOR_OUTPUTMIX, outputMix};

    if (enableInput) { // Create the audio input buffer queue.
        SLDataLocator_IODevice deviceInputLocator = {SL_DATALOCATOR_IODEVICE,
                                                     SL_IODEVICE_AUDIOINPUT,
                                                     SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
        SLDataSource inputSource = {&deviceInputLocator, NULL};
        SLDataLocator_AndroidSimpleBufferQueue inputLocator = {
                SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1};
        SLDataFormat_PCM inputFormat = {SL_DATAFORMAT_PCM, NUM_CHANNELS,
                                        (SLuint32) samplerate * 1000, SL_PCMSAMPLEFORMAT_FIXED_16,
                                        SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_CENTER,
                                        SL_BYTEORDER_LITTLEENDIAN};
        SLDataSink inputSink = {&inputLocator, &inputFormat};
        const SLInterfaceID inputInterfaces[2] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                                  SL_IID_ANDROIDCONFIGURATION};
        (*openSLEngineInterface)->CreateAudioRecorder(openSLEngineInterface, &inputBufferQueue,
                                                      &inputSource, &inputSink, 2, inputInterfaces,
                                                      requireds);

        if (inputStreamType == -1)
            inputStreamType = (int) SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION; // Configure the voice recognition preset which has no signal processing for lower latency.
        if (inputStreamType > -1) {
            SLAndroidConfigurationItf inputConfiguration;
            if ((*inputBufferQueue)->GetInterface(inputBufferQueue, SL_IID_ANDROIDCONFIGURATION,
                                                  &inputConfiguration) == SL_RESULT_SUCCESS) {
                SLuint32 st = (SLuint32) inputStreamType;
                (*inputConfiguration)->SetConfiguration(inputConfiguration,
                                                        SL_ANDROID_KEY_RECORDING_PRESET, &st,
                                                        sizeof(SLuint32));
            };
        };

        (*inputBufferQueue)->Realize(inputBufferQueue, SL_BOOLEAN_FALSE);
    };

    if (enableOutput) { // Create the audio output buffer queue.
        SLDataLocator_AndroidSimpleBufferQueue outputLocator = {
                SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1};
        SLDataFormat_PCM outputFormat = {SL_DATAFORMAT_PCM, NUM_CHANNELS,
                                         (SLuint32) samplerate * 1000, SL_PCMSAMPLEFORMAT_FIXED_16,
                                         SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_CENTER,
                                         SL_BYTEORDER_LITTLEENDIAN};
        SLDataSource outputSource = {&outputLocator, &outputFormat};
        const SLInterfaceID outputInterfaces[2] = {SL_IID_BUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION};
        SLDataSink outputSink = {&outputMixLocator, NULL};
        (*openSLEngineInterface)->CreateAudioPlayer(openSLEngineInterface, &outputBufferQueue,
                                                    &outputSource, &outputSink, 2, outputInterfaces,
                                                    requireds);

        // Configure the stream type.
        if (outputStreamType > -1) {
            SLAndroidConfigurationItf outputConfiguration;
            if ((*outputBufferQueue)->GetInterface(outputBufferQueue, SL_IID_ANDROIDCONFIGURATION,
                                                   &outputConfiguration) == SL_RESULT_SUCCESS) {
                SLint32 st = (SLint32) outputStreamType;
                (*outputConfiguration)->SetConfiguration(outputConfiguration,
                                                         SL_ANDROID_KEY_STREAM_TYPE, &st,
                                                         sizeof(SLint32));
            };
        };

        (*outputBufferQueue)->Realize(outputBufferQueue, SL_BOOLEAN_FALSE);
    };

    if (enableInput) { // Initialize the audio input buffer queue.
        (*inputBufferQueue)->GetInterface(inputBufferQueue, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                          &inputBufferQueueInterface);

        (*inputBufferQueueInterface)->RegisterCallback(inputBufferQueueInterface,
                                                       &SuperpoweredAndroidAudioIO_InputCallback,
                                                       this);
        (*inputBufferQueueInterface)->Enqueue(inputBufferQueueInterface, fifobuffer,
                                              (SLuint32) buffersize * NUM_CHANNELS * 2);
    };

    if (enableOutput) { // Initialize the audio output buffer queue.
        (*outputBufferQueue)->GetInterface(outputBufferQueue, SL_IID_BUFFERQUEUE,
                                           &outputBufferQueueInterface);
        //LOGD("RegisterCallBack %p", (char *) outputBufferQueueInterface);
        (*outputBufferQueueInterface)->RegisterCallback(outputBufferQueueInterface,
                                                        &SuperpoweredAndroidAudioIO_OutputCallback,
                                                        this);
        (*outputBufferQueueInterface)->Enqueue(outputBufferQueueInterface, fifobuffer,
                                               (SLuint32) buffersize * NUM_CHANNELS * 2);
    };

    startQueues();
}

void SuperpoweredAndroidAudioIO::onForeground() {
    foreground = true;
    startQueues();
}

void SuperpoweredAndroidAudioIO::onBackground() {
    foreground = false;
}

void SuperpoweredAndroidAudioIO::start() {
    startQueues();
}

void SuperpoweredAndroidAudioIO::stop() {
    stopQueues();
}

SuperpoweredAndroidAudioIO::~SuperpoweredAndroidAudioIO() {
    stopQueues();
    usleep(200000);
    if (outputBufferQueue) (*outputBufferQueue)->Destroy(outputBufferQueue);
    if (inputBufferQueue) (*inputBufferQueue)->Destroy(inputBufferQueue);
    (*outputMix)->Destroy(outputMix);
    (*openSLEngine)->Destroy(openSLEngine);
    free(fifobuffer);
    free(silence);
}
