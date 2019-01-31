/* Copyright [2018] <shichen.fsc@alibaba-inc.com>
 * File: AudioManager/engine/audio_ALSA/alsa_io.h
 */

#ifndef AUDIOMANAGER_ENGINE_AUDIO_ALSA_ALSA_IO_H_
#define AUDIOMANAGER_ENGINE_AUDIO_ALSA_ALSA_IO_H_

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "stdint.h"
#include <stdbool.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

#include "audio_common.h"
#include "circular_buffer.h"

namespace nuiam {

enum ALSA_STATE {
  ALSA_PLAYSTATE_NONE,
  ALSA_PLAYSTATE_OPENED,
  ALSA_PLAYSTATE_PLAYING,
  ALSA_PLAYSTATE_PAUSED,
  ALSA_PLAYSTATE_STOPPED,
  ALSA_PLAYSTATE_CLOSED,

  ALSA_CAPTURESTATE_NONE,
  ALSA_CAPTURESTATE_OPENED,
  ALSA_CAPTURESTATE_CAPTURING,
  ALSA_CAPTURESTATE_PAUSED,
  ALSA_CAPTURESTATE_STOPPED,
  ALSA_CAPTURESTATE_CLOSED,
};

enum AlsaSampleRate {
  ALSA_SAMPLINGRATE_8 = 8000,
  ALSA_SAMPLINGRATE_16 = 16000,
  ALSA_SAMPLINGRATE_24 = 24000,
  ALSA_SAMPLINGRATE_32 = 32000,
  ALSA_SAMPLINGRATE_441 = 44100,
  ALSA_SAMPLINGRATE_48 = 48000,
};

typedef struct AlsaFormat_ {
  snd_pcm_t *handle;
  //snd_pcm_hw_params_t *hw_params;
  circular_buffer *ring_buffer;
  pthread_mutex_t mutex;
  int audio_state;
  int audio_event;
  int wait_interval;
  int max_buf_size;
  int bufSizeDurationOneSec;
  pthread_t thread_id;

  uint32_t format_type;
  uint32_t bit_per_sample;
  uint32_t byte_per_sample;
  uint32_t num_channels;
  uint32_t channel_mask;
  uint32_t endianness;
  uint32_t sample_rate;

  //snd_pcm_uframes_t period_size;
  //uint32_t chunk_bytes; //period_size * bit_per_sample * num_channels / 8

  //audio file descriptor
  int play_type;
  const char *locator;
  FILE *fd;
  int start;
  int length;
} AlsaFormat;

typedef struct AlsaStream_ {
  AlsaFormat player;
  AlsaFormat capture;
} AlsaStream;

AMResult alsa_output_open(AlsaFormat *player);
AMResult alsa_output_start(AlsaFormat *player);
AMResult alsa_output_pause(AlsaFormat *player);
AMResult alsa_output_stop(AlsaFormat *player, bool drain);
AMResult alsa_output_close(AlsaFormat *player);

AMResult alsa_input_open(AlsaFormat *capture);
AMResult alsa_input_start(AlsaFormat *capture);
AMResult alsa_input_pause(AlsaFormat *capture);
AMResult alsa_input_stop(AlsaFormat *capture);
AMResult alsa_input_close(AlsaFormat *capture);

AMResult alsa_set_state(AlsaFormat *alsa, int alsa_state);

} // namespace nuiam

#endif // AUDIOMANAGER_ENGINE_AUDIO_ALSA_ALSA_IO_H_
