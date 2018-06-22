/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/engine/audio_openSL/audio_opensl_impl.cc
 */

#include "opensl_io.h"
#include "audio_common.h"
#include "audio_log.h"
#include "audio_opensl_impl.h"

namespace audiomanager {

static OpenslStream *global_opensl_in = NULL;
static OpenslStream *global_opensl_out = NULL;
const int kBufferFrames = 1024;

// just parse sample rate, format type, bits per sample and endianness
static AMResult parse_data_format(AMDataFormat *am_data_format, 
                                  SLDataFormat_PCM *sl_data_format) {
  switch (am_data_format->sample_rate) {
    case kAMSampleRate8K: {
      sl_data_format->samplesPerSec = SL_SAMPLINGRATE_8;
      break;
    }
    case kAMSampleRate16K: {
      sl_data_format->samplesPerSec = SL_SAMPLINGRATE_16;
      break;
    }
    case kAMSampleRate32K: {
      sl_data_format->samplesPerSec = SL_SAMPLINGRATE_32;
      break;
    }
    case kAMSampleRate44K1: {
      sl_data_format->samplesPerSec = SL_SAMPLINGRATE_44_1;
      break;
    }
    case kAMSampleRate48K: {
      sl_data_format->samplesPerSec = SL_SAMPLINGRATE_48;
      break;
    }
    default: {
      LOGI("Use default sample rate 16K.");
      sl_data_format->samplesPerSec = SL_SAMPLINGRATE_48;
    }
  }

  switch (am_data_format->format_type) {
    case kAMDataFormatPCMNonInterleaved:
    case kAMDataFormatPCMInterleaved: {
      sl_data_format->formatType = SL_DATAFORMAT_PCM;
      break;
    }
    default: {
      LOGI("Use default format type PCM.");
      sl_data_format->formatType = SL_DATAFORMAT_PCM;
    }
  }

  switch (am_data_format->bits_per_sample) {
    case kAMSampleFormatFixed8: {
      sl_data_format->bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_8;
      break;
    }
    case kAMSampleFormatFixed16: {
      sl_data_format->bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
      break;
    }
    case kAMSampleFormatFixed24: {
      sl_data_format->bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_24;
      break;
    }
    case kAMSampleFormatFixed32: {
      sl_data_format->bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_32;
      break;
    }
    default: {
      LOGI("Use default bits per sample 16bits.");
      sl_data_format->bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    }
  }

  if (kAMByteOrderLittleEndian == am_data_format->endianness)
    sl_data_format->endianness = SL_BYTEORDER_LITTLEENDIAN;
  else if (kAMByteOrderBigEndian == am_data_format->endianness)
    sl_data_format->endianness = SL_BYTEORDER_BIGENDIAN;
  else {
    LOGI("Use default endiannes little endian.");
    sl_data_format->endianness = SL_BYTEORDER_LITTLEENDIAN;
  }

  if (am_data_format->num_channels > 1) {
    sl_data_format->channelMask =
        SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
  } else {
    sl_data_format->channelMask = SL_SPEAKER_FRONT_CENTER;
  }

  sl_data_format->numChannels = am_data_format->num_channels;

  return kAMSuccess;
}

char* OpenslEngine::audio_get_version() {
  OpenslStream *opensl_stream = global_opensl_in;
  char *engine_version = kAudioEngineVersion;
  int result = -SL_RESULT_UNKNOWN_ERROR;
  if (NULL == opensl_stream)
    opensl_stream = global_opensl_out;
  if (NULL == opensl_stream) {
    LOGE("Donnot open any output device or input device!");
    return engine_version;
  }

  result = openSL_get_version(opensl_stream, engine_version);
  return engine_version;
}

AMResult OpenslEngine::audio_output_set_vol(int vol, int player_id) {
  OpenslStream *opensl_stream = global_opensl_out;
  int result = -SL_RESULT_UNKNOWN_ERROR;
  if (NULL == opensl_stream) return -kAMResourceError;

  result = openSL_set_player_vol(opensl_stream, vol, player_id);
  return -result;
}

AMResult OpenslEngine::audio_output_get_vol(int player_id) {
  OpenslStream *opensl_stream = global_opensl_out;
  int result = -SL_RESULT_UNKNOWN_ERROR;
  if (NULL == opensl_stream) return -kAMResourceError;

  result = openSL_get_player_vol(opensl_stream, player_id);
  return -result;
}

AMResult OpenslEngine::audio_output_set_mute(int player_id, bool mute) {
  OpenslStream *opensl_stream = global_opensl_out;
  int result = -SL_RESULT_UNKNOWN_ERROR;
  if (NULL == opensl_stream) return -kAMResourceError;

  result = openSL_set_player_mute(opensl_stream, player_id, mute);
  return -result;
}

AMResult OpenslEngine::audio_output_open(AMDataFormat *out) {
  OpenslStream *opensl_stream = NULL;
  AMResult result = -1;
  int player_id = 0;

  if (NULL == global_opensl_in && NULL == global_opensl_out) {
    opensl_stream = (OpenslStream *)malloc(sizeof(OpenslStream));
    LOGD("Create a new OpenSL_Stream for output.");
    if (NULL == opensl_stream) {
      LOGE("Create OpenSL_Stream failed!");
      return -kAMMemoryFailure;
    }
    memset(opensl_stream, 0, sizeof(OpenslStream));
  } else if (global_opensl_in != NULL) {
    opensl_stream = global_opensl_in;
    LOGD("OpenSL_Stream of input has been existed. Use the same.");
  } else if (global_opensl_out != NULL) {
    opensl_stream = global_opensl_out;
    LOGD("OpenSL_Stream of output has been existed. Use the same.");
  }
  global_opensl_out = opensl_stream;

  // generate player id
  for (player_id = 0; player_id < PLAYER_MAX; player_id++) {
    if (0 == ((opensl_stream->player_channel_mask >> player_id) & 0x1)) {
      LOGD("Player ID (%d) is usable at present.", player_id);
      break;
    }
  }
  if (player_id >= PLAYER_MAX) {
    LOGI("Have exhausted all player.");
    return -kAMParameterInvalid;
  }

  // parse AMDataFormat into OpenslStream
  parse_data_format(out, &opensl_stream->out_format_pcm[player_id]);
  opensl_stream->out_channels[player_id] = out->num_channels;
  opensl_stream->outlock[player_id] = createThreadLock();

  // create some resource
  opensl_stream->out_buf_samples[player_id] = kBufferFrames * out->num_channels;
  if (opensl_stream->out_buf_samples[player_id] != 0) {
    opensl_stream->output_buf[player_id][0] = (char *)calloc(
        opensl_stream->out_buf_samples[player_id] * sizeof(short),
        sizeof(char));
    opensl_stream->output_buf[player_id][1] = (char *)calloc(
        opensl_stream->out_buf_samples[player_id] * sizeof(short),
        sizeof(char));
    if (NULL == opensl_stream->output_buf[player_id][0] ||
        NULL == opensl_stream->output_buf[player_id][1]) {
      LOGE("Output buffer of OpenSL_Stream calloc failed!");
      audio_output_close(player_id);
      return -kAMMemoryFailure;
    }
  }
  opensl_stream->cur_output_buf[player_id] = 0;
  opensl_stream->cur_output_index[player_id] =
      opensl_stream->out_buf_samples[player_id];

  // create engine, in and out just create once
  if (NULL == global_opensl_in && 0 == opensl_stream->player_num) {
    result = openSL_create_engine(opensl_stream);
    if (result != SL_RESULT_SUCCESS) {
      LOGE("Create openSL engine failed! result = %d.", result);
      audio_output_close(player_id);
      return -result;
    } else {
      LOGD("Create openSL engine success!");
    }
  }

  // create output mix
  if (0 == opensl_stream->player_num) {
    result = openSL_create_output_mix(opensl_stream);
    if (result != SL_RESULT_SUCCESS) {
      LOGE("Create openSL outputMix failed! result = %d.", result);
      audio_output_close(player_id);
      return -result;
    } else {
      LOGD("Create openSL outputMix success!");
    }
  }

  // config player and create player
  result = openSL_init_player(opensl_stream, player_id);
  if (result != SL_RESULT_SUCCESS) {
    LOGE("Create openSL player(%d) failed! result = %d.", player_id, result);
    audio_output_close(player_id);
    return -result;
  } else {
    LOGD("Create openSL player(%d) success!", player_id);
    opensl_stream->player_channel_mask |= (1<<player_id);
    opensl_stream->player_num++;
    // return player id
    result = player_id;
  }

  notifyThreadLock(opensl_stream->outlock[player_id]);
  opensl_stream->time = 0.;

  return result;
}

AMResult OpenslEngine::audio_output_close(int player_id) {
  int result = -1;
  OpenslStream *opensl_stream = global_opensl_out;
  if (NULL == opensl_stream) return kAMSuccess;

  // remove openSL player
  result = openSL_remove_player(opensl_stream, player_id);
  if (kAMSuccess == result) {
    if (opensl_stream->player_num != 0) opensl_stream->player_num--;
    opensl_stream->player_channel_mask &= ~(1<<player_id);
  } else {
    LOGE("This player was null.");
    return -result;
  }
  // destroy output mix
  if (0 == opensl_stream->player_num) {
    openSL_destroy_output_mix(opensl_stream);
    global_opensl_out = NULL;
  }
  // destroy openSL engine
  if (NULL == global_opensl_in && 0 == opensl_stream->player_num)
    openSL_destroy_engine(opensl_stream);
  // clear some source
  if (opensl_stream->output_buf[player_id][0] != NULL) {
    free(opensl_stream->output_buf[player_id][0]);
    opensl_stream->output_buf[player_id][0] = NULL;
  }
  if (opensl_stream->output_buf[player_id][1] != NULL) {
    free(opensl_stream->output_buf[player_id][1]);
    opensl_stream->output_buf[player_id][1] = NULL;
  }
  if (NULL == global_opensl_in && NULL == global_opensl_out) free(opensl_stream);

  return kAMSuccess;
}

AMBufferCount OpenslEngine::audio_output_write(void *src_buffer,
                                               const int buffer_size,
                                               int player_id) {
  OpenslStream *opensl_stream = global_opensl_out;
  int bytes = 0;
  int result = -1;

  if (NULL == opensl_stream) return -kAMResourceError;
  if (!(opensl_stream->player_channel_mask & (1<<player_id))) {
    LOGE("Player id(%d) is invalid. Player channel mask is 0x%x",
        player_id,
        opensl_stream->player_channel_mask);
    return -kAMParameterInvalid;
  }

  if (opensl_stream->output_state[player_id] != SL_PLAYSTATE_PLAYING) {
    result = openSL_set_player_state(opensl_stream,
                                     player_id,
                                     SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) {
      LOGE("Output set player state playing failed.(%d)", result);
      return bytes;
    }
    opensl_stream->output_state[player_id] = SL_PLAYSTATE_PLAYING;
  }

  bytes = openSL_write_enqueue(opensl_stream,
                               src_buffer,
                               buffer_size,
                               player_id);
  return bytes;
}

AMBufferCount OpenslEngine::audio_output_getBufferCount(int player_id) {
  return kAMSuccess;
}

AMResult OpenslEngine::audio_output_pause(int player_id) {
  OpenslStream *opensl_stream = global_opensl_out;
  if (opensl_stream != NULL) {
    openSL_set_player_state(opensl_stream, player_id, SL_PLAYSTATE_PAUSED);
    opensl_stream->output_state[player_id] = SL_PLAYSTATE_PAUSED;
    return kAMSuccess;
  } else {
    return -kAMResourceLost;
  }
}

AMResult OpenslEngine::audio_output_stop(int player_id) {
  OpenslStream *opensl_stream = global_opensl_out;
  if (opensl_stream != NULL) {
    openSL_set_player_state(opensl_stream, player_id, SL_PLAYSTATE_STOPPED);
    opensl_stream->output_state[player_id] = SL_PLAYSTATE_STOPPED;
    return kAMSuccess;
  } else {
    return -kAMResourceLost;
  }
}

AMResult OpenslEngine::audio_output_getAudioFormat(AMDataFormat *data_format,
                                                   int player_id) {
  return kAMSuccess;
}

AMResult OpenslEngine::audio_output_getAudioStatus(AMAudioStatus *audio_status,
                                                   int player_id) {
  return kAMSuccess;
}

AMResult OpenslEngine::audio_output_setAudioStatus(AMAudioStatus *audio_status,
                                                   int player_id) {
  return kAMSuccess;
}

AMResult OpenslEngine::audio_outputFromFile_open(AMFileInfo *file_info,
                                                 AMDataFormat *data_format) {
  OpenslStream *opensl_stream = NULL;
  AMResult result = -1;
  int player_id = 0;

  if (NULL == global_opensl_in && NULL == global_opensl_out) {
    opensl_stream = (OpenslStream *)malloc(sizeof(OpenslStream));
    LOGD("Create a new OpenSL_Stream for output.");
    if (NULL == opensl_stream) {
      LOGE("Create OpenSL_Stream failed!");
      return -kAMMemoryFailure;
    }
    memset(opensl_stream, 0, sizeof(OpenslStream));
  } else if (global_opensl_in != NULL) {
    opensl_stream = global_opensl_in;
    LOGD("OpenSL_Stream of input has been existed. Use the same.");
  } else if (global_opensl_out != NULL) {
    opensl_stream = global_opensl_out;
    LOGD("OpenSL_Stream of output has been existed. Use the same.");
  }
  global_opensl_out = opensl_stream;

  // generate player id
  for (player_id = 0; player_id < PLAYER_MAX; player_id++) {
    if (0 == ((opensl_stream->player_channel_mask >> player_id) & 0x1)) {
      LOGD("Player ID (%d) is usable at present.", player_id);
      break;
    }
  }
  if (player_id >= PLAYER_MAX) {
    LOGI("Have exhausted all player.");
    return -kAMParameterInvalid;
  }

  // parse AMDataFormat into OpenslStream
  parse_data_format(data_format, &opensl_stream->out_format_pcm[player_id]);
  opensl_stream->out_channels[player_id] = data_format->num_channels;
  opensl_stream->play_type[player_id] = file_info->file_type;
  opensl_stream->locator = file_info->file_path;
  opensl_stream->fd = file_info->fd;
  opensl_stream->start = file_info->start;
  opensl_stream->length = file_info->length;

  // create engine, in and out just create once
  if (NULL == global_opensl_in && 0 == opensl_stream->player_num) {
    result = openSL_create_engine(opensl_stream);
    if (result != SL_RESULT_SUCCESS) {
      LOGE("Create openSL engine failed! result = %d.", result);
      audio_outputFromFile_close(player_id);
      return -result;
    } else {
      LOGD("Create openSL engine success!");
    }
  }

  // create output mix
  if (0 == opensl_stream->player_num) {
    result = openSL_create_output_mix(opensl_stream);
    if (result != SL_RESULT_SUCCESS) {
      LOGE("Create openSL outputMix failed! result = %d.", result);
      audio_outputFromFile_close(player_id);
      return -result;
    } else {
      LOGD("Create openSL outputMix success!");
    }
  }

  // config player and create player
  result = openSL_init_player(opensl_stream, player_id);
  if (result != SL_RESULT_SUCCESS) {
    LOGE("Create openSL player(%d) failed! result = %d.", player_id, result);
    audio_outputFromFile_close(player_id);
    return -result;
  } else {
    LOGD("Create openSL player(%d) success!", player_id);
    opensl_stream->player_channel_mask |= (1<<player_id);
    opensl_stream->player_num++;
    // return player id
    result = player_id;
  }

  return -result;
}

AMResult OpenslEngine::audio_outputFromFile_close(int player_id) {
  int result = -1;
  OpenslStream *opensl_stream = global_opensl_out;
  if (NULL == opensl_stream) return kAMSuccess;

  // remove openSL player
  result = openSL_remove_player(opensl_stream, player_id);
  if (kAMSuccess == result) {
    if (opensl_stream->player_num != 0) opensl_stream->player_num--;
    opensl_stream->player_channel_mask &= ~(1<<player_id);
  } else {
    LOGE("This player was null.");
    return -result;
  }
  // destroy output mix
  if (0 == opensl_stream->player_num) {
    openSL_destroy_output_mix(opensl_stream);
    global_opensl_out = NULL;
  }
  // destroy openSL engine
  if (NULL == global_opensl_in && 0 == opensl_stream->player_num) {
    openSL_destroy_engine(opensl_stream);
    free(opensl_stream);
  }
  return kAMSuccess;
}

AMResult OpenslEngine::audio_outputFromFile_start(int player_id) {
  OpenslStream *opensl_stream = global_opensl_out;
  if (opensl_stream != NULL) {
    openSL_set_player_state(opensl_stream, player_id, SL_PLAYSTATE_PLAYING);
    return kAMSuccess;
  } else {
    return -kAMResourceLost;
  }
}

AMResult OpenslEngine::audio_outputFromFile_pause(int player_id) {
  OpenslStream *opensl_stream = global_opensl_out;
  if (opensl_stream != NULL) {
    openSL_set_player_state(opensl_stream, player_id, SL_PLAYSTATE_PAUSED);
    return kAMSuccess;
  } else {
    return -kAMResourceLost;
  }
}

AMResult OpenslEngine::audio_outputFromFile_stop(int player_id) {
  OpenslStream *opensl_stream = global_opensl_out;
  if (opensl_stream != NULL) {
    openSL_set_player_state(opensl_stream, player_id, SL_PLAYSTATE_STOPPED);
    return kAMSuccess;
  } else {
    return -kAMResourceLost;
  }
}

AMResult OpenslEngine::audio_outputFromFile_getAudioFormat(
    AMDataFormat *data_format,
    int player_id) {
  return kAMSuccess;
}
AMResult OpenslEngine::audio_outputFromFile_getAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  return kAMSuccess;
}

AMResult OpenslEngine::audio_outputFromFile_setAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  return kAMSuccess;
}

AMResult OpenslEngine::audio_input_open(AMDataFormat *data_format) {
  OpenslStream *opensl_stream = NULL;
  AMResult result = -1;

  if (NULL == global_opensl_in && NULL == global_opensl_out) {
    opensl_stream = (OpenslStream *)malloc(sizeof(OpenslStream));
    LOGD("Create a new OpenSL_Stream for input.");
  } else if (global_opensl_out != NULL) {
    opensl_stream = global_opensl_out;
    LOGD("OpenSL_Stream of output has been existed. Use the same.");
  } else if (global_opensl_in != NULL) {
    opensl_stream = global_opensl_in;
    LOGD("OpenSL_Stream of input has been existed. Use the same.");
  }

  if (NULL == opensl_stream) {
    LOGE("Create OpenSL_Stream failed!");
    return -kAMMemoryFailure;
  }
  global_opensl_in = opensl_stream;

  // parse AMDataFormat into OpenslStream
  parse_data_format(data_format, &opensl_stream->in_format_pcm);
  opensl_stream->in_channels = data_format->num_channels;
  opensl_stream->inlock = createThreadLock();

  // create some resource
  opensl_stream->in_buf_samples = kBufferFrames * data_format->num_channels;
  if (opensl_stream->in_buf_samples != 0) {
    opensl_stream->input_buf[0] = (char *)calloc(
        opensl_stream->in_buf_samples * sizeof(short),
        sizeof(char));
    opensl_stream->input_buf[1] = (char *)calloc(
        opensl_stream->in_buf_samples * sizeof(short),
        sizeof(char));
    if (NULL == opensl_stream->input_buf[0] ||
        NULL == opensl_stream->input_buf[1]) {
      LOGE("Input buffer of OpenSL_Stream calloc failed!");
      audio_input_close();
      return -kAMMemoryFailure;
    }
  }
  opensl_stream->cur_input_buf = 0;
  opensl_stream->cur_input_index = opensl_stream->in_buf_samples;

  // create engine, in and out just create once.
  if (NULL == global_opensl_out) {
    result = openSL_create_engine(opensl_stream);
    if (result != SL_RESULT_SUCCESS) {
      LOGE("Create openSL engine failed! result = %d.", result);
      audio_input_close();
      return -result;
    } else {
      LOGD("Create openSL engine success!");
    }
  }

  // config recorder and create recorder
  result = openSL_init_recorder(opensl_stream);
  if (result != SL_RESULT_SUCCESS) {
    LOGE("Create openSL recorder failed! result = %d.", result);
    audio_input_close();
    return -result;
  } else {
    LOGD("Create openSL recorder success!");
  }

  notifyThreadLock(opensl_stream->inlock);
  opensl_stream->time = 0.;

  return result;
}

AMResult OpenslEngine::audio_input_close() {
  OpenslStream *opensl_stream = global_opensl_in;

  if (NULL == opensl_stream)
    return kAMSuccess;

  openSL_remove_recorder(opensl_stream);
  if (NULL == global_opensl_out)
    openSL_destroy_engine(opensl_stream);

  if (opensl_stream->input_buf[0] != NULL) {
    free(opensl_stream->input_buf[0]);
    opensl_stream->input_buf[0] = NULL;
  }
  if (opensl_stream->input_buf[1] != NULL) {
    free(opensl_stream->input_buf[1]);
    opensl_stream->input_buf[1] = NULL;
  }

  global_opensl_in = NULL;
  if (NULL == global_opensl_out)
    free(opensl_stream);

  return kAMSuccess;
}

AMBufferCount OpenslEngine::audio_input_read(void *read_buffer,
                                             int buffer_size) {
  OpenslStream *opensl_stream = global_opensl_in;
  int bytes = 0;
  if (NULL == opensl_stream || 0 == opensl_stream->in_buf_samples)
    return kAMSuccess;
  if (opensl_stream->input_state != SL_RECORDSTATE_RECORDING) {
    openSL_set_recorder_state(opensl_stream, SL_RECORDSTATE_RECORDING);
    opensl_stream->input_state = SL_RECORDSTATE_RECORDING;
  }
  bytes = openSL_read_enqueue(opensl_stream, read_buffer, buffer_size);
  return bytes;
}

AMResult OpenslEngine::audio_input_pause() {
  OpenslStream *opensl_stream = global_opensl_in;
  if (opensl_stream != NULL) {
    openSL_set_recorder_state(opensl_stream, SL_RECORDSTATE_PAUSED);
    opensl_stream->input_state = SL_RECORDSTATE_PAUSED;
    return kAMSuccess;
  } else {
    return -kAMResourceLost;
  }
}

AMResult OpenslEngine::audio_input_stop() {
  OpenslStream *opensl_stream = global_opensl_in;
  if (opensl_stream != NULL) {
    openSL_set_recorder_state(opensl_stream, SL_RECORDSTATE_STOPPED);
    opensl_stream->input_state = SL_RECORDSTATE_STOPPED;
    return kAMSuccess;
  } else {
    return -kAMResourceLost;
  }
}

AMResult OpenslEngine::audio_input_getAudioFormat(AMDataFormat *data_format) {
  return kAMSuccess;
}

AMResult OpenslEngine::audio_input_getAudioStatus(AMAudioStatus *audio_status) {
  return kAMSuccess;
}

AMResult OpenslEngine::audio_input_setAudioStatus(AMAudioStatus *audio_status) {
  return kAMSuccess;
}

AMResult OpenslEngine::audio_inputToFile_open(AMFileInfo *file_info,
                                              AMDataFormat *data_format) {
  return kAMSuccess;
}

AMResult OpenslEngine::audio_inputToFile_close() {
  return kAMSuccess;
}

AMResult OpenslEngine::audio_inputToFile_start() {
  OpenslStream *opensl_stream = global_opensl_in;
  if (opensl_stream != NULL) {
    openSL_set_recorder_state(opensl_stream, SL_RECORDSTATE_RECORDING);
    return kAMSuccess;
  } else {
    return -kAMResourceLost;
  }
}

AMResult OpenslEngine::audio_inputToFile_pause() {
  OpenslStream *opensl_stream = global_opensl_in;
  if (opensl_stream != NULL) {
    openSL_set_recorder_state(opensl_stream, SL_RECORDSTATE_PAUSED);
    return kAMSuccess;
  } else {
    return -kAMResourceLost;
  }
}

AMResult OpenslEngine::audio_inputToFile_stop() {
  OpenslStream *opensl_stream = global_opensl_in;
  if (opensl_stream != NULL) {
    openSL_set_recorder_state(opensl_stream, SL_RECORDSTATE_STOPPED);
    return kAMSuccess;
  } else {
    return -kAMResourceLost;
  }
}

AMResult OpenslEngine::audio_inputToFile_getAudioFormat(
    AMDataFormat *data_format) {
  return kAMSuccess;
}

AMResult OpenslEngine::audio_inputToFile_getAudioStatus(
    AMAudioStatus *audio_status) {
  return kAMSuccess;
}

AMResult OpenslEngine::audio_inputToFile_setAudioStatus(
    AMAudioStatus *audio_status) {
  return kAMSuccess;
}

} // namespace audiomanager

