/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/* This is a JNI example where we use native methods to play sounds
 * using OpenSL ES. See the corresponding Java source file located at:
 *
 *   src/com/example/nativeaudio/NativeAudio/NativeAudio.java
 */

#include <stdlib.h>
#include <jni.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "audio_manager.h"

#define LOG_TAG "NativeAudio"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define VECSAMPS_STEREO 128
#define VECSAMPS_MONO 64
#define TEST_CAPTURE_FILE_PATH "/sdcard/audio.pcm"
#define CONV16BIT 32768

static int isEcho = 0;
static int g_loop_exit = 0;
static short inbuffer[VECSAMPS_STEREO], outbuffer[VECSAMPS_STEREO];
static FILE * fp = NULL;

static int global_player_id0 = -1;
static char global_player_vol0 = 0;
static bool global_echo_player_mute = false;

static int global_assets_player_id = -1;
static char global_assets_player_vol = 0;
static bool global_assets_player_mute = false;

#ifdef __cplusplus
extern "C" {
#endif

static audiomanager::AudioManager *global_handler = NULL;

void *startEcho_process(void *context) {
    audiomanager::AudioManager *audiomanager = global_handler;
    //int i, j;
    static int buffers, result;

    while (1) {
        if (isEcho) {
            buffers = audiomanager->audio_IAudioInput_read((void *)inbuffer,
                                                           VECSAMPS_STEREO * sizeof(short));
            /*
            for (i = 0, j = 0; i < buffers; i++, j += 2) {
                outbuffer[j] = outbuffer[j + 1] = inbuffer[i];
            }
             */
            if (global_player_id0 >= 0)
                result = audiomanager->audio_IAudioOutput_write((void *)inbuffer, buffers, global_player_id0);
        } else {
            usleep(100);
        }
    }
}

// open echo - open player and recorder devices
void Java_com_example_nativeaudio_NativeAudio_openEcho(JNIEnv *env, jclass clazz) {
    audiomanager::AMDataFormat sink_data_format;
    audiomanager::AMDataFormat in_data_format;
    audiomanager::AudioManager *audiomanager = NULL;
    int result = -1;

    if (NULL == global_handler) {
        audiomanager = audiomanager::AudioManager::Create();
        if (NULL == audiomanager) {
            LOGE("Fail to create audiomanager!");
            return;
        }
        global_handler = audiomanager;
    } else {
        audiomanager = global_handler;
    }

    // init audio data format
    sink_data_format.format_type = audiomanager::kAMDataFormatPCMInterleaved;
    sink_data_format.num_channels = 1;
    sink_data_format.sample_rate = audiomanager::kAMSampleRate16K;
    sink_data_format.bits_per_sample = audiomanager::kAMSampleFormatFixed16;

    in_data_format.format_type = audiomanager::kAMDataFormatPCMInterleaved;
    in_data_format.num_channels = 1;
    in_data_format.sample_rate = audiomanager::kAMSampleRate16K;
    in_data_format.bits_per_sample = audiomanager::kAMSampleFormatFixed16;

    result = audiomanager->audio_IAudioOutput_open(&sink_data_format);
    if (result >= 0) {
        LOGI("Open player0(%d) success.", result);
        global_player_id0 = result;
    } else
        LOGE("Open player failed(%d).", result);

    audiomanager->audio_IAudioInput_open(&in_data_format);
}

// start echo - start player and recorder
void Java_com_example_nativeaudio_NativeAudio_startEcho(JNIEnv *env, jclass clazz) {
    isEcho = 1;
    pthread_t thread_echo;
    pthread_create(&thread_echo, NULL, startEcho_process, (void *) NULL);
    pthread_detach(thread_echo);
}

// pause echo - pause player and recorder
void Java_com_example_nativeaudio_NativeAudio_pauseEcho(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    isEcho = 0;
    audiomanager->audio_IAudioInput_pause();
    audiomanager->audio_IAudioOutput_pause(global_player_id0);
}

// stop echo - stop player and recorder
void Java_com_example_nativeaudio_NativeAudio_stopEcho(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    isEcho = 0;
    audiomanager->audio_IAudioInput_stop();
    audiomanager->audio_IAudioOutput_stop(global_player_id0);
}

// close echo - close player and recorder devices
void Java_com_example_nativeaudio_NativeAudio_closeEcho(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    isEcho = 0;
    int result = -audiomanager::kAMUnknownError;

    if (audiomanager != NULL) {
        result = audiomanager->audio_IAudioInput_close();
        if (result != audiomanager::kAMSuccess) {
            LOGE("Close echo input failed! ret = %d", result);
            return;
        }
        result = audiomanager->audio_IAudioOutput_close(global_player_id0);
        if (result != audiomanager::kAMSuccess) {
            LOGE("Close echo output failed! ret = %d", result);
            return;
        }
    }
    result = audiomanager::AudioManager::Destroy(audiomanager);
    if (result != audiomanager::kAMSuccess) {
        LOGE("destory audiomanager failed when close echo! ret = %d", result);
        return;
    }
    global_handler = NULL;
}

// echo volume up - volume + 5/100
void Java_com_example_nativeaudio_NativeAudio_echoVolUp(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    if (NULL == audiomanager)
        return;
    global_player_vol0 = audiomanager->audio_IAudioManager_getPlayerVolume(global_player_id0);
    global_player_vol0 += 5;
    audiomanager->audio_IAudioManager_setPlayerVolume(global_player_vol0, global_player_id0);
}

// echo volume down - volume - 5/100
void Java_com_example_nativeaudio_NativeAudio_echoVolDown(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    if (NULL == audiomanager)
        return;
    global_player_vol0 = audiomanager->audio_IAudioManager_getPlayerVolume(global_player_id0);
    global_player_vol0 -= 5;
    audiomanager->audio_IAudioManager_setPlayerVolume(global_player_vol0, global_player_id0);
}

// echo player mute - mute or unmute player
void Java_com_example_nativeaudio_NativeAudio_echoMute(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    if (NULL == audiomanager)
        return;
    global_echo_player_mute = !global_echo_player_mute;
    audiomanager->audio_IAudioManager_setPlayerMute(global_player_id0, global_echo_player_mute);
}

// open URI - open player device
void Java_com_example_nativeaudio_NativeAudio_openURI(JNIEnv *env, jclass clazz) {
    audiomanager::AMDataFormat data_format;
    audiomanager::AMFileInfo file_info;

    file_info.file_path = "http://www.freesound.org/data/previews/18/18765_18799-lq.mp3";
    file_info.file_type = audiomanager::kAMFileTypeURI;

    audiomanager::AudioManager *audiomanager = audiomanager::AudioManager::Create();
    if (NULL == audiomanager) {
        LOGE("Fail to create audiomanager!");
    }
    global_handler = audiomanager;

    // init audio data format
    data_format.format_type = audiomanager::kAMDataFormatPCMInterleaved;
    data_format.num_channels = 2;
    data_format.sample_rate = audiomanager::kAMSampleRate16K;
    data_format.bits_per_sample = audiomanager::kAMSampleFormatFixed16;

    audiomanager->audio_IAudioOutputFromFile_open(&file_info, &data_format);
}

// start URI - start player
void Java_com_example_nativeaudio_NativeAudio_startURI(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    audiomanager->audio_IAudioOutputFromFile_start(global_player_id0);
}

// pause URI - pause player
void Java_com_example_nativeaudio_NativeAudio_pauseURI(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    audiomanager->audio_IAudioOutputFromFile_pause(global_player_id0);
}

// stop URI - stop player
void Java_com_example_nativeaudio_NativeAudio_stopURI(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    audiomanager->audio_IAudioOutputFromFile_stop(global_player_id0);
}

// close URI - close player device
void Java_com_example_nativeaudio_NativeAudio_closeURI(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;

    audiomanager->audio_IAudioOutputFromFile_close(global_player_id0);

    audiomanager::AudioManager::Destroy(audiomanager);
    global_handler = NULL;
}

// open ASSET - open player device
void Java_com_example_nativeaudio_NativeAudio_openASSETS(JNIEnv *env, jclass clazz, jobject assetManager, jstring filename) {
    audiomanager::AMDataFormat data_format;
    audiomanager::AMFileInfo file_info;
    int fd = 0;
    int result = -1;
    off_t start, length;
    const char *utf8 = env->GetStringUTFChars(filename, NULL);
    audiomanager::AudioManager *audiomanager = NULL;

    // use asset manager to open asset by filename
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    AAsset* asset = AAssetManager_open(mgr, utf8, AASSET_MODE_UNKNOWN);
    if (NULL == asset)
        LOGE("AAssetManager_open faild!");
    env->ReleaseStringUTFChars(filename, utf8);
    LOGI("Start open assets file %s.", utf8);

    // open asset as file descriptor
    fd = AAsset_openFileDescriptor(asset, &start, &length);

    AAsset_close(asset);

    file_info.file_path = utf8;
    file_info.file_type = audiomanager::kAMFileTypeASSETS;
    file_info.fd = fd;
    file_info.start = start;
    file_info.length = length;
#if defined(__LP64__)
    LOGI("Open assets file fd(%ld)", file_info.fd);
#else
    LOGI("Open assets file fd(%lld)", file_info.fd);
#endif
    if (NULL == global_handler) {
        audiomanager = audiomanager::AudioManager::Create();
        if (NULL == audiomanager) {
            LOGE("Fail to create audiomanager!");
            return;
        }
        global_handler = audiomanager;
    } else {
        audiomanager = global_handler;
    }

    // data_format no use
    result = audiomanager->audio_IAudioOutputFromFile_open(&file_info, &data_format);
    if (result >= 0) {
        LOGI("Open assets player(%d) success.", result);
        global_assets_player_id = result;
    } else
        LOGE("Open assets player failed(%d).", result);
}

// start ASSET - start player
void Java_com_example_nativeaudio_NativeAudio_startASSETS(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    audiomanager->audio_IAudioOutputFromFile_start(global_assets_player_id);
}

// pause ASSET - pause player
void Java_com_example_nativeaudio_NativeAudio_pauseASSETS(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    audiomanager->audio_IAudioOutputFromFile_pause(global_assets_player_id);
}

// stop ASSET - stop player
void Java_com_example_nativeaudio_NativeAudio_stopASSETS(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    audiomanager->audio_IAudioOutputFromFile_stop(global_assets_player_id);
}

// close ASSET - close player device
void Java_com_example_nativeaudio_NativeAudio_closeASSETS(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    int result = -audiomanager::kAMUnknownError;

    if (audiomanager != NULL) {
        result = audiomanager->audio_IAudioOutputFromFile_close(global_assets_player_id);
        if (result != audiomanager::kAMSuccess) {
            LOGE("Close assets failed! ret = %d", result);
            return;
        }
    }

    result = audiomanager::AudioManager::Destroy(audiomanager);
    if (result != audiomanager::kAMSuccess) {
        LOGE("destory audiomanager failed when close assets! ret = %d", result);
        return;
    }
    global_handler = NULL;
}

// asset volume up - volume + 5/100
void Java_com_example_nativeaudio_NativeAudio_assetVolUp(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    if (NULL == audiomanager)
        return;
    global_assets_player_vol = audiomanager->audio_IAudioManager_getPlayerVolume(global_assets_player_id);
    global_assets_player_vol += 5;
    audiomanager->audio_IAudioManager_setPlayerVolume(global_assets_player_vol, global_assets_player_id);
}

// asset volume down - volume - 5/100
void Java_com_example_nativeaudio_NativeAudio_assetVolDown(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    if (NULL == audiomanager)
        return;
    global_assets_player_vol = audiomanager->audio_IAudioManager_getPlayerVolume(global_assets_player_id);
    global_assets_player_vol -= 5;
    audiomanager->audio_IAudioManager_setPlayerVolume(global_assets_player_vol, global_assets_player_id);
}

// asset player mute - mute or unmute player
void Java_com_example_nativeaudio_NativeAudio_assetMute(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    if (NULL == audiomanager)
        return;
    global_assets_player_mute = !global_assets_player_mute;
    audiomanager->audio_IAudioManager_setPlayerMute(global_assets_player_id, global_assets_player_mute);
}


void *recording_process(void *context) {
    audiomanager::AudioManager *audiomanager = global_handler;
    static int bytes;
    short buffer[VECSAMPS_MONO];
    int i;

    while (!g_loop_exit) {
        bytes = audiomanager->audio_IAudioInput_read(inbuffer, VECSAMPS_MONO * sizeof(short));
        if (fwrite((unsigned char *)buffer, bytes, 1, fp) != 1) {
            LOGE("failed to save captured data !\n ");
            break;
        }
    }
    return NULL;
}

// open recorder - open recorder devices
void Java_com_example_nativeaudio_NativeAudio_openRecorder(JNIEnv *env, jclass clazz) {
    audiomanager::AMDataFormat in_data_format;

    audiomanager::AudioManager *audiomanager = audiomanager::AudioManager::Create();
    if (NULL == audiomanager) {
        LOGE("Fail to create audiomanager!");
    }
    global_handler = audiomanager;

    // init audio data format
    in_data_format.format_type = audiomanager::kAMDataFormatPCMInterleaved;
    in_data_format.num_channels = 1;
    in_data_format.sample_rate = audiomanager::kAMSampleRate44K1;
    in_data_format.bits_per_sample = audiomanager::kAMSampleFormatFixed16;

    audiomanager->audio_IAudioInput_open(&in_data_format);
}

// start recording - start recorder
void Java_com_example_nativeaudio_NativeAudio_recording(JNIEnv *env, jclass clazz) {
    pthread_t thread_recorder;
    fp = fopen(TEST_CAPTURE_FILE_PATH, "wb");
    if(NULL == fp) {
        LOGE("cannot open file (%s)\n", TEST_CAPTURE_FILE_PATH);
        return;
    }
    g_loop_exit = 0;
    pthread_create(&thread_recorder, NULL, recording_process, (void *) NULL);
    pthread_detach(thread_recorder);
}

// stop recorder - stop recorder
void Java_com_example_nativeaudio_NativeAudio_stopRecorder(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;

    if (audiomanager != NULL)
        audiomanager->audio_IAudioInput_stop();

    if(fp != NULL) {
        fclose(fp);
    }
}

// close recorder - close recorder devices
void Java_com_example_nativeaudio_NativeAudio_closeRecorder(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    int result = -audiomanager::kAMUnknownError;

    if (audiomanager != NULL) {
        result = audiomanager->audio_IAudioInput_close();
        if (result != audiomanager::kAMSuccess) {
            LOGE("Close recorder failed! ret = %d", result);
            return;
        }
    }

    result = audiomanager::AudioManager::Destroy(audiomanager);
    if (result != audiomanager::kAMSuccess) {
        LOGE("destory audiomanager failed when close recorder! ret = %d", result);
        return;
    }
    global_handler = NULL;
}

// get versions
void Java_com_example_nativeaudio_NativeAudio_getVersion(JNIEnv *env, jclass clazz) {
    audiomanager::AudioManager *audiomanager = global_handler;
    audiomanager::AMAudioVersions *versions= NULL;

    if (audiomanager != NULL) {
        versions = audiomanager->audio_IAudioManager_getVersions();
        if (versions == NULL) {
            LOGE("Get versions failed!");
            return;
        }
    }
    LOGI("Audio Manager Version: %s", versions->audio_manager_version);
    LOGI("Audio Engine Version: %s", versions->audio_engine_version);
}
#ifdef __cplusplus
}
#endif
