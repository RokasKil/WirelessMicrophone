#include <jni.h>
#include <string>
#include "audio/OboeRecorder.h"
#include "network/Server.h"

OboeRecorder recorder;
Server *server = NULL;
extern "C" JNIEXPORT jstring JNICALL
Java_lt_rokas_wirelessmic_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
extern "C"
JNIEXPORT void JNICALL
Java_lt_rokas_wirelessmic_MainActivity_startAudio(JNIEnv *env, jobject thiz) {
    if (server == NULL) {
        server = new Server("", 35555, &recorder.queue);

        LOGI("Started server: %d", server->start());
    }
    recorder.startRecording();
}
extern "C"
JNIEXPORT void JNICALL
Java_lt_rokas_wirelessmic_MainActivity_stopAudio(JNIEnv *env, jobject thiz) {
    recorder.stopRecording();
    server->stop();
    delete server;
    server = NULL;
}
