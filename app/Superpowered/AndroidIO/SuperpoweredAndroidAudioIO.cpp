#include "SuperpoweredAndroidAudioIO.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <android/log.h>


#define NUM_CHANNELS 1

short int *SuperpoweredAndroidAudioIO::InputCallBack() {

    LOGD("InputCallBack - 19 %d %d", writeBufferIndex, readBufferIndex);

    short int *buffer = fifobuffer + writeBufferIndex * bufferStep;
    writeBufferIndex = (writeBufferIndex + 1) % numBuffers;

    LOGD("InputCallBack - 25 %d %d", writeBufferIndex, readBufferIndex);

    if (!hasOutput) { // When there is no audio output configured.
        int buffersAvailable = writeBufferIndex - readBufferIndex;
        if (buffersAvailable < 0)
            buffersAvailable = numBuffers - (readBufferIndex - writeBufferIndex);
        if (buffersAvailable * buffersize >=
            latencySamples) { // if we have enough audio input available

            LOGD("InputCallBack - 34");

            performRender(callbackData,
                          fifobuffer + readBufferIndex * bufferStep,
                          buffersize, samplerate);

            LOGD("InputCallBack - 41");

            readBufferIndex = (readBufferIndex + 1) % numBuffers;
        };
    }

    LOGD("InputCallBack - 49");

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

    LOGD("OutputCallBack - 87 %d %d", writeBufferIndex, readBufferIndex);

    int buffersAvailable = writeBufferIndex - readBufferIndex;
    if (buffersAvailable < 0)
        buffersAvailable = numBuffers - readBufferIndex + writeBufferIndex;
    short int *output = fifobuffer + readBufferIndex * bufferStep;

    LOGD("OutputCallBack - 94 %d %d", writeBufferIndex, readBufferIndex);

    if (hasInput) { // If audio input is enabled.
        LOGD("OutputCallBack - 99 %d %d %d", buffersAvailable, buffersize, latencySamples);
        if (buffersAvailable * buffersize >=
            latencySamples) { // if we have enough audio input available

            LOGD("OutputCallBack - 102");
            if (callbackData == nullptr) {
                LOGD("callbackData Null");
            }
            LOGD("OutputCallBack - 102");
            if (output == nullptr) {
                LOGD("output Null");
            }
            LOGD("OutputCallBack - 106");


            if (!performRender(callbackData, output, buffersize, samplerate)) {
                memset(output, 0, (size_t) buffersize * NUM_CHANNELS * 2);
                silenceSamples += buffersize;
            } else silenceSamples = 0;

            LOGD("OutputCallBack - 109");

        } else output = NULL; // dropout, not enough audio input
    } else { // If audio input is not enabled.
        short int *audioToGenerate = fifobuffer + writeBufferIndex * bufferStep;

        LOGD("OutputCallBack - 115");

        if (!performRender(callbackData, audioToGenerate, buffersize,
                           samplerate)) {
            memset(audioToGenerate, 0, (size_t) buffersize * NUM_CHANNELS * 2);
            silenceSamples += buffersize;
        } else silenceSamples = 0;

        LOGD("OutputCallBack - 123");

        writeBufferIndex = (writeBufferIndex + 1) % numBuffers;

        if ((buffersAvailable + 1) * buffersize < latencySamples)
            output = NULL; // dropout, not enough audio generated
    };

    LOGD("OutputCallBack - 130");

    if (output) {
        readBufferIndex = (readBufferIndex + 1) % numBuffers;
    };

    return output;


}

// This is called periodically by the input audio queue. Audio input is received from the media server at this point.
static void SuperpoweredAndroidAudioIO_InputCallback(
        SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    SuperpoweredAndroidAudioIO *ctrl = (SuperpoweredAndroidAudioIO *) pContext;

    short int *buffer = ctrl->InputCallBack();
    (*caller)->Enqueue(caller, buffer, (SLuint32) ctrl->buffersize * NUM_CHANNELS * 2);
}

// This is called periodically by the output audio queue. Audio for the user should be provided here.
static void SuperpoweredAndroidAudioIO_OutputCallback(
        SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    SuperpoweredAndroidAudioIO *ctrl = (SuperpoweredAndroidAudioIO *) pContext;

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
                                                       AudioContrllerPerformRender performRender,
                                                       CallbackData *callbackData,
                                                       int inputStreamType,
                                                       int outputStreamType, int latencySamples) {
    static const SLboolean requireds[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_FALSE};

    readBufferIndex = 0;
    writeBufferIndex = 0;
    silenceSamples = 0;
    this->samplerate = samplerate;
    this->buffersize = buffersize;
    this->callbackData = callbackData;
    this->performRender = performRender;
    hasInput = enableInput;
    hasOutput = enableOutput;
    foreground = true;
    started = false;
    silence = (short int *) malloc((size_t) buffersize * NUM_CHANNELS * 2);
    memset(silence, 0, (size_t) buffersize * NUM_CHANNELS * 2);
    this->latencySamples = latencySamples < buffersize ? buffersize : latencySamples;

    numBuffers = (latencySamples / buffersize) * 2;
    if (numBuffers < 32) numBuffers = 32;
    bufferStep = (buffersize + 64) * NUM_CHANNELS;
    size_t fifoBufferSizeBytes = numBuffers * bufferStep * sizeof(short int);
    fifobuffer = (short int *) malloc(fifoBufferSizeBytes);
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
