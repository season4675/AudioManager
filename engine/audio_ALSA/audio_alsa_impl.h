/* Copyright [2018] <shichen.fsc@alibaba-inc.com>
 * File: AudioManager/engine/audio_ALSA/audio_alsa_impl.h
 */

#ifndef AUDIOMANAGER_ENGINE_AUDIO_ALSA_AUDIO_ALSA_IMPL_H_
#define AUDIOMANAGER_ENGINE_AUDIO_ALSA_AUDIO_ALSA_IMPL_H_

#include "audio_engine_impl.h"

namespace nuiam {

static char kAudioEngineVersion[128] = "Audio Engine ALSA Version ";

class AlsaEngine : public AudioEngineImpl {
 public:
  virtual char *audio_get_version();

  virtual AMResult audio_output_set_vol(int vol, int player_id);
  virtual AMResult audio_output_get_vol(int player_id);
  virtual AMResult audio_output_set_mute(int player_id, bool mute);
  virtual void audio_registerListener(const AMEventListener &listener);

  virtual AMResult audio_output_open(AMDataFormat *data_format);
  virtual AMResult audio_output_close(int player_id);
  virtual AMBufferCount audio_output_write(void *src_buffer,
                                           const int buffer_size,
                                           int player_id);
  virtual AMBufferCount audio_output_getBufferCount(int player_id);
  virtual AMResult audio_output_pause(int player_id);
  virtual AMResult audio_output_stop(int player_id, bool drain = false);
  virtual AMResult audio_output_getAudioFormat(AMDataFormat *data_format,
                                               int player_id);
  virtual AMResult audio_output_getAudioStatus(AMAudioStatus *audio_status,
                                               int player_id);
  virtual AMResult audio_output_setAudioStatus(AMAudioStatus *audio_status,
                                               int player_id);

  virtual AMResult audio_outputFromFile_open(AMFileInfo *file_info,
                                             AMDataFormat *data_format);
  virtual AMResult audio_outputFromFile_close(int player_id);
  virtual AMResult audio_outputFromFile_start(int player_id);
  virtual AMResult audio_outputFromFile_pause(int player_id);
  virtual AMResult audio_outputFromFile_stop(int player_id);
  virtual AMResult audio_outputFromFile_getAudioFormat(
      AMDataFormat *data_format,
      int player_id);
  virtual AMResult audio_outputFromFile_getAudioStatus(
      AMAudioStatus *audio_status,
      int player_id);
  virtual AMResult audio_outputFromFile_setAudioStatus(
      AMAudioStatus *audio_status,
      int player_id);

  virtual AMResult audio_input_open(AMDataFormat *data_format);
  virtual AMResult audio_input_close();
  virtual AMBufferCount audio_input_read(void *read_buffer, int buffer_count);
  virtual AMResult audio_input_pause();
  virtual AMResult audio_input_stop();
  virtual AMResult audio_input_getAudioFormat(AMDataFormat *data_format);
  virtual AMResult audio_input_getAudioStatus(AMAudioStatus *audio_status);
  virtual AMResult audio_input_setAudioStatus(AMAudioStatus *audio_status);

  virtual AMResult audio_inputToFile_open(AMFileInfo *file_info,
                                          AMDataFormat *data_format);
  virtual AMResult audio_inputToFile_close();
  virtual AMResult audio_inputToFile_start();
  virtual AMResult audio_inputToFile_pause();
  virtual AMResult audio_inputToFile_stop();
  virtual AMResult audio_inputToFile_getAudioFormat(
      AMDataFormat *data_format);
  virtual AMResult audio_inputToFile_getAudioStatus(
      AMAudioStatus *audio_status);
  virtual AMResult audio_inputToFile_setAudioStatus(AMAudioStatus *audio_status);

  AMEventListener audio_listener_;
};

} // namespace nuiam

#endif // AUDIOMANAGER_ENGINE_AUDIO_ALSA_AUDIO_ALSA_IMPL_H_

