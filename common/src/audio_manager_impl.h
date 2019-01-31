/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/common/src/audio_manager_impl.h
 */

#ifndef AUDIOMANAGER_COMMON_INCLUDE_AUDIO_MANAGER_IMPL_H_
#define AUDIOMANAGER_COMMON_INCLUDE_AUDIO_MANAGER_IMPL_H_

#include <mutex>
#include "audio_common.h"
#include "audio_manager.h"
#include "audio_engine.h"

namespace audiomanager {

static char kAudioManagerVersion[128] = "Audio Manager Version 0.3";

class AudioManagerImpl : public AudioManager {
 public:
  AudioManagerImpl();
  virtual ~AudioManagerImpl();

  virtual AMAudioVersions *audio_IAudioManager_getVersions();
  virtual AMStatus *audio_IAudioManager_getStatus();
  virtual AMResult audio_IAudioManager_setPlayerVolume(int vol, int player_id);
  virtual AMResult audio_IAudioManager_getPlayerVolume(int player_id);
  virtual AMResult audio_IAudioManager_setPlayerMute(int player_id,
                                                     bool mute);
  virtual AMResult audio_IAudioManager_registerListener(const AMEventListener &listener);

  // Audio Output
  virtual AMResult audio_IAudioOutput_open(AMDataFormat *src, AMDataFormat *sink);
  virtual AMResult audio_IAudioOutput_close(int player_id);
  virtual AMBufferCount audio_IAudioOutput_write(void *src_buffer,
                                                 const int buffer_size,
                                                 int player_id);
  virtual AMBufferCount audio_IAudioOutput_getBufferCount(int player_id);
  virtual AMResult audio_IAudioOutput_pause(int player_id);
  virtual AMResult audio_IAudioOutput_stop(int player_id, bool drain = false);
  virtual AMResult audio_IAudioOutput_getAudioFormat(
      AMDataFormat *data_format,
      int player_id);
  virtual AMResult audio_IAudioOutput_getAudioStatus(
      AMAudioStatus *audio_status,
      int player_id);
  virtual AMResult audio_IAudioOutput_setAudioStatus(
      AMAudioStatus *audio_status,
      int player_id);

  virtual AMResult audio_IAudioOutputFromFile_open(AMFileInfo *file_info,
                                                   AMDataFormat *data_format);
  virtual AMResult audio_IAudioOutputFromFile_close(int player_id);
  virtual AMResult audio_IAudioOutputFromFile_start(int player_id);
  virtual AMResult audio_IAudioOutputFromFile_pause(int player_id);
  virtual AMResult audio_IAudioOutputFromFile_stop(int player_id);
  virtual AMResult audio_IAudioOutputFromFile_getAudioFormat(
      AMDataFormat *data_format,
      int player_id);
  virtual AMResult audio_IAudioOutputFromFile_getAudioStatus(
      AMAudioStatus *audio_status,
      int player_id);
  virtual AMResult audio_IAudioOutputFromFile_setAudioStatus(
      AMAudioStatus *audio_status,
      int player_id);

  // Audio Input
  virtual AMResult audio_IAudioInput_open(AMDataFormat *data_format);
  virtual AMResult audio_IAudioInput_close();
  virtual AMBufferCount audio_IAudioInput_read(void *read_buffer,
                                               const int buffer_count);
  virtual AMResult audio_IAudioInput_pause();
  virtual AMResult audio_IAudioInput_stop();
  virtual AMResult audio_IAudioInput_getAudioFormat(AMDataFormat *data_format);
  virtual AMResult audio_IAudioInput_getAudioStatus(
      AMAudioStatus *audio_status);
  virtual AMResult audio_IAudioInput_setAudioStatus(
      AMAudioStatus *audio_status);

  virtual AMResult audio_IAudioInputToFile_open(AMFileInfo *file_info,
                                                AMDataFormat *data_format);
  virtual AMResult audio_IAudioInputToFile_close();
  virtual AMResult audio_IAudioInputToFile_start();
  virtual AMResult audio_IAudioInputToFile_pause();
  virtual AMResult audio_IAudioInputToFile_stop();
  virtual AMResult audio_IAudioInputToFile_getAudioFormat(
      AMDataFormat *data_format);
  virtual AMResult audio_IAudioInputToFile_getAudioStatus(
      AMAudioStatus *audio_status);
  virtual AMResult audio_IAudioInputToFile_setAudioStatus(
      AMAudioStatus *audio_status);

 private:
  AudioEngine *audio_engine_;
  AMDataFormat *input_data_format_;
  AMAudioStatus *input_audio_status_;
  AMDataFormat *output_data_format_;
  AMAudioStatus *output_audio_status_;
  AMAudioVersions *audio_versions_;
  AMStatus *audio_status_;
  std::mutex lock;
};

} // namespace audiomanager

#endif // AUDIOMANAGER_COMMON_INCLUDE_AUDIO_MANAGER_IMPL_H_

