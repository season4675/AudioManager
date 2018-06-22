/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/engine/audio_iOS/audio_ios_impl.mm
 */

#include "audio_common.h"
#include "audio_log.h"
#include "circular_buffer.h"
//#import "AudioInputQueue.h"
#import "AudioUnitInput.h"
//#import "AudioOutputQueue.h"
#import "AudioUnitOutput.h"
#include "audio_ios_common.h"
#include "audio_ios_impl.h"

namespace audiomanager {

static AudioStream *global_ios_in = NULL;
static AudioStream *global_ios_out = NULL;

// just parse sample rate, format type, bits per sample and endianness
static AMResult parse_data_format(AMDataFormat *am_data_format, 
                                  AudioStreamBasicDescription *data_format) {
  switch (am_data_format->sample_rate) {
    case kAMSampleRate8K: {
      data_format->mSampleRate = kSampleRate8;
      break;
    }
    case kAMSampleRate16K: {
      data_format->mSampleRate = kSampleRate16;
      break;
    }
    case kAMSampleRate44K1: {
      data_format->mSampleRate = kSampleRate44K1;
      break;
    }
    case kAMSampleRate48K: {
      data_format->mSampleRate = kSampleRate48;
      break;
    }
    default: {
      LOGI("Use default sample rate 8K.");
      data_format->mSampleRate = kSampleRate16;
    }
  }

  switch (am_data_format->format_type) {
    case kAMDataFormatPCMNonInterleaved:
    case kAMDataFormatPCMInterleaved: {
      data_format->mFormatID = kAudioFormatLinearPCM;
      break;
    }
    default: {
      LOGI("Use default format type PCM.");
      data_format->mFormatID = kAudioFormatLinearPCM;
    }
  }

  switch (am_data_format->bits_per_sample) {
    case kAMSampleFormatFixed8: {
      data_format->mBitsPerChannel = 8;
      break;
    }
    case kAMSampleFormatFixed16: {
      data_format->mBitsPerChannel = 16;
      break;
    }
    case kAMSampleFormatFixed24: {
      data_format->mBitsPerChannel = 24;
      break;
    }
    case kAMSampleFormatFixed32: {
      data_format->mBitsPerChannel = 32;
      break;
    }
    default: {
      LOGI("Use default bits per sample 16bits.");
      data_format->mBitsPerChannel = 16;
    }
  }

  if (kAMByteOrderLittleEndian == am_data_format->endianness)
    data_format->mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | 
        kLinearPCMFormatFlagIsPacked;
  else if (kAMByteOrderBigEndian == am_data_format->endianness)
    data_format->mFormatFlags = kLinearPCMFormatFlagIsSignedInteger |
        kLinearPCMFormatFlagIsPacked | kLinearPCMFormatFlagIsBigEndian;
  else {
    LOGI("Use default endiannes little endian.");
    //data_format->mFormatFlags = kLinearPCMFormatFlagIsSignedInteger |
    //    kLinearPCMFormatFlagIsPacked;
    data_format->mFormatFlags = kAudioFormatFlagIsSignedInteger |
        kAudioFormatFlagIsPacked;
  }

  data_format->mFramesPerPacket = 1; // 1 frame per packet.

  data_format->mChannelsPerFrame = am_data_format->num_channels;

  data_format->mBytesPerFrame = (data_format->mBitsPerChannel / 8) * 
    data_format->mChannelsPerFrame;
  data_format->mBytesPerPacket = data_format->mBytesPerFrame;

  return kAMSuccess;
}

char* AudioQueueEngine::audio_get_version() {
  char *engine_version = kAudioEngineVersion;
  return engine_version;
}

AMResult AudioQueueEngine::audio_output_set_vol(int vol, int player_id) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;
  if (NULL == audio_stream) {
    result = -kAMResourceError;
    return result;
  }
  result = audio_stream->player->SetVolume(vol);
  return result;
}

AMResult AudioQueueEngine::audio_output_get_vol(int player_id) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;
  if (NULL == audio_stream) {
    result = -kAMResourceError;
    return result;
  }
  result = audio_stream->player->GetVolume();
  return result;
}

AMResult AudioQueueEngine::audio_output_set_mute(int player_id, bool mute) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;
  if (NULL == audio_stream) {
    result = -kAMResourceError;
    return result;
  }
  return result;
}

AMResult AudioQueueEngine::audio_output_open(AMDataFormat *data_format) {
  AudioStream *audio_stream = NULL;
  AMResult result = -kAMUnknownError;
  int player_id = 0;

  if (NULL == global_ios_in && NULL == global_ios_out) {
    audio_stream = (AudioStream *)malloc(sizeof(AudioStream));
    LOGD("Create a new AudioStream for output.");
  } else {
    audio_stream = global_ios_in;
  }
  if (NULL == audio_stream) {
    LOGE("Create AudioStream failed!");
    return -kAMMemoryFailure;
  }
  global_ios_out = audio_stream;

  //audio_stream->player = new AudioOutputQueue();
  audio_stream->player = new AudioUnitOutput();

  // set AMDataFormat into AudioStream
  parse_data_format(data_format, &audio_stream->player->mPlayFormat);
  result = audio_stream->player->InitPlayer();
  if (result == kAMSuccess) result = player_id;

  return result;
}

AMResult AudioQueueEngine::audio_output_close(int player_id) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;

  if (NULL == audio_stream || NULL == audio_stream->player) {
    LOGE("audio_stream or audio_stream->player is NULL!");
    return -kAMResourceError;
  }

  result = audio_stream->player->RemovePlayer();
  if (result != kAMSuccess) {
    LOGE("Remove player failed!");
    return result;
  }
  delete audio_stream->player;
  audio_stream->player = NULL;
  if (NULL == global_ios_in) {
    free(audio_stream);
    audio_stream = NULL;
  }
  global_ios_out = NULL;

  return kAMSuccess;
}

AMBufferCount AudioQueueEngine::audio_output_write(void *src_buffer,
                                                   const int buffer_size,
                                                   int player_id) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;
  AMBufferCount count = 0;
  int bytes = buffer_size, loop = 0;
  char *write_buf = (char *)src_buffer;

  if (NULL == audio_stream || NULL == audio_stream->player)
    return -kAMResourceError;

  if (audio_stream->output_state != kAudioStatePlaying) {
    result = audio_stream->player->StartPlay();
    if (result != kAMSuccess) {
      return result;
    }
    audio_stream->output_state = kAudioStatePlaying;
  }

  pthread_mutex_lock(&audio_stream->player->output_mutex);

  count = write_circular_buffer_bytes(
      audio_stream->player->outrb,
      //(char *)audio_stream->player->outputBuffer,
      write_buf,
      bytes);
  /*
  if (0 == count) {
    pthread_mutex_unlock(&audio_stream->recorder->input_mutex);
    return count;
  }
  for (loop = 0; loop < count; loop++) {
    audio_stream->player->outputBuffer[loop] = write_buf[loop];
  }
  */

  pthread_mutex_unlock(&audio_stream->player->output_mutex);

  return count;
}

AMBufferCount AudioQueueEngine::audio_output_getBufferCount(int player_id) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_output_pause(int player_id) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;

  if (NULL == audio_stream || NULL == audio_stream->player)
    return -kAMResourceError;

  result = audio_stream->player->PausePlay();
  return result;
}

AMResult AudioQueueEngine::audio_output_stop(int player_id) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;

  if (NULL == audio_stream || NULL == audio_stream->player)
    return -kAMResourceError;

  result = audio_stream->player->StopPlay();
  return result;
}

AMResult AudioQueueEngine::audio_output_getAudioFormat(
    AMDataFormat *data_format,
    int player_id) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_output_getAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_output_setAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_outputFromFile_open(AMFileInfo *file_info,
                                                     AMDataFormat *data_format) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_outputFromFile_close(int player_id) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_outputFromFile_start(int player_id) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_outputFromFile_pause(int player_id) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_outputFromFile_stop(int player_id) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_outputFromFile_getAudioFormat(
    AMDataFormat *data_format,
    int player_id) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_outputFromFile_getAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_outputFromFile_setAudioStatus(
    AMAudioStatus *audio_status,
    int player_id) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_input_open(AMDataFormat *data_format) {
  AudioStream *audio_stream = NULL;
  AMResult result = -kAMUnknownError;

  if (global_ios_in != NULL) {
    LOGD("a new AudioStream for input has create.");
    return kAMSuccess;
  }

  if (NULL == global_ios_in && NULL == global_ios_out) {
    audio_stream = (AudioStream *)malloc(sizeof(AudioStream));
    LOGD("Create a new AudioStream for input.");
  } else {
    audio_stream = global_ios_out;
    LOGD("a AudioStream for input has created by output.");
  }
  if (NULL == audio_stream) {
    LOGE("Create AudioStream failed!");
    return -kAMMemoryFailure;
  }
  global_ios_in = audio_stream;

  //audio_stream->recorder = new AudioInputQueue();
  audio_stream->recorder = new AudioUnitInput();

  // set AMDataFormat into AudioStream
  parse_data_format(data_format, &audio_stream->recorder->mRecordFormat);
  result = audio_stream->recorder->InitRecorder();

  return result;
}

AMResult AudioQueueEngine::audio_input_close() {
  AudioStream *audio_stream = global_ios_in;
  AMResult result = -kAMUnknownError;

  if (NULL == audio_stream || NULL == audio_stream->recorder) {
    LOGE("audio_stream or audio_stream->recorder is NULL!");
    return -kAMResourceError;
  }

  result = audio_stream->recorder->RemoveRecorder();
  if (result != kAMSuccess) {
    LOGE("Remove recorder failed!");
    return result;
  }
  delete audio_stream->recorder;
  audio_stream->recorder = NULL;
  if (NULL == global_ios_out) {
    free(audio_stream);
    audio_stream = NULL;
  }
  global_ios_in = NULL;

  return kAMSuccess;
}

AMBufferCount AudioQueueEngine::audio_input_read(void *read_buffer,
                                                 int buffer_size) {
  AudioStream *audio_stream = global_ios_in;
  AMResult result = -kAMUnknownError;
  AMBufferCount count = 0;
  int bytes = buffer_size, loop = 0;
  char *buffer = (char *)read_buffer;

  if (NULL == audio_stream || NULL == audio_stream->recorder) {
    LOGE("audio_stream or audio_stream->recorder is null!");
    return -kAMResourceError;
  }

  if (audio_stream->input_state != kAudioStateRecording) {
    result = audio_stream->recorder->StartRecord();
    if (result != kAMSuccess) {
      LOGE("Start Recorder failed! result=%d.", result);
      return result;
    }
    audio_stream->input_state = kAudioStateRecording;
  }
#if 1
  pthread_mutex_lock(&audio_stream->recorder->input_mutex);

  count = read_circular_buffer_bytes(audio_stream->recorder->inrb,
                                     //(char *)audio_stream->recorder->inputBuffer,
                                     buffer,
                                     bytes);
  /*
  if (count == 0) {
    pthread_mutex_unlock(&audio_stream->recorder->input_mutex);
    return count;
  }
  for (loop = 0; loop < count; loop++) {
    buffer[loop] = audio_stream->recorder->inputBuffer[loop];
  }
  */

  pthread_mutex_unlock(&audio_stream->recorder->input_mutex);
#endif
  return count;
}

AMResult AudioQueueEngine::audio_input_pause() {
  AudioStream *audio_stream = global_ios_in;
  AMResult result = -kAMUnknownError;

  if (NULL == audio_stream || NULL == audio_stream->recorder)
    return -kAMResourceError;

  result = audio_stream->recorder->PauseRecord();
  
  if (kAMSuccess == result)
    audio_stream->input_state = kAudioStatePaused;
  return result;
}

AMResult AudioQueueEngine::audio_input_stop() {
  AudioStream *audio_stream = global_ios_in;
  AMResult result = -kAMUnknownError;

  if (NULL == audio_stream || NULL == audio_stream->recorder)
    return -kAMResourceError;

  result = audio_stream->recorder->StopRecord();
  if (kAMSuccess == result) {
    audio_stream->input_state = kAudioStateStopped;
    LOGD("Stop recorder success!");
  } else {
    LOGE("Stop recorder failed!(%d)", result);
  }
  return result;
}

AMResult AudioQueueEngine::audio_input_getAudioFormat(
    AMDataFormat *data_format) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_input_getAudioStatus(
    AMAudioStatus *audio_status) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_input_setAudioStatus(
    AMAudioStatus *audio_status) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_inputToFile_open(AMFileInfo *file_info,
                                                  AMDataFormat *data_format) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_inputToFile_close() {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_inputToFile_start() {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_inputToFile_pause() {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_inputToFile_stop() {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_inputToFile_getAudioFormat(
    AMDataFormat *data_format) {
  return kAMSuccess;
}
AMResult AudioQueueEngine::audio_inputToFile_getAudioStatus(
   AMAudioStatus *audio_status) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_inputToFile_setAudioStatus(
    AMAudioStatus *audio_status) {
  return kAMSuccess;
}

} // namespace audiomanager

