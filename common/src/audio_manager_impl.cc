/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/common/src/audio_manager_impl.cc
 */

#include "audio_manager.h"
#include "audio_engine.h"
#include "audio_manager_impl.h"
#include "audio_log.h"

namespace audiomanager {

AudioManagerImpl::AudioManagerImpl() {
  audio_engine_ = new AudioEngine;

  input_data_format_ = new AMDataFormat;
  memset(input_data_format_, 0, sizeof(AMDataFormat));
  output_data_format_ = new AMDataFormat;
  memset(output_data_format_, 0, sizeof(AMDataFormat));

  input_audio_status_ = new AMAudioStatus;
  memset(input_audio_status_, 0, sizeof(AMAudioStatus));
  output_audio_status_ = new AMAudioStatus;
  memset(output_audio_status_, 0, sizeof(AMAudioStatus));

  audio_versions_ = new AMAudioVersions;
  memset(audio_versions_, 0, sizeof(AMAudioVersions));
  audio_versions_->audio_manager_version = kAudioManagerVersion;
  audio_versions_->audio_engine_version = "NULL";

  audio_status_ = new AMStatus;
  memset(audio_status_, 0, sizeof(AMStatus));
}

AudioManagerImpl::~AudioManagerImpl() {
  delete audio_status_;
  audio_status_ = NULL;

  audio_versions_->audio_manager_version = NULL;
  audio_versions_->audio_engine_version = NULL;
  delete audio_versions_;
  audio_versions_ = NULL;

  delete output_audio_status_;
  output_audio_status_ = NULL;
  delete input_audio_status_;
  input_audio_status_ = NULL;

  delete output_data_format_;
  output_data_format_ = NULL;
  delete input_data_format_;
  input_data_format_ = NULL;

  delete audio_engine_;
  audio_engine_ = NULL;
}

AMAudioVersions *AudioManagerImpl::audio_IAudioManager_getVersions() {
  if (audio_versions_ != NULL && audio_engine_ != NULL) {
    audio_versions_->audio_manager_version = kAudioManagerVersion;
    audio_versions_->audio_engine_version =
        audio_engine_->audio_engine_getVersion();
    if (audio_versions_->audio_manager_version != NULL) {
      LOGI("get audio manager version:%s",
          audio_versions_->audio_manager_version);
    }
    if (audio_versions_->audio_engine_version != NULL) {
      LOGI("get audio engine version:%s",
          audio_versions_->audio_engine_version);
    }
  } else {
    LOGE("get audiomanagr versions null.");
  }
  return audio_versions_;
}

AMStatus *AudioManagerImpl::audio_IAudioManager_getStatus() {
  if (NULL == audio_status_) {
    LOGE("AudioManager status audio_status_ is NULL!");
    return NULL;
  }
  return audio_status_;
}

AMResult AudioManagerImpl::audio_IAudioManager_setPlayerVolume(int vol,
                                                               int player_id) {
  int result = -kAMUnknownError;

  if (player_id < PLAYER_MIN || player_id >= PLAYER_MAX) {
    LOGE("player id (%d) is invalid!", player_id);
    result = -kAMParameterInvalid;
    return result;
  }
  if (vol < MIN_VOL_LEVEL || vol > MAX_VOL_LEVEL) {
    LOGE("volume value (%d) is invalid!", vol);
    result = -kAMParameterInvalid;
    return result;
  }
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    result = -kAMResourceError;
    return result;
  }
  result = audio_engine_->audio_engine_set_player_vol(vol, player_id);
  if (kAMSuccess == result) {
    audio_status_->player_status[player_id].volume = vol;
  }
  return result;
}

// if 0 <= AMResult <= 100, express volume level.
// if AMResult <0, express error.
AMResult AudioManagerImpl::audio_IAudioManager_getPlayerVolume(int player_id) {
  int result = -kAMUnknownError;
  if (player_id < PLAYER_MIN || player_id >= PLAYER_MAX) {
    LOGE("player id (%d) is invalid!", player_id);
    result = -kAMParameterInvalid;
    return result;
  }
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    result = -kAMResourceError;
    return result;
  }
  result = audio_engine_->audio_engine_get_player_vol(player_id);
  if (result >= MIN_VOL_LEVEL && result <= MAX_VOL_LEVEL) {
    if (result != audio_status_->player_status[player_id].volume) {
      LOGE("engine volume level (%d) unequal to AMStatus volume level(%d)",
          result, audio_status_->player_status[player_id].volume);
      audio_status_->player_status[player_id].volume = result;
    }
  }
  return result;
}

AMResult AudioManagerImpl::audio_IAudioManager_setPlayerMute(int player_id,
                                                             bool mute) {
  int result = -kAMUnknownError;
  if (player_id < PLAYER_MIN || player_id >= PLAYER_MAX) {
    LOGE("player id (%d) is invalid!", player_id);
    result = -kAMParameterInvalid;
    return result;
  }
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    result = -kAMResourceError;
    return result;
  }
  result = audio_engine_->audio_engine_set_player_mute(player_id, mute);
  if (kAMSuccess == result) {
    audio_status_->player_status[player_id].mute = mute;
  }
  LOGD("set_mute is ok?(%d)",result);
  return result;
}

AMResult AudioManagerImpl::audio_IAudioOutput_open(AMDataFormat *data_format) {
  int result = -kAMUnknownError;

  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    result = -kAMResourceError;
    return result;
  }
  result = audio_engine_->audio_engine_output_open(data_format);
  if (result >= PLAYER_MIN && result < PLAYER_MAX) {
    audio_status_->player_num++;
    audio_status_->player_channel_mask |= (1<<result);
  }
  // return a player id
  return result;
}

AMResult AudioManagerImpl::audio_IAudioOutput_close(int player_id) {
  int result = -kAMUnknownError;

  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    result = -kAMResourceError;
    return result;
  }
  result = audio_engine_->audio_engine_output_close(player_id);
  if (kAMSuccess == result) {
    audio_status_->player_num--;
    audio_status_->player_channel_mask &= ~(1<<player_id);
  }
  return result;
}

AMBufferCount AudioManagerImpl::audio_IAudioOutput_write(
    void *src_buffer,
    const int buffer_size,
    int player_id) {
  int result = -1;

  if (0 == buffer_size) {
    LOGI("Write buffer size is zero, a invalid write!");
    return -kAMParameterInvalid;
  }
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    result = -kAMResourceError;
    return result;
  }
  if (player_id < PLAYER_MIN || player_id >= PLAYER_MAX) {
    LOGE("player id (%d) is invalid!", player_id);
    result = -kAMParameterInvalid;
    return result;
  }
  result = audio_engine_->audio_engine_output_write(src_buffer,
                                                    buffer_size,
                                                    player_id);
  return result;
}

AMBufferCount AudioManagerImpl::audio_IAudioOutput_getBufferCount(
    int player_id) {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_output_getBufferCount(player_id);
}

AMResult AudioManagerImpl::audio_IAudioOutput_getAudioFormat(
                             AMDataFormat *data_format,
                             int player_id) {
  data_format = output_data_format_;
  return kAMSuccess;
}

AMResult AudioManagerImpl::audio_IAudioOutput_getAudioStatus(
                             AMAudioStatus *audio_status,
                             int player_id) {
  audio_status = output_audio_status_;
  return kAMSuccess;
}

AMResult AudioManagerImpl::audio_IAudioOutput_setAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_output_setAudioStatus(audio_status,
                                                           player_id);
}

AMResult AudioManagerImpl::audio_IAudioOutput_pause(int player_id) {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_output_pause(player_id);
}

AMResult AudioManagerImpl::audio_IAudioOutput_stop(int player_id) {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_output_stop(player_id);
}

AMResult AudioManagerImpl::audio_IAudioOutputFromFile_open(
    AMFileInfo *file_info,
    AMDataFormat *data_format) {
  int result = -kAMUnknownError;

  if (NULL == file_info) {
    LOGE("File info is NULL.");
    return -kAMParameterInvalid;
  }
  if (NULL == file_info->file_path) {
    LOGE("File path is NULL.");
    return -kAMParameterInvalid;
  }
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    result = -kAMResourceError;
    return result;
  }
  result = audio_engine_->audio_engine_outputFromFile_open(file_info,
                                                           data_format);
  if (result >= PLAYER_MIN && result < PLAYER_MAX) {
    audio_status_->player_num++;
    audio_status_->player_channel_mask |= (1<<result);
  }
  return result;
}

AMResult AudioManagerImpl::audio_IAudioOutputFromFile_close(int player_id) {
  int result = -kAMUnknownError;

  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    result = -kAMResourceError;
    return result;
  }
  result = audio_engine_->audio_engine_outputFromFile_close(player_id);
  if (kAMSuccess == result) {
    audio_status_->player_num--;
    audio_status_->player_channel_mask &= ~(1<<player_id);
  }
  return result;
}

AMResult AudioManagerImpl::audio_IAudioOutputFromFile_start(int player_id) {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_outputFromFile_start(player_id);
}

AMResult AudioManagerImpl::audio_IAudioOutputFromFile_pause(int player_id) {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_outputFromFile_pause(player_id);
}

AMResult AudioManagerImpl::audio_IAudioOutputFromFile_stop(int player_id) {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_outputFromFile_stop(player_id);
}

AMResult AudioManagerImpl::audio_IAudioOutputFromFile_getAudioFormat(
    AMDataFormat *data_format,
    int player_id) {
  data_format = output_data_format_;
  return kAMSuccess;
}

AMResult AudioManagerImpl::audio_IAudioOutputFromFile_getAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  audio_status = output_audio_status_;
  return kAMSuccess;
}

AMResult AudioManagerImpl::audio_IAudioOutputFromFile_setAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_outputFromFile_setAudioStatus(audio_status,
                                                                   player_id);
}

AMResult AudioManagerImpl::audio_IAudioInput_open(AMDataFormat *data_format) {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_input_open(data_format);
}

AMResult AudioManagerImpl::audio_IAudioInput_close() {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_input_close();
}

AMBufferCount AudioManagerImpl::audio_IAudioInput_read(void *read_buffer,
                                                       const int buffer_count) {
  int result = -1;

  if (0 == buffer_count) {
    LOGI("Read buffer count is zero, a invalid read!");
    return -kAMParameterInvalid;
  }
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    result = -kAMResourceError;
    return result;
  }
  result = audio_engine_->audio_engine_input_read(read_buffer, buffer_count);
  return result;
}

AMResult AudioManagerImpl::audio_IAudioInput_pause() {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_input_pause();
}

AMResult AudioManagerImpl::audio_IAudioInput_stop() {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_input_stop();
}

AMResult AudioManagerImpl::audio_IAudioInput_getAudioFormat(
    AMDataFormat *data_format) {
  data_format = input_data_format_;
  return kAMSuccess;
}

AMResult AudioManagerImpl::audio_IAudioInput_getAudioStatus(
    AMAudioStatus *audio_status) {
  audio_status = input_audio_status_;
  return kAMSuccess;
}

AMResult AudioManagerImpl::audio_IAudioInput_setAudioStatus(
    AMAudioStatus *audio_status) {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_input_setAudioStatus(audio_status);
}

AMResult AudioManagerImpl::audio_IAudioInputToFile_open(
    AMFileInfo *file_info,
    AMDataFormat *data_format) {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_inputToFile_open(file_info, data_format);
}

AMResult AudioManagerImpl::audio_IAudioInputToFile_close() {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_inputToFile_close();
}

AMResult AudioManagerImpl::audio_IAudioInputToFile_start() {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_inputToFile_start();
}

AMResult AudioManagerImpl::audio_IAudioInputToFile_pause() {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_inputToFile_pause();
}

AMResult AudioManagerImpl::audio_IAudioInputToFile_stop() {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_inputToFile_stop();
}

AMResult AudioManagerImpl::audio_IAudioInputToFile_getAudioFormat(
    AMDataFormat *data_format) {
  data_format = input_data_format_;
  return kAMSuccess;
}

AMResult AudioManagerImpl::audio_IAudioInputToFile_getAudioStatus(
    AMAudioStatus *audio_status) {
  audio_status = input_audio_status_;
  return kAMSuccess;
}

AMResult AudioManagerImpl::audio_IAudioInputToFile_setAudioStatus(
    AMAudioStatus *audio_status) {
  if (NULL == audio_engine_) {
    LOGE("AudioManager operation-interface audio_engine_ is NULL!");
    return -kAMResourceError;
  }
  return audio_engine_->audio_engine_inputToFile_setAudioStatus(audio_status);
}

} // namespace audiomanager

