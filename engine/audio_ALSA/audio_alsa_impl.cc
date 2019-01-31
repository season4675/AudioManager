/* Copyright [2018] <shichen.fsc@alibaba-inc.com>
 * File: AudioManager/engine/audio_ALSA/audio_alsa_impl.cc
 */

#define TAG "AudioAlsaImpl"

#include "audio_common.h"
#include "audio_log.h"
#include "audio_alsa_impl.h"
#include "alsa_io.h"

namespace nuiam {

static AlsaStream *global_alsa_in = NULL;
static AlsaStream *global_alsa_out = NULL;

bool write_check(AlsaEngine *engine, circular_buffer *wp, int buffer_size) {
  AlsaStream *audio_stream = global_alsa_out;
  int wr_size = wp->size;
  int wr_remaining = checkspace_circular_buffer(wp, 1);
  int buf_size_used = wr_size - wr_remaining;
  int interval = 0; //ms
  int bufSizeTenMs = audio_stream->player.bufSizeDurationOneSec / 100; // the buf size in 10ms

  if (buffer_size > wr_size / 3) {
    // too long
    interval = 0;
    audio_stream->player.audio_event = kAMWriteTooLong;
    audio_stream->player.wait_interval = interval;
    if (engine->audio_listener_.audio_event_callback != NULL) {
      engine->audio_listener_.audio_event_callback(
          (AMEvent)audio_stream->player.audio_event,
          audio_stream->player.wait_interval);
    }
    return false;
  }
  if (buffer_size > audio_stream->player.max_buf_size) {
    audio_stream->player.max_buf_size = buffer_size;
  }
  if (buffer_size > wr_remaining) {
    // 需要播放多久才可以腾出多久才能再继续装数据
    int remain_time0 = (buffer_size - wr_remaining) / bufSizeTenMs + 1; // the times of 10ms
    // 剩余数据需要播放多久
    int remain_time1 = buf_size_used / bufSizeTenMs; // the times of 10ms
    interval = 10 * ((remain_time0 < remain_time1) ? remain_time0 : remain_time1);
    // too long
    if (audio_stream->player.audio_event != kAMWriteTooLong ||
        audio_stream->player.wait_interval != interval) {
      // callback
      audio_stream->player.audio_event = kAMWriteTooLong;
      audio_stream->player.wait_interval = interval;
      if (engine->audio_listener_.audio_event_callback != NULL) {
        engine->audio_listener_.audio_event_callback(
            (AMEvent)audio_stream->player.audio_event,
            audio_stream->player.wait_interval);
      }
    }
    return false;
  } else {
    if (wr_remaining <= (audio_stream->player.max_buf_size * 3)) {
    // 预判下次可能装不下历史最大写入量的三倍
      interval = 10 * ((buf_size_used + buffer_size) / bufSizeTenMs);
      // 由于外部interval不准，保守操作
      if (interval > 100)
        interval = interval / 4 * 3;
      // wait
      if (audio_stream->player.audio_event != kAMWriteWait ||
        audio_stream->player.wait_interval != interval) {
        // callback
        audio_stream->player.audio_event = kAMWriteWait;
        audio_stream->player.wait_interval = interval;
        if (engine->audio_listener_.audio_event_callback != NULL) {
          engine->audio_listener_.audio_event_callback(
            (AMEvent)audio_stream->player.audio_event,
            audio_stream->player.wait_interval);
        }
      }
    }
    if (wr_remaining > (wr_size / 2)) {
    // 判断剩余数据量大于写入量的两倍，认为空间充分
      interval = 0;
      // promise
      if (audio_stream->player.audio_event != kAMWritePromise ||
        audio_stream->player.wait_interval != interval) {
        // callback
        audio_stream->player.audio_event = kAMWritePromise;
        audio_stream->player.wait_interval = interval;
        if (engine->audio_listener_.audio_event_callback != NULL) {
          engine->audio_listener_.audio_event_callback(
            (AMEvent)audio_stream->player.audio_event,
            audio_stream->player.wait_interval);
        }
      }
    }
    return true;
  }
}


// just parse sample rate, format type, bits per sample and endianness
static AMResult parse_data_format(AMDataFormat *am_data_format, 
                                  AlsaFormat *alsa_data_format) {
  switch (am_data_format->sample_rate) {
    case kAMSampleRate8K: {
      alsa_data_format->sample_rate = ALSA_SAMPLINGRATE_8;
      break;
    }
    case kAMSampleRate16K: {
      alsa_data_format->sample_rate = ALSA_SAMPLINGRATE_16;
      break;
    }
    case kAMSampleRate24K: {
      alsa_data_format->sample_rate = ALSA_SAMPLINGRATE_24;
      break;
    }
    case kAMSampleRate32K: {
      alsa_data_format->sample_rate = ALSA_SAMPLINGRATE_32;
      break;
    }
    case kAMSampleRate44K1: {
      alsa_data_format->sample_rate = ALSA_SAMPLINGRATE_441;
      break;
    }
    case kAMSampleRate48K: {
      alsa_data_format->sample_rate = ALSA_SAMPLINGRATE_48;
      break;
    }
    default: {
      KLOGI(TAG, "Use default sample rate 16K.");
      alsa_data_format->sample_rate = ALSA_SAMPLINGRATE_16;
    }
  }

  switch (am_data_format->format_type) {
    case kAMDataFormatPCMNonInterleaved:
    case kAMDataFormatPCMInterleaved: {
      alsa_data_format->format_type = SND_PCM_ACCESS_RW_INTERLEAVED;
      break;
    }
    default: {
      KLOGI(TAG, "Use default format type PCM interleaved");
      alsa_data_format->format_type = SND_PCM_ACCESS_RW_INTERLEAVED;
    }
  }

  switch (am_data_format->bits_per_sample) {
    case kAMSampleFormatFixed8: {
      alsa_data_format->bit_per_sample = SND_PCM_FORMAT_S8;
      alsa_data_format->byte_per_sample = 1;
      break;
    }
    case kAMSampleFormatFixed16: {
      alsa_data_format->bit_per_sample = SND_PCM_FORMAT_S16_LE;
      alsa_data_format->byte_per_sample = 2;
      break;
    }
    case kAMSampleFormatFixed24: {
      alsa_data_format->bit_per_sample = SND_PCM_FORMAT_S24_LE;
      alsa_data_format->byte_per_sample = 3;
      break;
    }
    case kAMSampleFormatFixed32: {
      alsa_data_format->bit_per_sample = SND_PCM_FORMAT_S32_LE;
      alsa_data_format->byte_per_sample = 4;
      break;
    }
    default: {
      KLOGI(TAG, "Use default bits per sample 16bits.");
      alsa_data_format->bit_per_sample = SND_PCM_FORMAT_S16_LE;
      alsa_data_format->byte_per_sample = 2;
    }
  }

  alsa_data_format->num_channels = am_data_format->num_channels;

  return kAMSuccess;
}

char* AlsaEngine::audio_get_version() {
  AlsaStream *alsa_stream = global_alsa_in;
  char *engine_version = kAudioEngineVersion;
  int result = -kAMUnknownError;
  if (NULL == alsa_stream)
    alsa_stream = global_alsa_out;
  if (NULL == alsa_stream) {
    KLOGE(TAG, "Donnot open any output device or input device!");
    return engine_version;
  }

  //result = alsa_get_version(alsa_stream, engine_version);
  return engine_version;
}

AMResult AlsaEngine::audio_output_set_vol(int vol, int player_id) {
  AlsaStream *alsa_stream = global_alsa_out;
  int result = -kAMUnknownError;
  if (NULL == alsa_stream) return -kAMResourceError;

  //result = alsa_set_player_vol(alsa_stream, vol, player_id);
  return result;
}

AMResult AlsaEngine::audio_output_get_vol(int player_id) {
  AlsaStream *alsa_stream = global_alsa_out;
  int result = -kAMUnknownError;
  if (NULL == alsa_stream) return -kAMResourceError;

  //result = alsa_get_player_vol(alsa_stream, player_id);
  return result;
}

AMResult AlsaEngine::audio_output_set_mute(int player_id, bool mute) {
  AlsaStream *alsa_stream = global_alsa_out;
  int result = kAMUnknownError;
  if (NULL == alsa_stream) return -kAMResourceError;

  //result = alsa_set_player_mute(alsa_stream, player_id, mute);
  return result;
}

void AlsaEngine::audio_registerListener(const AMEventListener &listener) {
  audio_listener_.audio_event_callback = listener.audio_event_callback;
  audio_listener_.user_data = listener.user_data;
}

AMResult AlsaEngine::audio_output_open(AMDataFormat *out) {
  AlsaStream *alsa_stream = NULL;
  AMResult result = -1;
  int player_id = 0;

  if (NULL == global_alsa_in && NULL == global_alsa_out) {
    alsa_stream = (AlsaStream *)malloc(sizeof(AlsaStream));
    KLOGD(TAG, "Create a new Alsa_Stream for output.");
    if (NULL == alsa_stream) {
      KLOGE(TAG, "Create Alsa_Stream failed!");
      return -kAMMemoryFailure;
    }
    memset(alsa_stream, 0, sizeof(AlsaStream));
  } else if (global_alsa_in != NULL) {
    alsa_stream = global_alsa_in;
    KLOGD(TAG, "Alsa_Stream of input has been existed. player use the same.");
  } else if (global_alsa_out != NULL) {
    KLOGD(TAG, "Alsa_Stream of output has been existed. player use the same.");
  }

  if (global_alsa_out != NULL) return kAMSuccess;

  if (NULL == alsa_stream) {
    KLOGE(TAG, "player create Alsa_Stream for player failed!");
    return -kAMMemoryFailure;
  }
  global_alsa_out = alsa_stream;

  // parse AMDataFormat into AlsaStream
  parse_data_format(out, &alsa_stream->player);

  // snd_device open and set paramters
  result = alsa_output_open(&alsa_stream->player);
  if (result != kAMSuccess) {
    KLOGE(TAG, "audio_output_open failed!");
  }

  return result;
}

AMResult AlsaEngine::audio_output_close(int player_id) {
  int result = -1;
  AlsaStream *alsa_stream = global_alsa_out;
  if (NULL == alsa_stream) return kAMSuccess;

  // close alsa player
  result = alsa_output_close(&alsa_stream->player);
  if (result != kAMSuccess) {
    KLOGE(TAG, "audio_output_close failed!");
  }

  if (NULL == global_alsa_in && NULL == global_alsa_out) free(alsa_stream);
  global_alsa_out = NULL;

  return kAMSuccess;
}

AMBufferCount AlsaEngine::audio_output_write(void *src_buffer,
                                             const int buffer_size,
                                             int player_id) {
  AlsaStream *alsa_stream = global_alsa_out;
  char *write_buf = (char *)src_buffer;
  int bytes = 0;
  int result = -1;

  if (NULL == alsa_stream) return -kAMResourceError;
  if (alsa_stream->player.audio_state != ALSA_PLAYSTATE_PLAYING) {
    result = alsa_output_start(&alsa_stream->player);
    if (result != kAMSuccess) {
      KLOGE(TAG, "audio_output_write set state failed!");
      return bytes;
    }
  }

  if (!write_check(this, alsa_stream->player.ring_buffer, buffer_size)) {
    return 0;
  }
  pthread_mutex_lock(&alsa_stream->player.mutex);
  bytes = write_circular_buffer_bytes(
      alsa_stream->player.ring_buffer, write_buf, buffer_size);
  pthread_mutex_unlock(&alsa_stream->player.mutex);

  return bytes;
}

AMBufferCount AlsaEngine::audio_output_getBufferCount(int player_id) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_output_pause(int player_id) {
  int result = -1;
  AlsaStream *alsa_stream = global_alsa_out;
  if (NULL == alsa_stream) return kAMSuccess;

  // pause alsa player
  result = alsa_output_pause(&alsa_stream->player);
  if (result != kAMSuccess) {
    KLOGE(TAG, "audio_output_pause failed!");
  }

  if (NULL == global_alsa_in && NULL == global_alsa_out) free(alsa_stream);

  return kAMSuccess;
}

AMResult AlsaEngine::audio_output_stop(int player_id, bool drain) {
  int result = -1;
  AlsaStream *alsa_stream = global_alsa_out;
  if (NULL == alsa_stream) return kAMSuccess;

  // stop alsa player
  result = alsa_output_stop(&alsa_stream->player, drain);
  if (result != kAMSuccess) {
    KLOGE(TAG, "audio_output_stop failed!");
  }
  if (NULL == global_alsa_in && NULL == global_alsa_out) free(alsa_stream);

  return kAMSuccess;
}

AMResult AlsaEngine::audio_output_getAudioFormat(AMDataFormat *data_format,
                                                   int player_id) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_output_getAudioStatus(AMAudioStatus *audio_status,
                                                   int player_id) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_output_setAudioStatus(AMAudioStatus *audio_status,
                                                   int player_id) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_outputFromFile_open(AMFileInfo *file_info,
                                               AMDataFormat *data_format) {
  AlsaStream *alsa_stream = NULL;
  AMResult result = -1;
  int player_id = 0;

  if (NULL == global_alsa_in && NULL == global_alsa_out) {
    alsa_stream = (AlsaStream *)malloc(sizeof(AlsaStream));
    KLOGD(TAG, "Create a new Alsa_Stream for output.");
    if (NULL == alsa_stream) {
      KLOGE(TAG, "Create Alsa_Stream failed!");
      return -kAMMemoryFailure;
    }
    memset(alsa_stream, 0, sizeof(AlsaStream));
  } else if (global_alsa_in != NULL) {
    alsa_stream = global_alsa_in;
    KLOGD(TAG, "Alsa_Stream of input has been existed. Use the same.");
  } else if (global_alsa_out != NULL) {
    alsa_stream = global_alsa_out;
    KLOGD(TAG, "Alsa_Stream of output has been existed. Use the same.");
  }
  global_alsa_out = alsa_stream;

  // parse AMDataFormat into AlsaStream
  parse_data_format(data_format, &alsa_stream->player);
  alsa_stream->player.play_type = file_info->file_type;
  alsa_stream->player.locator = file_info->file_path;
  alsa_stream->player.start = file_info->start;
  alsa_stream->player.length = file_info->length;

  // snd_device open and set paramters
  result = alsa_output_open(&alsa_stream->player);
  if (result != kAMSuccess) {
    KLOGE(TAG, "audio_output_open failed!");
  }

  return result;
}

AMResult AlsaEngine::audio_outputFromFile_close(int player_id) {
  int result = -1;
  AlsaStream *alsa_stream = global_alsa_out;
  if (NULL == alsa_stream) return kAMSuccess;

  // close alsa player
  result = alsa_output_close(&alsa_stream->player);
  if (result != kAMSuccess) {
    KLOGE(TAG, "audio_output_close failed!");
  }

  if (NULL == global_alsa_in && NULL == global_alsa_out) free(alsa_stream);

  return kAMSuccess;
}

AMResult AlsaEngine::audio_outputFromFile_start(int player_id) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_outputFromFile_pause(int player_id) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_outputFromFile_stop(int player_id) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_outputFromFile_getAudioFormat(
    AMDataFormat *data_format,
    int player_id) {
  return kAMSuccess;
}
AMResult AlsaEngine::audio_outputFromFile_getAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_outputFromFile_setAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_input_open(AMDataFormat *data_format) {
  AlsaStream *alsa_stream = NULL;
  AMResult result = -1;

  if (NULL == global_alsa_in && NULL == global_alsa_out) {
    alsa_stream = (AlsaStream *)malloc(sizeof(AlsaStream));
    KLOGD(TAG, "Create a new Alsa_Stream for input.");
  } else if (global_alsa_out != NULL) {
    alsa_stream = global_alsa_out;
    KLOGD(TAG, "Alsa_Stream of output has been existed. recorder use the same.");
  } else if (global_alsa_in != NULL) {
    KLOGD(TAG, "Alsa_Stream of input has been existed. recorder use the same.");
  }

  if (global_alsa_in != NULL) return kAMSuccess;

  if (NULL == alsa_stream) {
    KLOGE(TAG, "Recorder create Alsa_Stream failed!");
    return -kAMMemoryFailure;
  }
  global_alsa_in = alsa_stream;

  // parse AMDataFormat into AlsaStream
  parse_data_format(data_format, &alsa_stream->capture);

  // snd_device open and set paramters
  result = alsa_input_open(&alsa_stream->capture);
  if (result != kAMSuccess) {
    KLOGE(TAG, "audio_input_open failed!");
  }

  return result;
}

AMResult AlsaEngine::audio_input_close() {
  int result = -1;
  AlsaStream *alsa_stream = global_alsa_in;
  if (NULL == alsa_stream) return kAMSuccess;

  // close alsa capture
  result = alsa_input_close(&alsa_stream->capture);

  if (NULL == global_alsa_in && NULL == global_alsa_out) free(alsa_stream);
  global_alsa_in = NULL;

  return kAMSuccess;
}

AMBufferCount AlsaEngine::audio_input_read(void *read_buffer,
                                             int buffer_size) {
  AlsaStream *alsa_stream = global_alsa_in;
  char *buffer = (char *)read_buffer;
  int bytes = 0;
  int result = -1;

  if (NULL == alsa_stream) return -kAMResourceError;

  if (alsa_stream->capture.audio_state != ALSA_CAPTURESTATE_CAPTURING) {
    result = alsa_input_start(&alsa_stream->capture);
    if (result != kAMSuccess) {
      KLOGE(TAG, "audio_input_read set state failed!");
      return bytes;
    }
  }

  pthread_mutex_lock(&alsa_stream->capture.mutex);
  bytes = read_circular_buffer_bytes(alsa_stream->capture.ring_buffer,
                                     buffer,
                                     buffer_size);
  pthread_mutex_unlock(&alsa_stream->capture.mutex);
  return bytes;
}

AMResult AlsaEngine::audio_input_pause() {
  int result = -1;
  AlsaStream *alsa_stream = global_alsa_in;
  if (NULL == alsa_stream) return kAMSuccess;

  // pause alsa capture
  result = alsa_input_pause(&alsa_stream->capture);
  if (result != kAMSuccess) {
    KLOGE(TAG, "alsa_input_pause failed!(%d)", result);
  }

  if (NULL == global_alsa_in && NULL == global_alsa_out) free(alsa_stream);

  return kAMSuccess;
}

AMResult AlsaEngine::audio_input_stop() {
  int result = -1;
  AlsaStream *alsa_stream = global_alsa_in;
  if (NULL == alsa_stream) return kAMSuccess;

  // stop alsa capture
  result = alsa_input_stop(&alsa_stream->capture);
  if (result != kAMSuccess) {
    KLOGE(TAG, "audio_input_stop failed!");
  }

  if (NULL == global_alsa_in && NULL == global_alsa_out) free(alsa_stream);

  return kAMSuccess;
}

AMResult AlsaEngine::audio_input_getAudioFormat(AMDataFormat *data_format) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_input_getAudioStatus(AMAudioStatus *audio_status) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_input_setAudioStatus(AMAudioStatus *audio_status) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_inputToFile_open(AMFileInfo *file_info,
                                              AMDataFormat *data_format) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_inputToFile_close() {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_inputToFile_start() {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_inputToFile_pause() {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_inputToFile_stop() {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_inputToFile_getAudioFormat(
    AMDataFormat *data_format) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_inputToFile_getAudioStatus(
    AMAudioStatus *audio_status) {
  return kAMSuccess;
}

AMResult AlsaEngine::audio_inputToFile_setAudioStatus(
    AMAudioStatus *audio_status) {
  return kAMSuccess;
}

} // namespace nuiam

