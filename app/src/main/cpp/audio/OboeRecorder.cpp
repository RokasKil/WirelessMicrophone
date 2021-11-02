//
// Created by Rokas on 2021-11-01.
//

#include "OboeRecorder.h"

OboeRecorder::OboeRecorder(int channelCount, int sampleRate) {
    this->channelCount = channelCount;
    this->sampleRate = sampleRate;
}

int32_t OboeRecorder::startRecording() {
    if (recording) {
        return -1;
    }
    AudioStreamBuilder builder;
    Result result = builder.setSharingMode(SharingMode::Exclusive)
        ->setDirection(Direction::Input)
        ->setPerformanceMode(PerformanceMode::LowLatency)
        ->setChannelCount(channelCount)
        ->setSampleRate(sampleRate)
        ->setCallback(this)
        ->setFormat(AudioFormat::I16) //Signed 16bit integers
        //->setFramesPerCallback(120)
        //->setFramesPerDataCallback(120)
        //->setBufferCapacityInFrames(240)
        ->openStream(mStream);

    if (result != Result::OK) {
        LOGE("Error opening stream %s", convertToText(result));
        return (int32_t) result;
    }
    auto setBufferSizeResult = mStream->setBufferSizeInFrames(mStream->getFramesPerBurst() * 2);
    if (setBufferSizeResult) {
        LOGD("New buffer size is %d frames", setBufferSizeResult.value());
    }
    else {
        LOGE("Failed to set buffer size %s", convertToText(setBufferSizeResult.error()));
        mStream->close();
        return (int32_t) result;
    }
    result = mStream->requestStart();

    if (result != Result::OK) {
        LOGE("Error starting stream %s", convertToText(result));
        mStream->close();
        return (int32_t) result;
    }
    LOGI("Opened with %d sample rate and %d channels api version: %d", mStream->getSampleRate(), mStream->getChannelCount(), mStream->getAudioApi());
    recording = true;
    return (int32_t) result;
}

void OboeRecorder::stopRecording() {
    // Stop, close and delete in case not already closed.
    std::lock_guard <std::mutex> lock(mLock);
    if (mStream && recording) {
        mStream->stop();
        mStream->close();
        mStream.reset();
        recording = false;
    }
}

bool OboeRecorder::isRecording() {
    return recording;
}

DataCallbackResult OboeRecorder::onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) {
    //LOGD("Got %d frames: ", numFrames);
    //One sample per frame for each channel !!!
    //by default we have 2 channels running on I16 format that means
    // one frame is 2 * 2 = 4 bytes
    //queue.
    if (!queue.was_full()) {
        auto *audioDataCasted = (int16_t *) audioData;
        for (int i = 0; i < numFrames * channelCount; i++) {
            queue.push(audioDataCasted[i]);
        }
    }
    return DataCallbackResult::Continue;
}