/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/engine/audio_openSL/opensl_io.c
 */

#define TAG "OpenslIO"

#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include "opensl_io.h"
#include <android/log.h>

#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, "[OpenSL]", __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, "[OpenSL]", __VA_ARGS__))
#define LOGD(...) \
  ((void)__android_log_print(ANDROID_LOG_DEBUG, "[OpenSL]", __VA_ARGS__))

#define CONV16BIT 32768
#define CONVMYFLT (1./32768.)

static OpenslStream *global_opensl = NULL;
const int global_callback_flag[PLAYER_MAX] =
{
  0, 1, 2, 3, 4, 5, 6, 7
};

// thread Locks
// to ensure synchronisation between callbacks and processing code
void *createThreadLock(void)
{
  threadLock *lock;
  lock = (threadLock*)malloc(sizeof(threadLock));
  if (lock == NULL) return NULL;
  memset(lock, 0, sizeof(threadLock));
  if (pthread_mutex_init(&(lock->m), (pthread_mutexattr_t*)NULL) != 0) {
    free((void*)lock);
    return NULL;
  }
  if (pthread_cond_init(&(lock->c), (pthread_condattr_t*)NULL) != 0) {
    pthread_mutex_destroy(&(lock->m));
    free((void*)lock);
    return NULL;
  }
  lock->s = (unsigned char)1;

  return lock;
}

int waitThreadLock(void *lock) {
  threadLock *threadlock;
  struct timeval now;
  struct timespec outtime;
  int retval = 0;
  int loop = 2;

  threadlock = (threadLock*)lock;
  pthread_mutex_lock(&(threadlock->m));
  while (!threadlock->s && loop-- > 0) {
    gettimeofday(&now, NULL);
    outtime.tv_sec = now.tv_sec + 1;
    outtime.tv_nsec = now.tv_usec * 1000;
    pthread_cond_timedwait(&(threadlock->c), &(threadlock->m), &outtime);
  }
  if (loop <= 0) {
    LOGE("waitThreadLock failed!");
    threadlock->s = (unsigned char)0;
    pthread_mutex_unlock(&(threadlock->m));
    return SL_RESULT_INTERNAL_ERROR;
  }
  threadlock->s = (unsigned char)0;
  pthread_mutex_unlock(&(threadlock->m));
}

void notifyThreadLock(void *lock) {
  threadLock *threadlock;
  threadlock = (threadLock*)lock;
  pthread_mutex_lock(&(threadlock->m));
  threadlock->s = (unsigned char)1;
  pthread_cond_signal(&(threadlock->c));
  pthread_mutex_unlock(&(threadlock->m));
}

void destroyThreadLock(void *lock) {
  threadLock *threadlock;
  threadlock = (threadLock*)lock;
  if (threadlock == NULL) return;
  notifyThreadLock(threadlock);
  pthread_cond_destroy(&(threadlock->c));
  pthread_mutex_destroy(&(threadlock->m));
  free(threadlock);
}

int openSL_read_enqueue(OpenslStream *opensl_stream,
                        void *buffer,
                        int size) {
  char *in_buf;
  int loop;
  int result = SL_RESULT_SUCCESS;
  int index = opensl_stream->cur_input_index;
  int bytes = opensl_stream->in_buf_samples * sizeof(short);
  char *read_buf = (char *)buffer;

  if (NULL == opensl_stream || 0 == bytes) return 0;
  in_buf = opensl_stream->input_buf[opensl_stream->cur_input_buf];
  for (loop = 0; loop < size; loop++) {
    if (index >= bytes) {
      result = waitThreadLock(opensl_stream->inlock);
      if (result == SL_RESULT_INTERNAL_ERROR)
        return -result;
      (*opensl_stream->recorder_buf_que)->Enqueue(
          opensl_stream->recorder_buf_que, 
          in_buf,
          bytes);
      opensl_stream->cur_input_buf = (opensl_stream->cur_input_buf ? 0 : 1);
      index = 0;
      in_buf = opensl_stream->input_buf[opensl_stream->cur_input_buf];
    }
    read_buf[loop] = in_buf[index++];
  }
  opensl_stream->cur_input_index = index;
  return loop;
}

int openSL_write_enqueue(OpenslStream *opensl_stream,
                         void *buffer,
                         int size,
                         int player_id) {
  char *out_buf;
  int loop;
  int result = SL_RESULT_SUCCESS;
  int index = opensl_stream->cur_output_index[player_id];
  int bytes = opensl_stream->out_buf_samples[player_id] * sizeof(short);
  char *write_buf = (char *)buffer;

  if (NULL == opensl_stream || 0 == bytes) return 0;
  out_buf = opensl_stream->
      output_buf[player_id][opensl_stream->cur_output_buf[player_id]];
  for (loop = 0; loop < size; loop++) {
    out_buf[index++] = write_buf[loop];
    if (index >= bytes) {
      result = waitThreadLock(opensl_stream->outlock[player_id]);
      if (result == SL_RESULT_INTERNAL_ERROR)
        return -result;
      (*opensl_stream->bq_player_buf_que[player_id])->Enqueue(
          opensl_stream->bq_player_buf_que[player_id],
          out_buf,
          bytes);
      opensl_stream->cur_output_buf[player_id] =
          (opensl_stream->cur_output_buf[player_id] ? 0 : 1);
      index = 0;
      out_buf = opensl_stream->
          output_buf[player_id][opensl_stream->cur_output_buf[player_id]];
    }
  }
  opensl_stream->cur_output_index[player_id] = index;
  return loop;
}

// this callback handler is called every time a buffer finishes playing
void bq_player_callback(SLAndroidBufferQueueItf caller,
                        void *context) {
  //OpenslStream *opensl_stream = (OpenslStream *)context;
  OpenslStream *opensl_stream = global_opensl;
  int *player_id = (int *)context;
  notifyThreadLock(opensl_stream->outlock[*player_id]);
}

// this callback handler is called every time a buffer finishes recording
void bq_recorder_callback(SLAndroidSimpleBufferQueueItf buf_que,
                          void *context) {
  OpenslStream *opensl_stream = (OpenslStream *)context;
  notifyThreadLock(opensl_stream->inlock);
}

SLresult openSL_create_engine(OpenslStream *opensl_stream) {
  SLresult result = SL_RESULT_PRECONDITIONS_VIOLATED;

  // create engine
  result = slCreateEngine(&(opensl_stream->engine_obj),
                          0,
                          NULL,
                          0,
                          NULL,
                          NULL);
  if (result != SL_RESULT_SUCCESS) return result;

  // realize the engine
  result = (*(opensl_stream->engine_obj))->Realize(opensl_stream->engine_obj,
                                                   SL_BOOLEAN_FALSE);
  if (result != SL_RESULT_SUCCESS) return result;

  // get the engine interface,
  // which is needed in order to create other objects.
  result = (*(opensl_stream->engine_obj))->GetInterface(
      opensl_stream->engine_obj,
      SL_IID_ENGINE,
      &(opensl_stream->engine_itf));
  if (result != SL_RESULT_SUCCESS) return result;

  // get the engine capabilities interface - an implicit interface
  result = (*(opensl_stream->engine_obj))->GetInterface(
      opensl_stream->engine_obj,
      SL_IID_ENGINECAPABILITIES,
      &(opensl_stream->engine_cap_itf));
  if (result != SL_RESULT_SUCCESS) {
    LOGE("Engine capabilityies get interface failed! result=%d.", result);
    if (SL_RESULT_FEATURE_UNSUPPORTED == result)
      result = SL_RESULT_SUCCESS;
  }
  return result;
}

SLresult openSL_destroy_engine(OpenslStream *opensl_stream) {
  // destroy engine object, and invalidate all associated interfaces
  if (opensl_stream != NULL && opensl_stream->engine_obj != NULL) {
    (*(opensl_stream->engine_obj))->Destroy(opensl_stream->engine_obj);
    opensl_stream->engine_obj = NULL;
    opensl_stream->engine_itf = NULL;
    LOGD("OpenSLES destroy engine success!");
  }
  return SL_RESULT_SUCCESS;
}

SLresult openSL_create_output_mix(OpenslStream *opensl_stream) {
  SLresult result = SL_RESULT_UNKNOWN_ERROR;
  const SLInterfaceID interface_ids[1] = {SL_IID_ENVIRONMENTALREVERB};
  const SLboolean interface_req[1] = {SL_BOOLEAN_FALSE};

  if (opensl_stream == NULL) {
    LOGE("opensl_stream is nullptr!");
    return result;
  }
/*
  // huawei测试机会崩溃
  SLEnvironmentalReverbSettings reverb_settings =
      SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
*/

  // create output mix,
  // with environmental reverb specified as a non-required interface
  if (opensl_stream->engine_itf != NULL) {
    result = (*(opensl_stream->engine_itf))->CreateOutputMix(
        opensl_stream->engine_itf,
        &(opensl_stream->output_mix_obj),
        sizeof(interface_ids) / sizeof(interface_ids[0]),
        interface_ids,
        interface_req);
    if (result != SL_RESULT_SUCCESS) return result;
  } else {
    LOGE("opensl_stream->engine_itf is nullptr");
    return result;
  }

  if (opensl_stream->output_mix_obj != NULL) {
    result = (*(opensl_stream->output_mix_obj))->Realize(
        opensl_stream->output_mix_obj,
        SL_BOOLEAN_FALSE);
  } else {
    LOGE("opensl_stream->output_mix_obj is nullptr");
    return result;
  }

  // get the environmental reverb interface
  // this could fail if the environmental reverb effect is not available,
  // either because the feature is not present, excessive CPU load, or
  // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
/*
  // 这里huawei测试机会崩溃
  if (opensl_stream->output_mix_obj != NULL) {
    result = (*(opensl_stream->output_mix_obj))->GetInterface(
        opensl_stream->output_mix_obj,
        SL_IID_ENVIRONMENTALREVERB,
        &(opensl_stream->output_mix_env_reverb));
    if (SL_RESULT_SUCCESS == result) {
      result = (*(opensl_stream->output_mix_env_reverb))->
          SetEnvironmentalReverbProperties(
              opensl_stream->output_mix_env_reverb,
              &reverb_settings);
    }
  } else {
    LOGE("opensl_stream->output_mix_obj is nullptr");
    return result;
  }
*/
  // ignore unsuccessful result codes for environmental reverb,
  // as it is optional for this example
  return result;
}

SLresult openSL_destroy_output_mix(OpenslStream *opensl_stream) {
  // destroy output mix object, and invalidate all associated interface.
  if (opensl_stream != NULL && opensl_stream->output_mix_obj != NULL) {
    (*(opensl_stream->output_mix_obj))->Destroy(opensl_stream->output_mix_obj);
    opensl_stream->output_mix_obj = NULL;
    //opensl_stream->engine_itf = NULL;
    LOGD("OpenSLES destroy output mix success!");
  }

  return SL_RESULT_SUCCESS;
}

SLresult openSL_init_player(OpenslStream *opensl_stream, int player_id) {
  SLDataLocator_AndroidSimpleBufferQueue locator_buf_que =
  {
    SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
    2
  };
  SLDataLocator_AndroidFD locator_fd = {
    SL_DATALOCATOR_ANDROIDFD,
    opensl_stream->fd,
    opensl_stream->start,
    opensl_stream->length
  };
  SLDataFormat_PCM format_pcm =
  {
    opensl_stream->out_format_pcm[player_id].formatType,
    opensl_stream->out_format_pcm[player_id].numChannels,
    opensl_stream->out_format_pcm[player_id].samplesPerSec,
    opensl_stream->out_format_pcm[player_id].bitsPerSample,
    opensl_stream->out_format_pcm[player_id].bitsPerSample,
    opensl_stream->out_format_pcm[player_id].channelMask,
    opensl_stream->out_format_pcm[player_id].endianness
  };
  SLDataLocator_URI locator_uri =
  {
    SL_DATALOCATOR_URI,
    (SLchar *)opensl_stream->locator
  };
  SLDataFormat_MIME format_mime = 
  {
    SL_DATAFORMAT_MIME,
    NULL,
    SL_CONTAINERTYPE_UNSPECIFIED
  };
  SLInterfaceID interface_ids[3] =
  {
    SL_IID_BUFFERQUEUE,
    SL_IID_VOLUME,
    SL_IID_EFFECTSEND
  };
  SLboolean interface_req[3] =
  {
    SL_BOOLEAN_TRUE,
    SL_BOOLEAN_TRUE,
    SL_BOOLEAN_TRUE
  };
  SLDataSource audio_src;
  SLresult result = -1;

  // config audio src and sink
  if (kSLFileTypeURI == opensl_stream->play_type[player_id]) {
    if (NULL == opensl_stream->locator) {
      LOGE("OpenSL locator URI is NULL!");
      return SL_RESULT_PARAMETER_INVALID;
    } else {
      LOGD("OpenSL locator URI: %s", (SLchar *)opensl_stream->locator);
    }
    locator_uri.URI = (SLchar *)opensl_stream->locator;
    audio_src.pLocator = &locator_uri;
    audio_src.pFormat = &format_mime;
    interface_ids[0] = SL_IID_SEEK;
    interface_ids[1] = SL_IID_MUTESOLO;
    interface_ids[2] = SL_IID_VOLUME;
  } else if (kSLFileTypeASSETS == opensl_stream->play_type[player_id]) {
    audio_src.pLocator = &locator_fd;
    audio_src.pFormat = &format_mime;
    interface_ids[0] = SL_IID_SEEK;
    interface_ids[1] = SL_IID_MUTESOLO;
    interface_ids[2] = SL_IID_VOLUME;
  } else if (kSLFileTypePCM == opensl_stream->play_type[player_id]) {
    audio_src.pLocator = &locator_buf_que;
    audio_src.pFormat = &format_pcm;
  } else if (kSLFileTypeNONE == opensl_stream->play_type[player_id]) {
    audio_src.pLocator = &locator_buf_que;
    audio_src.pFormat = &format_pcm;
    interface_ids[0] = SL_IID_ANDROIDSIMPLEBUFFERQUEUE;
  }
  SLDataLocator_OutputMix locator_outmix = {SL_DATALOCATOR_OUTPUTMIX,
                                            opensl_stream->output_mix_obj};
  SLDataSink audio_sink = {&locator_outmix, NULL};

  // create audio player
  result = (*(opensl_stream->engine_itf))->CreateAudioPlayer(
      opensl_stream->engine_itf,
      &(opensl_stream->bq_player_obj[player_id]),
      &audio_src,
      &audio_sink,
      sizeof(interface_ids) / sizeof(interface_ids[0]),
      interface_ids,
      interface_req);
  if(result != SL_RESULT_SUCCESS) return result;

  // realize audio player
  result = (*(opensl_stream->bq_player_obj[player_id]))->Realize(
      opensl_stream->bq_player_obj[player_id],
      SL_BOOLEAN_FALSE);
  if(result != SL_RESULT_SUCCESS) return result;

  // get the play interface
  result = (*(opensl_stream->bq_player_obj[player_id]))->GetInterface(
      opensl_stream->bq_player_obj[player_id],
      SL_IID_PLAY,
      &(opensl_stream->bq_player_play[player_id]));
  if(result != SL_RESULT_SUCCESS) return result;

  if (kSLFileTypePCM == opensl_stream->play_type[player_id]
      || kSLFileTypeNONE == opensl_stream->play_type[player_id]) {
    // get the buffer queue interface
    result = (*(opensl_stream->bq_player_obj[player_id]))->GetInterface(
        opensl_stream->bq_player_obj[player_id],
        (kSLFileTypePCM == opensl_stream->play_type[player_id] ?
          SL_IID_BUFFERQUEUE : SL_IID_ANDROIDSIMPLEBUFFERQUEUE),
        &(opensl_stream->bq_player_buf_que[player_id]));
    if(result != SL_RESULT_SUCCESS) return result;

    // register callback on the buffer queue
    global_opensl = opensl_stream;
    result = (*(opensl_stream->bq_player_buf_que[player_id]))->RegisterCallback(
        opensl_stream->bq_player_buf_que[player_id],
        bq_player_callback,
        (void *)(&global_callback_flag[player_id]));
    if(result != SL_RESULT_SUCCESS) return result;
  }

  // get the volume interface
  result = (*(opensl_stream->bq_player_obj[player_id]))->GetInterface(
      opensl_stream->bq_player_obj[player_id],
      SL_IID_VOLUME,
      &(opensl_stream->bq_player_vol[player_id]));
  if(result != SL_RESULT_SUCCESS) return result;

  // get the max volume level
  result = (*(opensl_stream->bq_player_vol[player_id]))->GetMaxVolumeLevel(
      opensl_stream->bq_player_vol[player_id],
      &(opensl_stream->max_vol_level));
  if(result != SL_RESULT_SUCCESS) return result;

  // set the player's state to stopped
  result = (*(opensl_stream->bq_player_play[player_id]))->SetPlayState(
      opensl_stream->bq_player_play[player_id],
      SL_PLAYSTATE_STOPPED);
  if(result != SL_RESULT_SUCCESS) return result;
  opensl_stream->output_state[player_id] = SL_PLAYSTATE_STOPPED;

  return result;
}

SLresult openSL_remove_player(OpenslStream *opensl_stream, int player_id) {
  int result = SL_RESULT_RESOURCE_ERROR;
  // destroy buffer queue audio player object,
  // and invalidate all associated interfaces.
  if (opensl_stream != NULL &&
      opensl_stream->bq_player_obj[player_id] != NULL &&
      opensl_stream->bq_player_play[player_id] != NULL) {
    SLuint32 state = SL_PLAYSTATE_PLAYING;
    SLint16 loop = 10;
    (*(opensl_stream->bq_player_play[player_id]))->SetPlayState(
        opensl_stream->bq_player_play[player_id],
        SL_PLAYSTATE_STOPPED);
    do {
      if (opensl_stream != NULL && opensl_stream->bq_player_play[player_id] != NULL) {
        (*(opensl_stream->bq_player_play[player_id]))->GetPlayState(
            opensl_stream->bq_player_play[player_id],
            &state);
        if (state == SL_PLAYSTATE_STOPPED) {
          break;
        } else {
          if (loop < 5)
            usleep(3 * 1000);
        }
      } else {
        LOGE("set player state failed! bq_player_play is null!");
        return SL_RESULT_RESOURCE_ERROR;
      }
    } while (loop-- > 0 && state != SL_PLAYSTATE_STOPPED);
    if (loop <= 0) {
      LOGE("set player state failed! remove player failed!");
      return SL_RESULT_RESOURCE_ERROR;
    }
    opensl_stream->output_state[player_id] = 0;
    (*(opensl_stream->bq_player_obj[player_id]))->Destroy(
        opensl_stream->bq_player_obj[player_id]);
    opensl_stream->bq_player_obj[player_id] = NULL;
    opensl_stream->bq_player_play[player_id] = NULL;
    opensl_stream->bq_player_buf_que[player_id] = NULL;
    LOGD("OpenSLES remove player success!");
    result = SL_RESULT_SUCCESS;
  }
  return result;
}

SLresult openSL_init_recorder(OpenslStream *opensl_stream) {
  SLDataLocator_AndroidSimpleBufferQueue locator_buf_que =
  {
    SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
    2
  };
  SLDataFormat_PCM format_pcm =
  {
    opensl_stream->in_format_pcm.formatType,
    opensl_stream->in_format_pcm.numChannels,
    opensl_stream->in_format_pcm.samplesPerSec,
    opensl_stream->in_format_pcm.bitsPerSample,
    opensl_stream->in_format_pcm.bitsPerSample,
    opensl_stream->in_format_pcm.channelMask,
    opensl_stream->in_format_pcm.endianness
  };
  const SLInterfaceID interface_ids[2] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                          SL_IID_ANDROIDCONFIGURATION};
  //const SLInterfaceID interface_ids[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
  const SLboolean interface_req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
  //const SLboolean interface_req[1] = {SL_BOOLEAN_TRUE};
  SLresult result = -1;

  // config audio source
  SLDataLocator_IODevice locator_dev = {SL_DATALOCATOR_IODEVICE,
                                        SL_IODEVICE_AUDIOINPUT,
                                        SL_DEFAULTDEVICEID_AUDIOINPUT,
                                        NULL};
  SLDataSource audio_src = {&locator_dev, NULL};
  SLDataSink audio_sink = {&locator_buf_que, &format_pcm};

  // create audio recorder
  // (requires the RECORD_AUDIO permission)
  result = (*(opensl_stream->engine_itf))->CreateAudioRecorder(
      opensl_stream->engine_itf,
      &(opensl_stream->recorder_obj),
      &audio_src,
      &audio_sink,
      sizeof(interface_ids) / sizeof(interface_ids[0]),
      interface_ids,
      interface_req);
  if(result != SL_RESULT_SUCCESS) {
    LOGD("Create audio recorder failed! result = %d.", result);
    return result;
  }

  SLAndroidConfigurationItf inputConfig;
  result = (*(opensl_stream->recorder_obj))->GetInterface(
      opensl_stream->recorder_obj,
      SL_IID_ANDROIDCONFIGURATION,
      &inputConfig);
  SLuint32 presetValue = SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
  (*inputConfig)->SetConfiguration(inputConfig,
                                   SL_ANDROID_KEY_RECORDING_PRESET,
                                   &presetValue,
                                   sizeof(SLuint32));

  // realize audio recorder
  result = (*(opensl_stream->recorder_obj))->Realize(
      opensl_stream->recorder_obj,
      SL_BOOLEAN_FALSE);
  if(result != SL_RESULT_SUCCESS) {
    LOGD("Realize audio recorder failed! result = %d.", result);
    return result;
  }

  // get the recorder interface
  result = (*(opensl_stream->recorder_obj))->GetInterface(
      opensl_stream->recorder_obj,
      SL_IID_RECORD,
      &(opensl_stream->recorder_record));
  if(result != SL_RESULT_SUCCESS) {
    LOGD("Get the recorder interface failed! result = %d.", result);
    return result;
  }

  // get the buffer queue interface
  result = (*(opensl_stream->recorder_obj))->GetInterface(
      opensl_stream->recorder_obj,
      SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
      &(opensl_stream->recorder_buf_que));
  if(result != SL_RESULT_SUCCESS) {
    LOGD("Cet therecorder buffer queue failed! result = %d.", result);
    return result;
  }

  // register callback on the buffer queue
  result = (*(opensl_stream->recorder_buf_que))->RegisterCallback(
      opensl_stream->recorder_buf_que,
      bq_recorder_callback,
      opensl_stream);
  if(result != SL_RESULT_SUCCESS) return result;

  result = (*(opensl_stream->recorder_record))->SetRecordState(
      opensl_stream->recorder_record,
      SL_RECORDSTATE_STOPPED);
  if(result != SL_RESULT_SUCCESS) return result;
  opensl_stream->input_state = SL_RECORDSTATE_STOPPED;
  return result;
}

SLresult openSL_remove_recorder(OpenslStream *opensl_stream) {
  if (opensl_stream != NULL &&
      opensl_stream->recorder_record != NULL &&
      opensl_stream->recorder_obj != NULL) {
    SLuint32 state = SL_RECORDSTATE_RECORDING;
    SLint16 loop = 10;
    (*(opensl_stream->recorder_record))->SetRecordState(
        opensl_stream->recorder_record,
        SL_RECORDSTATE_STOPPED);
    do {
      if (opensl_stream != NULL && opensl_stream->recorder_record != NULL) {
        (*(opensl_stream->recorder_record))->GetRecordState(
            opensl_stream->recorder_record,
            &state);
        if (state == SL_RECORDSTATE_STOPPED) {
          break;
        } else {
          if (loop < 5)
            usleep(3 * 1000);
        }
      } else {
        LOGE("set recorder state failed! recorder_record is null!");
        return SL_RESULT_RESOURCE_ERROR;
      }
    } while (loop-- > 0 && state != SL_RECORDSTATE_STOPPED);

    if (loop <= 0) {
      LOGE("set recorder state failed! remove recorder failed!");
      return SL_RESULT_RESOURCE_ERROR;
    }

    opensl_stream->input_state = 0;
    (*(opensl_stream->recorder_obj))->Destroy(opensl_stream->recorder_obj);
    opensl_stream->recorder_obj = NULL;
    opensl_stream->recorder_record = NULL;
    opensl_stream->recorder_buf_que = NULL;
    LOGD("OpenSLES remove recorder success!");
  }

  return SL_RESULT_SUCCESS;
}

SLresult openSL_set_player_state(OpenslStream *opensl_stream,
                                 int player_id,
                                 SLuint32 audio_state) {
  SLresult result = SL_RESULT_UNKNOWN_ERROR;
  if (opensl_stream != NULL &&
      *(opensl_stream->bq_player_play[player_id]) != NULL) {
    (*(opensl_stream->bq_player_play[player_id]))->SetPlayState(
        opensl_stream->bq_player_play[player_id],
        audio_state);
    SLint16 loop = 10;
    SLuint32 state = SL_PLAYSTATE_PLAYING;
    (*(opensl_stream->bq_player_play[player_id]))->SetPlayState(
        opensl_stream->bq_player_play[player_id],
        audio_state);
    while (loop-- > 0 && state != audio_state) {
      (*(opensl_stream->bq_player_play[player_id]))->GetPlayState(
          opensl_stream->bq_player_play[player_id],
          &state);
      if (loop < 5)
        usleep(10 * 1000);
    }
    if (loop > 0 && audio_state == SL_PLAYSTATE_STOPPED) {
      if (*opensl_stream->bq_player_buf_que[player_id] != NULL) {
        result = (*opensl_stream->bq_player_buf_que[player_id])->Clear(
            opensl_stream->bq_player_buf_que[player_id]);
        if (result != SL_RESULT_SUCCESS)
          LOGD("clear player queue failed(%d).", result);

        notifyThreadLock(opensl_stream->outlock[player_id]);

        if (opensl_stream->output_buf[player_id][0] != NULL &&
            opensl_stream->output_buf[player_id][1] != NULL) {
          memset(opensl_stream->output_buf[player_id][0],
                 0,
                 opensl_stream->out_buf_samples[player_id] * sizeof(short) * sizeof(char));
          memset(opensl_stream->output_buf[player_id][1],
                 0,
                 opensl_stream->out_buf_samples[player_id] * sizeof(short) * sizeof(char));
        }
      } else {
        LOGE("bq_player_buf_que is null, set player state failed!");
      }
    }
  } else {
    return SL_RESULT_PARAMETER_INVALID;
  }
  return SL_RESULT_SUCCESS;
}

SLresult openSL_set_recorder_state(OpenslStream *opensl_stream,
                                   SLuint32 audio_state) {
  if (opensl_stream != NULL && *(opensl_stream->recorder_record) != NULL) {
    (*(opensl_stream->recorder_record))->SetRecordState(
        opensl_stream->recorder_record,
        audio_state);
  } else {
    return SL_RESULT_PARAMETER_INVALID;
  }
  return SL_RESULT_SUCCESS;
}

SLresult openSL_set_player_vol(OpenslStream *opensl_stream,
                               int vol,
                               int player_id) {
  int result = -SL_RESULT_UNKNOWN_ERROR;
  const SLmillibel min_millibel = SL_MILLIBEL_MIN / 100 * 100;
  SLmillibel volume = SL_MILLIBEL_MIN;
  int interval = MAX_VOL_LEVEL - MIN_VOL_LEVEL;

  if (opensl_stream != NULL) {
    volume = (opensl_stream->max_vol_level - min_millibel) /
        interval * vol + min_millibel;
    LOGD("set player volume level is %d-%dmB.", vol, volume);
    result = (*(opensl_stream->bq_player_vol[player_id]))->SetVolumeLevel(
        opensl_stream->bq_player_vol[player_id],
        volume);
  }
  return result;
}

SLresult openSL_get_player_vol(OpenslStream *opensl_stream, int player_id) {
  int result = -SL_RESULT_UNKNOWN_ERROR;
  const SLmillibel min_millibel = SL_MILLIBEL_MIN / 100 * 100;
  SLmillibel volume = SL_MILLIBEL_MIN;
  int vol = 0;
  int interval = MAX_VOL_LEVEL - MIN_VOL_LEVEL;

  if (opensl_stream != NULL) {
    result = (*(opensl_stream->bq_player_vol[player_id]))->GetVolumeLevel(
        opensl_stream->bq_player_vol[player_id],
        &volume);
    if (SL_RESULT_SUCCESS == result) {
      vol = (volume - min_millibel) * interval /
          (opensl_stream->max_vol_level - min_millibel);
      LOGD("get player volume level is %d-%dmB.", vol, volume);
      result = vol;
    }
  }
  return result;
}

SLresult openSL_set_player_mute(OpenslStream *opensl_stream,
                                int player_id,
                                bool mute) {
  int result = -SL_RESULT_UNKNOWN_ERROR;

  if (opensl_stream != NULL) {
    result = (*(opensl_stream->bq_player_vol[player_id]))->SetMute(
        opensl_stream->bq_player_vol[player_id],
        mute);
  }
  return result;
}

SLresult openSL_get_version(OpenslStream *opensl_stream, char *engine_version) {
  SLresult result = -1;
  SLint16 ver_major = 0, ver_minor = 0, ver_step = 0;
  char version[32] = {0};

  // Query API version
  if (opensl_stream->engine_cap_itf != NULL) {
    result = (*(opensl_stream->engine_cap_itf))->QueryAPIVersion(
        opensl_stream->engine_cap_itf,
        &ver_major,
        &ver_minor,
        &ver_step);
    if (result != SL_RESULT_SUCCESS) {
      LOGE("get query API version failed! ret=%d", result);
      return result;
    }
  }

  result = snprintf(version, 32, "%d.%d.%d", ver_major, ver_minor, ver_step);
  if (result < 0) {
    LOGE("snprintf API version failed! ret=%d", result);
    return result;
  }
  strncat(engine_version, version, strlen(version));

  return result;
}

