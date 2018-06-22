/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/common/include/audio_manager.h
 */

#ifndef AUDIOMANAGER_COMMON_INCLUDE_AUDIO_MANAGER_H_
#define AUDIOMANAGER_COMMON_INCLUDE_AUDIO_MANAGER_H_

#include "audio_common.h"

namespace audiomanager {

class AudioManager {
 public:
  static AudioManager *Create();
  static AMResult Destroy(AudioManager *audiomanager);

  AudioManager();
  virtual ~AudioManager();
  virtual AMAudioVersions *audio_IAudioManager_getVersions() = 0;
  virtual AMStatus *audio_IAudioManager_getStatus() = 0;
  virtual AMResult audio_IAudioManager_setPlayerVolume(int vol,
                                                       int player_id) = 0;
  virtual AMResult audio_IAudioManager_getPlayerVolume(int player_id) = 0;
  virtual AMResult audio_IAudioManager_setPlayerMute(int player_id,
                                                     bool mute) = 0;

  // Audio Output
  virtual AMResult audio_IAudioOutput_open(AMDataFormat *data_format) = 0;
  virtual AMResult audio_IAudioOutput_close(int player_id) = 0;
  virtual AMBufferCount audio_IAudioOutput_write(void *src_buffer,
                                                 const int buffer_size,
                                                 int player_id) = 0;
  virtual AMBufferCount audio_IAudioOutput_getBufferCount(int player_id) = 0;
  virtual AMResult audio_IAudioOutput_pause(int player_id) = 0;
  virtual AMResult audio_IAudioOutput_stop(int player_id) = 0;
  virtual AMResult audio_IAudioOutput_getAudioFormat(
      AMDataFormat *data_format,
      int player_id) = 0;
  virtual AMResult audio_IAudioOutput_getAudioStatus(
      AMAudioStatus *audio_status,
      int player_id) = 0;
  virtual AMResult audio_IAudioOutput_setAudioStatus(
      AMAudioStatus *audio_status,
      int player_id) = 0;

  // Audio Output From File
  virtual AMResult audio_IAudioOutputFromFile_open(
      AMFileInfo *file_info,
      AMDataFormat *data_format) = 0;
  virtual AMResult audio_IAudioOutputFromFile_close(int player_id) = 0;
  virtual AMResult audio_IAudioOutputFromFile_start(int player_id) = 0;
  virtual AMResult audio_IAudioOutputFromFile_pause(int player_id) = 0;
  virtual AMResult audio_IAudioOutputFromFile_stop(int player_id) = 0;
  virtual AMResult audio_IAudioOutputFromFile_getAudioFormat(
      AMDataFormat *data_format,
      int player_id) = 0;
  virtual AMResult audio_IAudioOutputFromFile_getAudioStatus(
      AMAudioStatus *audio_status,
      int player_id) = 0;
  virtual AMResult audio_IAudioOutputFromFile_setAudioStatus(
      AMAudioStatus *audio_status,
      int player_id) = 0;

  // Audio Input
  virtual AMResult audio_IAudioInput_open(AMDataFormat *data_format) = 0;
  virtual AMResult audio_IAudioInput_close() = 0;
  virtual AMBufferCount audio_IAudioInput_read(void *read_buffer,
                                               const int buffer_size) = 0;
  virtual AMResult audio_IAudioInput_pause() = 0;
  virtual AMResult audio_IAudioInput_stop() = 0;
  virtual AMResult audio_IAudioInput_getAudioFormat(
                     AMDataFormat *data_format) = 0;
  virtual AMResult audio_IAudioInput_getAudioStatus(
                     AMAudioStatus *audio_status) = 0;
  virtual AMResult audio_IAudioInput_setAudioStatus(
      AMAudioStatus *audio_status) = 0;

  // Audio Input To File
  virtual AMResult audio_IAudioInputToFile_open(AMFileInfo *file_info,
                                                AMDataFormat *data_format) = 0;
  virtual AMResult audio_IAudioInputToFile_close() = 0;
  virtual AMResult audio_IAudioInputToFile_start() = 0;
  virtual AMResult audio_IAudioInputToFile_pause() = 0;
  virtual AMResult audio_IAudioInputToFile_stop() = 0;
  virtual AMResult audio_IAudioInputToFile_getAudioFormat(
      AMDataFormat *data_format) = 0;
  virtual AMResult audio_IAudioInputToFile_getAudioStatus(
      AMAudioStatus *audio_status) = 0;
  virtual AMResult audio_IAudioInputToFile_setAudioStatus(
      AMAudioStatus *audio_status) = 0;

};

} // namespace audiomanager

#endif // AUDIOMANAGER_COMMON_INCLUDE_AUDIO_MANAGER_H_

