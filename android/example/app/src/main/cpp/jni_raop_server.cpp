/*
 * MIT License
 *
 * Copyright (c) 2019 dsafa22, All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <jni.h>
#include <cstddef>
#include "lib/raop.h"
#include "lib/dnssd.h"
#include "airplay.h"
//#include "log.h"
//#include "stream.h"
//#include "logger.h"
#include <malloc.h>
#include <cstring>
#include <cmath>
#include <string>
//#include "raop_server.h"

static JavaVM* g_JavaVM;

//void OnRecvAudioData(void *observer, pcm_data_struct *data) {
//    auto obj = (jobject) observer;
//    JNIEnv* jniEnv = NULL;
//    g_JavaVM->AttachCurrentThread(&jniEnv, NULL);
//    jclass cls = jniEnv->GetObjectClass(obj);
//    jmethodID onRecvAudioDataM = jniEnv->GetMethodID(cls, "onRecvAudioData", "([SJ)V");
//    jniEnv->DeleteLocalRef(cls);
//    jshortArray sarr = jniEnv->NewShortArray(data->data_len);
//    if (sarr == NULL) return;
//    jniEnv->SetShortArrayRegion(sarr, (jint) 0, data->data_len, (jshort *) data->data);
//    jniEnv->CallVoidMethod(obj, onRecvAudioDataM, sarr, (uint64_t) data->pts);
//    jniEnv->DeleteLocalRef(sarr);
//    g_JavaVM->DetachCurrentThread();
//}
//
//void OnRecvVideoData(void *observer, h264_decode_struct *data) {
//    auto obj = (jobject) observer;
//    JNIEnv* jniEnv = NULL;
//    g_JavaVM->AttachCurrentThread(&jniEnv, NULL);
//    jclass cls = jniEnv->GetObjectClass(obj);
//    jmethodID onRecvVideoDataM = jniEnv->GetMethodID(cls, "onRecvVideoData", "([BIJJII)V");
//    jniEnv->DeleteLocalRef(cls);
//    jbyteArray barr = jniEnv->NewByteArray(data->data_len);
//    if (barr == NULL) return;
//    jniEnv->SetByteArrayRegion(barr, (jint) 0, data->data_len, (jbyte *) data->data);
//    jniEnv->CallVoidMethod(obj, onRecvVideoDataM, barr, data->frame_type,
//                                         data->pts, data->pts, data->width, data->height);
//    free(data->data);
//    jniEnv->DeleteLocalRef(barr);
//    g_JavaVM->DetachCurrentThread();
//}

void onRecvVideoDestroy(void *observer) {
    auto obj = (jobject) observer;
    JNIEnv* jniEnv = NULL;
    g_JavaVM->AttachCurrentThread(&jniEnv, NULL);
    jclass cls = jniEnv->GetObjectClass(obj);
    jmethodID onRecvVideoDestroyM = jniEnv->GetMethodID(cls, "onRecvVideoDestroy", "()V");
    jniEnv->DeleteLocalRef(cls);
    jniEnv->CallVoidMethod(obj, onRecvVideoDestroyM);
    g_JavaVM->DetachCurrentThread();
}

//extern "C" void
//audio_process(void *cls, void *opaque, pcm_data_struct *data)
//{
//    auto *session = (audio_session_t *)opaque;
//    for (int i = 0; i < data->data_len; i++) {
//        data->data[i] = data->data[i] * session->volume;
//    }
//    OnRecvAudioData(cls, data);
//}
//
//extern "C" void
//video_process(void *cls, h264_decode_struct *data)
//{
//    OnRecvVideoData(cls, data);
//}

extern "C" void
video_destroy(void *cls)
{
    onRecvVideoDestroy(cls);
}

extern "C" JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_JavaVM = vm;
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_fang_myapplication_RaopServer_start(JNIEnv* env, jobject object, jstring deviceName, jbyteArray hwAddr) {
    const char *device_name = env->GetStringUTFChars(deviceName, 0);
    jbyte *hw_addr = env->GetByteArrayElements(hwAddr, 0);
    jsize hw_addr_len = env->GetArrayLength(hwAddr);
    void* cls = (void *) env->NewGlobalRef(object);
    //raop_server_t* raop_server = raop_server_init(cls, audio_process, video_process, video_destroy);
    //raop_server_start(raop_server, device_name, (char*) hw_addr, hw_addr_len);
    int ret = HHAirPlayStart("HelloAirplay20240511");
    env->ReleaseByteArrayElements(hwAddr, hw_addr, 0);
    env->ReleaseStringUTFChars(deviceName, device_name);
    return (jlong) (void *) 0;//raop_server;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_fang_myapplication_RaopServer_getPort(JNIEnv* env, jobject object, jlong opaque) {
    //auto * raop_server = (raop_server_t *) (void *) opaque;
    return 0;//raop_server_get_port(raop_server);
}

extern "C" JNIEXPORT void JNICALL
Java_com_fang_myapplication_RaopServer_stop(JNIEnv* env, jobject object, jlong opaque) {
//    auto * raop_server = (raop_server_t *) (void *) opaque;
//    auto obj = (jobject) raop_server_get_cls(raop_server);
//    raop_server_destroy(raop_server);
//    env->DeleteGlobalRef(obj);
}
