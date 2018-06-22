/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/engine/audio_engine_impl.h
 */

#ifndef AUDIOMANAGER_ENGINE_AUDIO_ENGINE_IMPL_H_
#define AUDIOMANAGER_ENGINE_AUDIO_ENGINE_IMPL_H_

namespace audiomanager {

class AudioEngineImpl {
 public:
  virtual char* audio_get_version() = 0;

  virtual AMResult audio_output_set_vol(int vol, int player_id) = 0;
  virtual AMResult audio_output_get_vol(int player_id) = 0;
  virtual AMResult audio_output_set_mute(int player_id, bool mute) = 0;

  // Audio Engine Output
  virtual AMResult audio_output_open(AMDataFormat *data_format) = 0;
  virtual AMResult audio_output_close(int player_id) = 0;
  virtual AMBufferCount audio_output_write(void *src_buffer,
                                           const int buffer_size,
                                           int player_id) = 0;
  virtual AMBufferCount audio_output_getBufferCount(int player_id) = 0;
  virtual AMResult audio_output_pause(int player_id) = 0;
  virtual AMResult audio_output_stop(int player_id) = 0;
  virtual AMResult audio_output_getAudioFormat(AMDataFormat *data_format,
                                               int player_id) = 0;
  virtual AMResult audio_output_getAudioStatus(AMAudioStatus *audio_status,
                                               int player_id) = 0;
  virtual AMResult audio_output_setAudioStatus(AMAudioStatus * audio_status,
                                               int player_id) = 0;

  // Audio Engine Output From File
  virtual AMResult audio_outputFromFile_open(AMFileInfo *file_inf,
                                             AMDataFormat *data_format) = 0;
  virtual AMResult audio_outputFromFile_close(int player_id) = 0;
  virtual AMResult audio_outputFromFile_start(int player_id) = 0;
  virtual AMResult audio_outputFromFile_pause(int player_id) = 0;
  virtual AMResult audio_outputFromFile_stop(int player_id) = 0;
  virtual AMResult audio_outputFromFile_getAudioFormat(
      AMDataFormat *data_format,
      int player_id) = 0;
  virtual AMResult audio_outputFromFile_getAudioStatus(
      AMAudioStatus *audio_status,
      int player_id) = 0;
  virtual AMResult audio_outputFromFile_setAudioStatus(
      AMAudioStatus *audio_status,
      int player_id) = 0;

  // Audio Engine Input
  virtual AMResult audio_input_open(AMDataFormat *data_format) = 0;
  virtual AMResult audio_input_close() = 0;
  virtual AMBufferCount audio_input_read(void *read_buffer,
                                         const int buffer_size) = 0;
  virtual AMResult audio_input_pause() = 0;
  virtual AMResult audio_input_stop() = 0;
  virtual AMResult audio_input_getAudioFormat(AMDataFormat *data_format) = 0;
  virtual AMResult audio_input_getAudioStatus(AMAudioStatus *audio_status) = 0;
  virtual AMResult audio_input_setAudioStatus(AMAudioStatus *audio_status) = 0;

  // Audio Engine Input To File
  virtual AMResult audio_inputToFile_open(AMFileInfo *file_info,
                                          AMDataFormat *data_format) = 0;
  virtual AMResult audio_inputToFile_close() = 0;
  virtual AMResult audio_inputToFile_start() = 0;
  virtual AMResult audio_inputToFile_pause() = 0;
  virtual AMResult audio_inputToFile_stop() = 0;
  virtual AMResult audio_inputToFile_getAudioFormat(
      AMDataFormat *data_format) = 0;
  virtual AMResult audio_inputToFile_getAudioStatus(
      AMAudioStatus *audio_status) = 0;
  virtual AMResult audio_inputToFile_setAudioStatus(
      AMAudioStatus *audio_status) = 0;
};

} // namespace audiomanager

#endif // AUDIOMANAGER_ENGINE_AUDIO_ENGINE_IMPL_H_
