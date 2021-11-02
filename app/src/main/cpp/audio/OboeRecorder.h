//
// Created by Rokas on 2021-11-01.
//

#ifndef WIRELESS_MICROPHONE_OBOERECORDER_H
#define WIRELESS_MICROPHONE_OBOERECORDER_H

#include <oboe/Oboe.h>
#include <math.h>
#include "../log/Log.h"
#include "../queue/Queue.h"

using namespace oboe;

class OboeRecorder: public oboe::AudioStreamCallback {
public:
    virtual ~OboeRecorder() = default;
    OboeRecorder() = default;
    OboeRecorder(int channelCount, int sampleRate);
    int32_t startRecording();
    void stopRecording();
    bool isRecording();
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override;

    AudioQueue queue;
private:
    std::mutex         mLock;
    std::shared_ptr<oboe::AudioStream> mStream;
    bool recording;

    // Stream params
    int channelCount = 1;
    int sampleRate = 48000;
};


#endif //WIRELESS_MICROPHONE_OBOERECORDER_H
