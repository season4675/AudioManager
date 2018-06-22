/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/engine/audio_engine.h
 */

#ifndef AUDIOMANAGER_ENGINE_AUDIO_ENGINE_H_
#define AUDIOMANAGER_ENGINE_AUDIO_ENGINE_H_

#include "audio_common.h"
#include "audio_engine_impl.h"

namespace audiomanager {

class AudioEngine {
 public:
  AudioEngine();
  ~AudioEngine();

  char* audio_engine_getVersion();
  AMResult audio_engine_set_player_vol(int vol, int player_id);
  AMResult audio_engine_get_player_vol(int player_id);
  AMResult audio_engine_set_player_mute(int player_id, bool mute);

  // Audio Engine Output
  AMResult audio_engine_output_open(AMDataFormat *data_format);
  AMResult audio_engine_output_close(int player_id);
  AMBufferCount audio_engine_output_write(void *src_buffer,
                                          const int buffer_size,
                                          int player_id);
  AMBufferCount audio_engine_output_getBufferCount(int player_id);
  AMResult audio_engine_output_pause(int player_id);
  AMResult audio_engine_output_stop(int player_id);
  AMResult audio_engine_output_getAudioFormat(AMDataFormat *data_format,
                                              int player_id);
  AMResult audio_engine_output_getAudioStatus(AMAudioStatus *audio_status,
                                              int player_id);
  AMResult audio_engine_output_setAudioStatus(AMAudioStatus *audio_status,
                                              int player_id);

  // Audio Engine Output From File
  AMResult audio_engine_outputFromFile_open(AMFileInfo *file_info,
                                            AMDataFormat *data_format);
  AMResult audio_engine_outputFromFile_close(int player_id);
  AMResult audio_engine_outputFromFile_start(int player_id);
  AMResult audio_engine_outputFromFile_pause(int player_id);
  AMResult audio_engine_outputFromFile_stop(int player_id);
  AMResult audio_engine_outputFromFile_getAudioFormat(
      AMDataFormat *data_format,
      int player_id);
  AMResult audio_engine_outputFromFile_getAudioStatus(
      AMAudioStatus *audio_status,
      int player_id);
  AMResult audio_engine_outputFromFile_setAudioStatus(
      AMAudioStatus *audio_status,
      int player_id);

  // Audio Engine Input
  AMResult audio_engine_input_open(AMDataFormat *data_format);
  AMResult audio_engine_input_close();
  AMBufferCount audio_engine_input_read(void *read_buffer,
                                        const int buffer_count);
  AMResult audio_engine_input_pause();
  AMResult audio_engine_input_stop();
  AMResult audio_engine_input_getAudioFormat(AMDataFormat *data_format);
  AMResult audio_engine_input_getAudioStatus(AMAudioStatus *audio_status);
  AMResult audio_engine_input_setAudioStatus(AMAudioStatus *audio_status);

  // Audio Engine Input To File
  AMResult audio_engine_inputToFile_open(AMFileInfo *file_info,
                                         AMDataFormat *data_format);
  AMResult audio_engine_inputToFile_close();
  AMResult audio_engine_inputToFile_start();
  AMResult audio_engine_inputToFile_pause();
  AMResult audio_engine_inputToFile_stop();
  AMResult audio_engine_inputToFile_getAudioFormat(AMDataFormat *data_format);
  AMResult audio_engine_inputToFile_getAudioStatus(
      AMAudioStatus *audio_status);
  AMResult audio_engine_inputToFile_setAudioStatus(
      AMAudioStatus *audio_status);

 private:
  AudioEngineImpl *engine_impl_;
};

} // namespace audiomanager

#endif // AUDIOMANAGER_ENGINE_AUDIO_ENGINE_H_
