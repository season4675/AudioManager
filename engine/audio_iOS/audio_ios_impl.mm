/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/engine/audio_iOS/audio_ios_impl.mm
 */

#define TAG "AudioIosImpl"

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

bool write_check(AudioQueueEngine *engine, AudioUnitOutput *player, int buffer_size, int player_id) {
  AudioStream *audio_stream = global_ios_out;
  circular_buffer *wp = player->outrb;
  int wr_size = wp->size;
  int wr_remaining = checkspace_circular_buffer(wp, 1);
  int buf_size_used = wr_size - wr_remaining;
  int interval = 0; //ms
  int bufSizeTenMs = player->bufSizeDurationOneSec / 100; // the buf size in 10ms
  int retry = 0;
  int underrun_timeout_ms = 2000;
  int retry_cnt = 10;
  while (buffer_size > wr_remaining) {
    if (retry ++ > retry_cnt) {
      KLOGE(TAG, "wait for %d ms underrrun!!", underrun_timeout_ms);
      audio_stream->output_state[player_id] = kAudioStatePaused;
      return false;
    }
    KLOGV(TAG, "buffer_size %d wr_remaining %d", buffer_size, wr_remaining);
    usleep(2000*1000/retry_cnt);
    wr_remaining = checkspace_circular_buffer(wp, 1);
  }
  return true;
  if (buffer_size > wr_size / 3) {
    // too long
    interval = 0;
    audio_stream->output_event = kAMWriteTooLong;
    audio_stream->output_interval = interval;
    engine->audio_listener_.audio_event_callback(
        (AMEvent)audio_stream->output_event,
        audio_stream->output_interval);
    return false;
  }
  if (buffer_size > audio_stream->max_buf_size) {
    audio_stream->max_buf_size = buffer_size;
  }
  if (buffer_size > wr_remaining) {
    // 需要播放多久才可以腾出多久才能再继续装数据
    int remain_time0 = (buffer_size - wr_remaining) / bufSizeTenMs + 1; // the times of 10ms
    // 剩余数据需要播放多久
    int remain_time1 = buf_size_used / bufSizeTenMs; // the times of 10ms
    interval = 10 * ((remain_time0 < remain_time1) ? remain_time0 : remain_time1);
    // too long
    if (audio_stream->output_event != kAMWriteTooLong ||
        audio_stream->output_interval != interval) {
      // callback
      audio_stream->output_event = kAMWriteTooLong;
      audio_stream->output_interval = interval;
      engine->audio_listener_.audio_event_callback(
          (AMEvent)audio_stream->output_event,
          audio_stream->output_interval);
    }
    return false;
  } else {
    if (wr_remaining <= (audio_stream->max_buf_size * 3)) {
    // 预判下次可能装不下历史最大写入量的三倍
      interval = 10 * ((buf_size_used + buffer_size) / bufSizeTenMs);
      // 由于外部interval不准，保守操作
      if (interval > 100)
        interval = interval / 4 * 3;
      // wait
      if (audio_stream->output_event != kAMWriteWait ||
        audio_stream->output_interval != interval) {
        // callback
        audio_stream->output_event = kAMWriteWait;
        audio_stream->output_interval = interval;
        engine->audio_listener_.audio_event_callback(
          (AMEvent)audio_stream->output_event,
          audio_stream->output_interval);
      }
    }
    if (wr_remaining > (wr_size / 2)) {
    // 判断剩余数据量大于写入量的两倍，认为空间充分
      interval = buffer_size / bufSizeTenMs * 10;
      if (interval > 10)
        interval -= 10;
      // promise
      if (audio_stream->output_event != kAMWritePromise ||
        audio_stream->output_interval != interval) {
        // callback
        audio_stream->output_event = kAMWritePromise;
        audio_stream->output_interval = interval;
        engine->audio_listener_.audio_event_callback(
          (AMEvent)audio_stream->output_event,
          audio_stream->output_interval);
      }
    }
    return true;
  }
}

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
    case kAMSampleRate24K: {
      data_format->mSampleRate = kSampleRate24;
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
      KLOGI(TAG, "Use default sample rate 16K.");
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
      KLOGI(TAG, "Use default format type PCM.");
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
      KLOGI(TAG, "Use default bits per sample 16bits.");
      data_format->mBitsPerChannel = 16;
    }
  }

  if (kAMByteOrderLittleEndian == am_data_format->endianness)
    data_format->mFormatFlags = kAudioFormatFlagIsSignedInteger |
        kAudioFormatFlagIsPacked;
  else if (kAMByteOrderBigEndian == am_data_format->endianness)
    data_format->mFormatFlags = kAudioFormatFlagIsSignedInteger |
        kAudioFormatFlagIsPacked | kAudioFormatFlagIsBigEndian;
  else {
    KLOGI(TAG, "Use default endiannes little endian.");
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
  result = audio_stream->player[player_id]->SetVolume(vol);
  return result;
}

AMResult AudioQueueEngine::audio_output_get_vol(int player_id) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;
  if (NULL == audio_stream) {
    result = -kAMResourceError;
    return result;
  }
  result = audio_stream->player[player_id]->GetVolume();
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

void AudioQueueEngine::audio_registerListener(const AMEventListener &listener) {
  audio_listener_.audio_event_callback = listener.audio_event_callback;
  audio_listener_.user_data = listener.user_data;
}

AMResult AudioQueueEngine::audio_output_open(AMDataFormat *data_format) {
  AudioStream *audio_stream = NULL;
  AMResult result = -kAMUnknownError;
  int player_id = 0;

  if (NULL == global_ios_in && NULL == global_ios_out) {
    audio_stream = (AudioStream *)malloc(sizeof(AudioStream));
    memset(audio_stream, 0, sizeof(AudioStream));
    KLOGD(TAG, "Create a new AudioStream for output.");
  } else if (global_ios_out != NULL) {
    audio_stream = global_ios_out;
    KLOGD(TAG, "A AudioStream for output has created by output");
  } else {
    audio_stream = global_ios_in;
    KLOGD(TAG, "A AudioStream for output has created by input");
  }
  if (NULL == audio_stream) {
    KLOGE(TAG, "Create AudioStream failed!");
    return -kAMMemoryFailure;
  }
  global_ios_out = audio_stream;

  // generate player id
  for (player_id = 0; player_id < PLAYER_MAX; player_id++) {
    if (0 == ((audio_stream->player_channel_mask >> player_id) & 0x1)) {
      KLOGD(TAG, "Player ID (%d) is usable at present.", player_id);
      break;
    }
  }
  if (player_id >= PLAYER_MAX) {
    KLOGI(TAG, "Have exhausted all player.");
    return -kAMParameterInvalid;
  }

  //audio_stream->player = new AudioOutputQueue();
  audio_stream->player[player_id] = new AudioUnitOutput();
  audio_stream->player[player_id]->AudioUnitRegisterListener(audio_listener_);

  // set AMDataFormat into AudioStream
  parse_data_format(data_format, &audio_stream->player[player_id]->mPlayFormat);
  result = audio_stream->player[player_id]->InitPlayer();
  if (result == kAMSuccess) {
    audio_stream->player_channel_mask |= (1<<player_id);
    audio_stream->output_state[player_id] = kAMAudioStateOpened;
    result = player_id;
  }

  return result;
}

AMResult AudioQueueEngine::audio_output_close(int player_id) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;

  if (NULL == audio_stream || NULL == audio_stream->player[player_id]) {
    KLOGE(TAG, "audio_stream or audio_stream->player[%d] is NULL!", player_id);
    return -kAMResourceError;
  }

  result = audio_stream->player[player_id]->RemovePlayer();
  if (result != kAMSuccess) {
    KLOGE(TAG, "Remove player failed!");
    return result;
  }
  delete audio_stream->player[player_id];
  audio_stream->player[player_id] = NULL;
  audio_stream->player_channel_mask &= ~(1<<player_id);
  audio_stream->output_state[player_id] = kAMAudioStateClosed;
  if (0 == audio_stream->player_channel_mask) {
    global_ios_out = NULL;
  }
  if (NULL == global_ios_in && NULL == global_ios_out) {
    free(audio_stream);
    audio_stream = NULL;
  }

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

  if (NULL == audio_stream || NULL == audio_stream->player[player_id])
    return -kAMResourceError;

  if (audio_stream->output_state[player_id] != kAudioStatePlaying) {
    result = audio_stream->player[player_id]->StartPlay();
    if (result != kAMSuccess) {
      return result;
    }
    audio_stream->output_state[player_id] = kAudioStatePlaying;
  }

  pthread_mutex_lock(&audio_stream->player[player_id]->output_mutex);

  if (!write_check(this, audio_stream->player[player_id], buffer_size, player_id)) {
    pthread_mutex_unlock(&audio_stream->player[player_id]->output_mutex);
    return 0;
  }

  count = write_circular_buffer_bytes(
      audio_stream->player[player_id]->outrb,
      write_buf,
      bytes);
  KLOGV(TAG, "write_circular_buffer_bytes %d", count);
  pthread_mutex_unlock(&audio_stream->player[player_id]->output_mutex);

  return count;
}

AMBufferCount AudioQueueEngine::audio_output_getBufferCount(int player_id) {
  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_output_pause(int player_id) {
#if 1
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;

  if (NULL == audio_stream || NULL == audio_stream->player[player_id])
    return -kAMResourceError;

  if (audio_stream->output_state[player_id] != kAudioStatePaused) {
    result = audio_stream->player[player_id]->PausePlay();
    if (result != kAMSuccess) {
      return result;
    }
    audio_stream->output_state[player_id] = kAudioStatePaused;
  }
  return result;
#endif
}

AMResult AudioQueueEngine::audio_output_stop(int player_id, bool drain) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;

  if (NULL == audio_stream || NULL == audio_stream->player[player_id])
    return -kAMResourceError;

  if (audio_stream->output_state[player_id] != kAudioStateStopped) {
    result = audio_stream->player[player_id]->StopPlay(drain);
    if (result != kAMSuccess) {
      return result;
    }
    audio_stream->output_state[player_id] = kAudioStateStopped;
  }
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
  AudioStream *audio_stream = NULL;
  AMResult result = -kAMUnknownError;
  int player_id = 0;

  if (NULL == global_ios_in && NULL == global_ios_out) {
    audio_stream = (AudioStream *)malloc(sizeof(AudioStream));
    memset(audio_stream, 0, sizeof(AudioStream));
    KLOGD(TAG, "Create a new AudioStream for output.");
  } else if (global_ios_out != NULL) {
    audio_stream = global_ios_out;
  } else {
    audio_stream = global_ios_in;
  }
  if (NULL == audio_stream) {
    KLOGE(TAG, "Create AudioStream failed!");
    return -kAMMemoryFailure;
  }
  global_ios_out = audio_stream;

  // generate player id
  for (player_id = 0; player_id < PLAYER_MAX; player_id++) {
    if (0 == ((audio_stream->player_channel_mask >> player_id) & 0x1)) {
      KLOGD(TAG, "Player ID (%d) is usable at present.", player_id);
      break;
    }
  }
  if (player_id >= PLAYER_MAX) {
    KLOGI(TAG, "Have exhausted all player.");
    return -kAMParameterInvalid;
  }

  //audio_stream->player = new AudioOutputQueue();
  audio_stream->player[player_id] = new AudioUnitOutput();

  // set AMDataFormat into AudioStream
  parse_data_format(data_format, &audio_stream->player[player_id]->mPlayFormat);
  audio_stream->player[player_id]->play_type = file_info->file_type;
  audio_stream->player[player_id]->locator = file_info->file_path;
  audio_stream->player[player_id]->start = file_info->start;
  audio_stream->player[player_id]->length = file_info->length;

  result = audio_stream->player[player_id]->InitPlayer();
  if (result == kAMSuccess) {
    audio_stream->player_channel_mask |= (1<<player_id);
    result = player_id;
  }

  return result;
}

AMResult AudioQueueEngine::audio_outputFromFile_close(int player_id) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;

  if (NULL == audio_stream || NULL == audio_stream->player[player_id]) {
    KLOGE(TAG, "audio_stream or audio_stream->player is NULL!");
    return -kAMResourceError;
  }

  result = audio_stream->player[player_id]->RemovePlayer();
  if (result != kAMSuccess) {
    KLOGE(TAG, "Remove player failed!");
    return result;
  }
  delete audio_stream->player[player_id];
  audio_stream->player[player_id] = NULL;
  audio_stream->player_channel_mask &= ~(1<<player_id);
  if (0 == audio_stream->player_channel_mask) {
    global_ios_out = NULL;
  }
  if (NULL == global_ios_in && NULL == global_ios_out) {
    free(audio_stream);
    audio_stream = NULL;
  }

  return kAMSuccess;
}

AMResult AudioQueueEngine::audio_outputFromFile_start(int player_id) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;

  if (NULL == audio_stream || NULL == audio_stream->player[player_id])
    return -kAMResourceError;

  if (audio_stream->output_state[player_id] != kAudioStatePlaying) {
    result = audio_stream->player[player_id]->StartPlay();
    if (result != kAMSuccess) {
      return result;
    }
    audio_stream->output_state[player_id] = kAudioStatePlaying;
  }
  return result;
}

AMResult AudioQueueEngine::audio_outputFromFile_pause(int player_id) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;

  if (NULL == audio_stream || NULL == audio_stream->player[player_id])
    return -kAMResourceError;

  if (audio_stream->output_state[player_id] != kAudioStatePaused) {
    result = audio_stream->player[player_id]->PausePlay();
    if (result != kAMSuccess) {
      return result;
    }
    audio_stream->output_state[player_id] = kAudioStatePaused;
  }
  return result;
}

AMResult AudioQueueEngine::audio_outputFromFile_stop(int player_id) {
  AudioStream *audio_stream = global_ios_out;
  AMResult result = -kAMUnknownError;

  if (NULL == audio_stream || NULL == audio_stream->player[player_id])
    return -kAMResourceError;

  if (audio_stream->output_state[player_id] != kAudioStateStopped) {
    result = audio_stream->player[player_id]->StopPlay();
    if (result != kAMSuccess) {
      return result;
    }
    audio_stream->output_state[player_id] = kAudioStateStopped;
  }
  return result;
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
    KLOGD(TAG, "a AudioStream for input has created.");
    return kAMSuccess;
  }

  if (NULL == global_ios_in && NULL == global_ios_out) {
    audio_stream = (AudioStream *)malloc(sizeof(AudioStream));
    KLOGD(TAG, "Create a new AudioStream for input.");
  } else {
    audio_stream = global_ios_out;
    KLOGD(TAG, "a AudioStream for input has created by output.");
  }
  if (NULL == audio_stream) {
    KLOGE(TAG, "Create AudioStream failed!");
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
    KLOGE(TAG, "audio_stream or audio_stream->recorder is NULL!");
    return -kAMResourceError;
  }

  result = audio_stream->recorder->RemoveRecorder();
  if (result != kAMSuccess) {
    KLOGE(TAG, "Remove recorder failed!");
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
    KLOGE(TAG, "audio_stream or audio_stream->recorder is null!");
    return -kAMResourceError;
  }

  if (audio_stream->input_state != kAudioStateRecording) {
    result = audio_stream->recorder->StartRecord();
    if (result != kAMSuccess) {
      KLOGE(TAG, "Start Recorder failed! result=%d.", result);
      return result;
    } else {
      KLOGD(TAG, "Start Recorder success!");
    }
    audio_stream->input_state = kAudioStateRecording;
  }
  pthread_mutex_lock(&audio_stream->recorder->input_mutex);
  count = read_circular_buffer_bytes(audio_stream->recorder->inrb,
                                     buffer,
                                     bytes);
  pthread_mutex_unlock(&audio_stream->recorder->input_mutex);
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
    KLOGD(TAG, "Stop recorder success!");
  } else {
    KLOGE(TAG, "Stop recorder failed!(%d)", result);
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

