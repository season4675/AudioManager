/* Copyright [2018] <season4675@gmail.com> 
 * File: AudioManager/engine/audio_engine.cc
 */

#include "audio_common.h"
#include "audio_manager.h"
#include "audio_manager_impl.h"
#include "audio_engine_impl.h"
#include "audio_engine.h"
#include "audio_log.h"

#ifdef _ANDROID_PLATFORM_
#include "audio_openSL/audio_opensl_impl.h"
#elif defined _IOS_PLATFORM_
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma clang diagnostic ignored "-Wunused-function"
#define HAVE_SNPRINTF
#endif
#include "audio_iOS/audio_ios_impl.h"
#endif

namespace audiomanager {

AudioEngine::AudioEngine() {
#ifdef _ANDROID_PLATFORM_
  engine_impl_ = new OpenslEngine;
#elif defined _IOS_PLATFORM_
  engine_impl_ = new AudioQueueEngine;
#endif
}

AudioEngine::~AudioEngine() {
  if (engine_impl_ != NULL)
    delete engine_impl_;
}

char* AudioEngine::audio_engine_getVersion() {
  char *engine_version = NULL;
  int result = -kAMUnknownError;
  if (NULL == engine_impl_) {
    LOGE("Cannot get OpenSL engine when get version!");
    return NULL;
  }
  engine_version = engine_impl_->audio_get_version();
  return engine_version;
}

AMResult AudioEngine::audio_engine_set_player_vol(int vol, int player_id) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_output_set_vol(vol, player_id);
  }
  return result;
}

AMResult AudioEngine::audio_engine_get_player_vol(int player_id) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_output_get_vol(player_id);
  }
  return result;
}

AMResult AudioEngine::audio_engine_set_player_mute(int player_id, bool mute) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_output_set_mute(player_id, mute);
  }
  return result;
}

AMResult AudioEngine::audio_engine_output_open(AMDataFormat *data_format) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_output_open(data_format);
  }
  return result;
}

AMResult AudioEngine::audio_engine_output_close(int player_id) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_output_close(player_id);
  }
  return result;
}

AMBufferCount AudioEngine::audio_engine_output_write(void *src_buffer,
                                                     const int buffer_size,
                                                     int player_id) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_output_write(src_buffer,
                                              buffer_size,
                                              player_id);
  }
  return result;
}

AMBufferCount AudioEngine::audio_engine_output_getBufferCount(int player_id) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_output_getBufferCount(player_id);
  }
  return result;
}

AMResult AudioEngine::audio_engine_output_pause(int player_id) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_output_pause(player_id);
  }
  return result;
}

AMResult AudioEngine::audio_engine_output_stop(int player_id) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_output_stop(player_id);
  }
  return result;
}

AMResult AudioEngine::audio_engine_output_getAudioFormat(
    AMDataFormat *data_format,
    int player_id) {
  return kAMSuccess;
}

AMResult AudioEngine::audio_engine_output_getAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  return kAMSuccess;
}

AMResult AudioEngine::audio_engine_output_setAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_output_setAudioStatus(audio_status, player_id);
  }
  return result;
}

AMResult AudioEngine::audio_engine_outputFromFile_open(
    AMFileInfo *file_info,
    AMDataFormat *data_format) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_outputFromFile_open(file_info, data_format);
  }
  return result;
}

AMResult AudioEngine::audio_engine_outputFromFile_close(int player_id) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_outputFromFile_close(player_id);
  }

  return result;
}

AMResult AudioEngine::audio_engine_outputFromFile_start(int player_id) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_outputFromFile_start(player_id);
  }
  return result;
}

AMResult AudioEngine::audio_engine_outputFromFile_pause(int player_id) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_outputFromFile_pause(player_id);
  }
  return result;
}

AMResult AudioEngine::audio_engine_outputFromFile_stop(int player_id) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_outputFromFile_stop(player_id);
  }
  return result;
}

AMResult AudioEngine::audio_engine_outputFromFile_getAudioFormat(
    AMDataFormat *data_format,
    int player_id) {
  return kAMSuccess;
}

AMResult AudioEngine::audio_engine_outputFromFile_getAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  return kAMSuccess;
}

AMResult AudioEngine::audio_engine_outputFromFile_setAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_outputFromFile_setAudioStatus(audio_status,
                                                               player_id);
  }
  return result;
}

AMResult AudioEngine::audio_engine_input_open(AMDataFormat *data_format) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_input_open(data_format);
  }
  return result;
}

AMResult AudioEngine::audio_engine_input_close() {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_input_close();
  }
  return result;
}

AMBufferCount AudioEngine::audio_engine_input_read(void *read_buffer,
                                                   const int buffer_count) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_input_read(read_buffer, buffer_count);
  }
  return result;
}

AMResult AudioEngine::audio_engine_input_pause() {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_input_pause();
  }
  return result;
}

AMResult AudioEngine::audio_engine_input_stop() {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_input_stop();
  }
  return result;
}

AMResult AudioEngine::audio_engine_input_getAudioFormat(
    AMDataFormat *data_format) {
  return kAMSuccess;
}

AMResult AudioEngine::audio_engine_input_getAudioStatus(
    AMAudioStatus *audio_status) {
  return kAMSuccess;
}

AMResult AudioEngine::audio_engine_input_setAudioStatus(
    AMAudioStatus *audio_status) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_input_setAudioStatus(audio_status);
  }
  return result;
}

AMResult AudioEngine::audio_engine_inputToFile_open(AMFileInfo *file_info,
                                                    AMDataFormat *data_format) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_inputToFile_open(file_info, data_format);
  }
  return result;
}

AMResult AudioEngine::audio_engine_inputToFile_close() {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_inputToFile_close();
  }
  return result;
}

AMResult AudioEngine::audio_engine_inputToFile_start() {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_inputToFile_start();
  }
  return result;
}

AMResult AudioEngine::audio_engine_inputToFile_pause() {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_inputToFile_pause();
  }
  return result;
}

AMResult AudioEngine::audio_engine_inputToFile_stop() {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_inputToFile_stop();
  }
  return result;
}

AMResult AudioEngine::audio_engine_inputToFile_getAudioFormat(
    AMDataFormat *data_format) {
  return kAMSuccess;
}

AMResult AudioEngine::audio_engine_inputToFile_getAudioStatus(
    AMAudioStatus *audio_status) {
  return kAMSuccess;
}

AMResult AudioEngine::audio_engine_inputToFile_setAudioStatus(
    AMAudioStatus *audio_status) {
  int result = -kAMUnknownError;
  if (engine_impl_ != NULL) {
    result = engine_impl_->audio_inputToFile_setAudioStatus(audio_status);
  }
  return result;
}

} // namespace audiomanager

