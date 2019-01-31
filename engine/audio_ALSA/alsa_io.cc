/* Copyright [2018] <shichen.fsc@alibaba-inc.com>
 * File: AudioManager/engine/audio_ALSA/alsa_io.c
 */

#define TAG "AlsaIO"

#include <math.h>
#include "audio_log.h"
#include "alsa_io.h"

#define _SND_DEBUG_
#undef _SND_DEBUG_

#ifdef _SND_DEBUG_
#include <string>
#endif

namespace nuiam {

#ifdef _SND_DEBUG_
std::ofstream alsa_save_stream;
std::ofstream play_save_stream;
#endif

#define PLAYER_BUFFER_TIME       1.0
#define PLAYER_PERIOD_TIME       0.08
#define PLAYER_LOOP_TIME        (PLAYER_PERIOD_TIME * 2)
#define PLAYER_LOOP_SLEEP_TIME  (PLAYER_LOOP_TIME * 3 / 4)
#define CAPTURE_BUFFER_TIME      0.2
#define CAPTURE_PERIOD_TIME      0.01
#define CAPTURE_LOOP_TIME        (CAPTURE_PERIOD_TIME * 2)
#define CAPTURE_LOOP_SLEEP_TIME  (CAPTURE_LOOP_TIME * 3 / 4)

static AlsaStream *global_alsa = NULL;

//______________________________________________________________________________
// Determine the size, in bytes,
// of a buffer necessary to represent the supplied number
// of seconds of audio data.
int ComputeBufferSize(AlsaFormat *alsa, float seconds) {
  int bytes = 0;
  bytes = (int)ceil(alsa->num_channels * alsa->sample_rate * alsa->byte_per_sample * seconds);

  return bytes;
}

void *arecorder(AlsaFormat *capture) {
  int frame = (int)(capture->sample_rate * CAPTURE_LOOP_TIME);
  int bytes = frame * capture->byte_per_sample * capture->num_channels;
  int result = 0;
  char *buffer = (char*)malloc(bytes);
  memset(buffer, 0, bytes);
  //KLOGD(TAG, "arecorder snd_pcm_readi %d bytes.", bytes);

  while (capture->audio_state == ALSA_CAPTURESTATE_CAPTURING) {
    result = snd_pcm_readi(capture->handle, buffer, frame);
    if (result <= 0) {
      KLOGD(TAG, "snd_pcm_readi failed, result=%d(%s)frames, except %d.",
          result, snd_strerror(result), frame);
      if (result == -ENODEV) {
        KLOGE(TAG, "please check mic device! open again!");
        int ret = alsa_input_open(capture);
        if (ret != kAMSuccess) {
          KLOGE(TAG, "open alsa device failed!(ret)", ret);
          sleep(2);
        }
      }
      usleep(CAPTURE_LOOP_SLEEP_TIME * 1000000);
      continue;
    }
    bytes = result * capture->byte_per_sample * capture->num_channels;
    //KLOGD(TAG, "arecorder readi %d frames = %d bytes", result, bytes);
#ifdef _SND_DEBUG_
    if (bytes > 0 && alsa_save_stream.is_open()) {
      alsa_save_stream.write(reinterpret_cast<const char*>(buffer), bytes);
    }
#endif
    pthread_mutex_lock(&capture->mutex);
    result = write_circular_buffer_bytes(capture->ring_buffer, buffer, bytes);
    pthread_mutex_unlock(&capture->mutex);
    usleep(CAPTURE_LOOP_SLEEP_TIME * 1000000);
//    KLOGD(TAG, "arecorder read %d bytes, except %d.", result, bytes);
  }
  free(buffer);
  buffer = NULL;
  pthread_exit(NULL);
}

void *aplayer(AlsaFormat *player) {
  int frame = (int)(player->sample_rate * PLAYER_LOOP_TIME);
  int bytes = frame * player->byte_per_sample * player->num_channels;
  int result = 0;
  char *buffer = (char*)malloc(bytes);

  while (player->audio_state == ALSA_PLAYSTATE_PLAYING) {
    if (player->play_type == nuiam::kAMFileTypePCM && player->fd) {
      result = fread((char *)buffer, 1, bytes, player->fd);
      if (result <= 0) {
        KLOGD(TAG, "audio file finish!");
        break;
      }
    } else {
      pthread_mutex_lock(&player->mutex);
      result = read_circular_buffer_bytes(player->ring_buffer, buffer, bytes);
      pthread_mutex_unlock(&player->mutex);
    }

    if (result <= 0) {
      usleep(PLAYER_LOOP_SLEEP_TIME * 1000000);
      continue;
    }
#ifdef _SND_DEBUG_
    if (play_save_stream.is_open()) {
      play_save_stream.write(reinterpret_cast<const char*>(buffer), result);
    }
#endif
    if (result != bytes) {
      frame = result / player->byte_per_sample / player->num_channels;
    }
    snd_pcm_sframes_t frame_ret = 0;
    frame_ret = snd_pcm_writei(player->handle, buffer, frame);
    if (frame_ret < 0) {
      frame_ret = snd_pcm_recover(player->handle, frame_ret, 0);
    }
    if (frame_ret <= 0) {
      KLOGE(TAG, "snd_pcm_writei failed:%s(%d), except write %d frames.",
          snd_strerror(frame_ret), frame_ret, frame);
    }
    usleep(PLAYER_LOOP_SLEEP_TIME * 1000000);
  }
  free(buffer);
  buffer = NULL;
  pthread_exit(NULL);
}

AMResult alsa_output_open(AlsaFormat *player) {
  int bufferByteSize = 0;
  int loop = 0;
  AMResult result = -kAMUnknownError;
  char *pcm_name[] = {"default", "hw:3,0", "hw:2,0", "hw:1,0", "hw:0,0"};
  unsigned int latency = (unsigned int)(PLAYER_PERIOD_TIME * 1000000);
  for (loop = 0; loop < 5; loop++) {
    result = snd_pcm_open(&player->handle, pcm_name[loop], SND_PCM_STREAM_PLAYBACK, 0);
    if (result) {
      KLOGE(TAG, "player snd_pcm_open %s failed! err:%s(%d)",
          pcm_name[loop],
          snd_strerror(result),
          result);
    } else {
      KLOGI(TAG, "snd_pcm_open (%s) success!", pcm_name[loop]);
      if ((result = snd_pcm_set_params(player->handle,
                                   player->bit_per_sample,
                                   player->format_type,
                                   player->num_channels,
                                   player->sample_rate,
                                   1,
                                   latency)) < 0) {
        KLOGE(TAG, "player open, but set params error: %s", snd_strerror(result));
        continue;
        //return -kAMInternalError;
      } else {
        KLOGD(TAG, "set params bit_per_sample:%d", player->bit_per_sample);
        KLOGD(TAG, "set params format_type:%d", player->format_type);
        KLOGD(TAG, "set params num_channels:%d", player->num_channels);
        KLOGD(TAG, "set params sample_rate:%d", player->sample_rate);
      }
      break;
    }
  }
  if (loop >= 5) {
    return result;
  }

  if ((result = snd_pcm_nonblock(player->handle, 0)) < 0) {
    KLOGE(TAG, "player set block failed");
    snd_pcm_close(player->handle);
    return -kAMInternalError;
  }

  // Set ALSA parameters
  snd_pcm_hw_params_t *params;
  unsigned int val, val2;
  int dir;
  snd_pcm_uframes_t frames;
  snd_pcm_hw_params_alloca(&params);
  snd_pcm_hw_params_any(player->handle, params);
  /* Interleaved mode */
  snd_pcm_hw_params_set_access(player->handle, params, player->format_type);
  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(player->handle, params, player->bit_per_sample);
  /* 2 channels */
  snd_pcm_hw_params_set_channels(player->handle, params, player->num_channels);
  /* 32000 bits/second sampling rate (CD quality) */
  val = player->sample_rate;
  snd_pcm_hw_params_set_rate_near(player->handle, params, &val, &dir);
  /* Set period size to 320 frames. */
  frames = (int)(player->sample_rate * PLAYER_PERIOD_TIME);
  snd_pcm_hw_params_set_period_size_near(player->handle, params, &frames, &dir);
  result = snd_pcm_hw_params(player->handle, params);
  if (result < 0)
    KLOGE(TAG, "snd_pcm_hw_params failed!(%s)", snd_strerror(result));

  player->bufSizeDurationOneSec = ComputeBufferSize(player, 1);
  bufferByteSize = ComputeBufferSize(player, PLAYER_BUFFER_TIME);
  KLOGD(TAG, "compute buffer size is %d", bufferByteSize);
  player->ring_buffer = create_circular_buffer(bufferByteSize * 4);
  if (player->ring_buffer == NULL) {
    KLOGE(TAG, "alsa_output_open create ring buffer failed!");
    return -kAMIoError;
  }

  // audio file
  if (player->play_type == nuiam::kAMFileTypePCM) {
    if (player->locator != NULL) {
      player->fd = fopen(player->locator, "r");
      if (player->fd) {
        fseek(player->fd, 0, SEEK_SET);
      } else {
        KLOGE(TAG, "audio file open failed!");
        snd_pcm_close(player->handle);
        free_circular_buffer(player->ring_buffer);
        player->ring_buffer = NULL;
        return -kAMIoError;
      }
    } else {
      KLOGE(TAG, "file path is null!");
      snd_pcm_close(player->handle);
      free_circular_buffer(player->ring_buffer);
      player->ring_buffer = NULL;
      return -kAMIoError;
    }
  }

  pthread_mutex_init(&player->mutex, NULL);
  player->audio_state = ALSA_PLAYSTATE_OPENED;

#ifdef _SND_DEBUG_
/******************打印参数*********************/
  snd_pcm_hw_params_get_channels(params, &val);
  printf("channels = %d\n", val);
  snd_pcm_hw_params_get_rate(params, &val, &dir);
  printf("rate = %d bps\n", val);
  snd_pcm_hw_params_get_period_time(params, &val, &dir);
  printf("period time = %d us\n", val);
  snd_pcm_hw_params_get_period_size(params, &frames, &dir);
  printf("period size = %ld frames\n", (long)frames);
  snd_pcm_hw_params_get_buffer_time(params, &val, &dir);
  printf("buffer time = %d us\n", val);
  snd_pcm_hw_params_get_buffer_size(params, (snd_pcm_uframes_t *)&val);
  printf("buffer size = %d frames\n", val);
  snd_pcm_hw_params_get_periods(params, &val, &dir);
  printf("periods per buffer = %d frames\n", val);
  snd_pcm_hw_params_get_rate_numden(params, &val, &val2);
  printf("exact rate = %d/%d bps\n", val, val2);
  val = snd_pcm_hw_params_get_sbits(params);
  printf("significant bits = %d\n", val);
  //snd_pcm_hw_params_get_tick_time(params,  &val, &dir);
  printf("tick time = %d us\n", val);
  val = snd_pcm_hw_params_is_batch(params);
  printf("is batch = %d\n", val);
  val = snd_pcm_hw_params_is_block_transfer(params);
  printf("is block transfer = %d\n", val);
  val = snd_pcm_hw_params_is_double(params);
  printf("is double = %d\n", val);
  val = snd_pcm_hw_params_is_half_duplex(params);
  printf("is half duplex = %d\n", val);
  val = snd_pcm_hw_params_is_joint_duplex(params);
  printf("is joint duplex = %d\n", val);
  val = snd_pcm_hw_params_can_overrange(params);
  printf("can overrange = %d\n", val);
  val = snd_pcm_hw_params_can_mmap_sample_resolution(params);
  printf("can mmap = %d\n", val);
  val = snd_pcm_hw_params_can_pause(params);
  printf("can pause = %d\n", val);
  val = snd_pcm_hw_params_can_resume(params);
  printf("can resume = %d\n", val);
  val = snd_pcm_hw_params_can_sync_start(params);
  printf("can sync start = %d\n", val);
/*******************************************************************/
  play_save_stream.open("/data/audio/play_data.pcm",
      std::ofstream::binary);
  if (play_save_stream.is_open()) {
    KLOGI(TAG, "playback save for debug in AudioManager is open");
  }
#endif

  return kAMSuccess;
}

AMResult alsa_output_start(AlsaFormat *player) {
  AMResult result = kAMUnknownError;
  if (player->audio_state == ALSA_PLAYSTATE_PAUSED) {
    int resume = 0;
    result = snd_pcm_pause(player->handle, resume);
    if (result) {
      result = snd_pcm_start(player->handle);
      if (result < 0) {
        KLOGE(TAG, "snd_pcm_start failed!(%s)", snd_strerror(result));
        return kAMInternalError;
      }
    }
  } else if (player->audio_state == ALSA_PLAYSTATE_PLAYING) {
    return kAMSuccess;
  } else if (player->audio_state == ALSA_PLAYSTATE_STOPPED) {
    result = snd_pcm_prepare(player->handle);
    if (result < 0) {
      KLOGE(TAG, "snd_pcm_prepare failed!(%s)", snd_strerror(result));
      return kAMInternalError;
    }
  } else {
    result = snd_pcm_start(player->handle);
    if (result < 0) {
      KLOGE(TAG, "snd_pcm_start failed!(%s)", snd_strerror(result));
      return kAMInternalError;
    }
  }
  result = pthread_create(&player->thread_id, NULL, aplayer, player);
  if (result) {
    KLOGE(TAG, "pthread_create aplayer failed!");
    return kAMPreconditionsViolated;
  }
  pthread_detach(player->thread_id);
  player->audio_state = ALSA_PLAYSTATE_PLAYING;
  KLOGD(TAG, "alsa_output_start success!");
  return kAMSuccess;
}

AMResult alsa_output_pause(AlsaFormat *player) {
  int pause = 1; // resume = 0
  int result = 0;
  result = snd_pcm_pause(player->handle, pause);
  if (result) {
    KLOGE(TAG, "snd_pcm_pause failed! err_cod:%s(%d)",
        snd_strerror(result), result);
    return kAMInternalError;
  }
  player->audio_state = ALSA_PLAYSTATE_PAUSED;
  KLOGD(TAG, "alsa_output_pause success!");
  return kAMSuccess;
}

AMResult alsa_output_stop(AlsaFormat *player, bool drain) {
  int result = -1;

  if (!drain) {
    result = snd_pcm_drop(player->handle);
    if (result) {
      KLOGE(TAG, "snd_pcm_drain failed! err_code:%s(%d)", 
          snd_strerror(result), result);
      return kAMInternalError;
    }

    // audio file
    if (player->play_type == nuiam::kAMFileTypePCM) {
      if (player->fd) {
        fseek(player->fd, 0, SEEK_SET);
      } else {
        KLOGE(TAG, "audio file inexisted!");
        return kAMIoError;
      }
    }
    clean_circular_buffer(player->ring_buffer);
    player->audio_state = ALSA_PLAYSTATE_STOPPED;
  }
  KLOGD(TAG, "alsa_output_stop success!");
  return kAMSuccess;
}

AMResult alsa_output_close(AlsaFormat *player) {
  int result = 0;
  snd_pcm_drop(player->handle);
  result = snd_pcm_close(player->handle);
  if (result) {
    KLOGE(TAG, "snd_pcm_close failed! err_code:%s(%d)",
        snd_strerror(result), result);
    return kAMInternalError;
  }

  // audio file
  if (player->play_type == nuiam::kAMFileTypePCM) {
    if (player->fd) {
      result = fclose(player->fd);
      if (result) {
        KLOGE(TAG, "audio file close failed!(status)", result);
      }
    } else {
      KLOGE(TAG, "audio file inexisted!");
      return kAMIoError;
    }
  }

  free_circular_buffer(player->ring_buffer);
  player->audio_state = ALSA_PLAYSTATE_CLOSED;
  player->handle = NULL;
#ifdef _SND_DEBUG_
  if (play_save_stream.is_open()) {
    play_save_stream.close();
  }
#endif
  KLOGD(TAG, "alsa_output_close success!");
  return kAMSuccess;
}

AMResult alsa_input_open(AlsaFormat *capture) {
  int bufferByteSize = 0;
  int loop = 0;
  AMResult result = kAMUnknownError;
  char *pcm_name[] = {"hw:0,0", "hw:1,0", "hw:2,0", "hw:3,0", "default"};
  unsigned long latency = (unsigned long)(CAPTURE_PERIOD_TIME * 1000000);
  for (loop = 0; loop < 5; loop++) {
    result = snd_pcm_open(&capture->handle, pcm_name[loop], SND_PCM_STREAM_CAPTURE, 0);
    if (result < 0) {
      KLOGE(TAG, "snd_pcm_open (%s) failed!(%s)", pcm_name[loop], snd_strerror(result));
      result = kAMIoError;
      //return result;
    } else {
      KLOGI(TAG, "snd_pcm_open (%s) success!", pcm_name[loop]);
      break;
    }
  }
  if (loop >= 5) {
    return result;
  }
  if ((result = snd_pcm_set_params(capture->handle,
                                   (snd_pcm_format_t)capture->bit_per_sample,
                                   (snd_pcm_access_t)capture->format_type,
                                   capture->num_channels,
                                   capture->sample_rate,
                                   1,
                                   latency)) < 0) {
    KLOGE(TAG, "capture set params error:%s", snd_strerror(result));
    snd_pcm_close(capture->handle);
    return kAMInternalError;
  } else {
    KLOGD(TAG, "set params bit_per_sample:%d", capture->bit_per_sample);
    KLOGD(TAG, "set params format_type:%d", capture->format_type);
    KLOGD(TAG, "set params num_channels:%d", capture->num_channels);
    KLOGD(TAG, "set params sample_rate:%d", capture->sample_rate);
  }

  if ((result = snd_pcm_nonblock(capture->handle, 1)) < 0) {
    KLOGE(TAG, "capture set block failed");
    snd_pcm_close(capture->handle);
    return kAMInternalError;
  }

  // Set ALSA parameters
  snd_pcm_hw_params_t *params;
  unsigned int val, val2;
  int dir;
  snd_pcm_uframes_t frames;
  snd_pcm_hw_params_alloca(&params);
  snd_pcm_hw_params_any(capture->handle, params);
  /* Interleaved mode */
  snd_pcm_hw_params_set_access(capture->handle, params, capture->format_type);
  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(capture->handle, params, capture->bit_per_sample);
  /* 10 channels */
  snd_pcm_hw_params_set_channels(capture->handle, params, capture->num_channels);
  /* 44100 bits/second sampling rate (CD quality) */
  val = capture->sample_rate;
  snd_pcm_hw_params_set_rate_near(capture->handle, params, &val, &dir);
  /* Set period size to 320 frames. */
  frames = (int)(capture->sample_rate * CAPTURE_PERIOD_TIME);
  snd_pcm_hw_params_set_period_size_near(capture->handle, params, &frames, &dir);
  result = snd_pcm_hw_params(capture->handle, params);
  if (result < 0)
    KLOGE(TAG, "snd_pcm_hw_params failed!(%s)", snd_strerror(result));

  bufferByteSize = ComputeBufferSize(capture, CAPTURE_BUFFER_TIME);
  KLOGD(TAG, "compute buffer size is %d", bufferByteSize);
  capture->ring_buffer = create_circular_buffer(bufferByteSize * 4);
  if (capture->ring_buffer == NULL) {
    KLOGE(TAG, "alsa_input_open create ring buffer failed!");
    return kAMIoError;
  }

  pthread_mutex_init(&capture->mutex, NULL);
  capture->audio_state = ALSA_CAPTURESTATE_OPENED;

#ifdef _SND_DEBUG_
/******************打印参数*********************/
  snd_pcm_hw_params_get_channels(params, &val);
  printf("channels = %d\n", val);
  snd_pcm_hw_params_get_rate(params, &val, &dir);
  printf("rate = %d bps\n", val);
  snd_pcm_hw_params_get_period_time(params, &val, &dir);
  printf("period time = %d us\n", val);
  snd_pcm_hw_params_get_period_size(params, &frames, &dir);
  printf("period size = %ld frames\n", (long)frames);
  snd_pcm_hw_params_get_buffer_time(params, &val, &dir);
  printf("buffer time = %d us\n", val);
  snd_pcm_hw_params_get_buffer_size(params, (snd_pcm_uframes_t *)&val);
  printf("buffer size = %d frames\n", val);
  snd_pcm_hw_params_get_periods(params, &val, &dir);
  printf("periods per buffer = %d frames\n", val);
  snd_pcm_hw_params_get_rate_numden(params, &val, &val2);
  printf("exact rate = %d/%d bps\n", val, val2);
  val = snd_pcm_hw_params_get_sbits(params);
  printf("significant bits = %d\n", val);
  //snd_pcm_hw_params_get_tick_time(params,  &val, &dir);
  printf("tick time = %d us\n", val);
  val = snd_pcm_hw_params_is_batch(params);
  printf("is batch = %d\n", val);
  val = snd_pcm_hw_params_is_block_transfer(params);
  printf("is block transfer = %d\n", val);
  val = snd_pcm_hw_params_is_double(params);
  printf("is double = %d\n", val);
  val = snd_pcm_hw_params_is_half_duplex(params);
  printf("is half duplex = %d\n", val);
  val = snd_pcm_hw_params_is_joint_duplex(params);
  printf("is joint duplex = %d\n", val);
  val = snd_pcm_hw_params_can_overrange(params);
  printf("can overrange = %d\n", val);
  val = snd_pcm_hw_params_can_mmap_sample_resolution(params);
  printf("can mmap = %d\n", val);
  val = snd_pcm_hw_params_can_pause(params);
  printf("can pause = %d\n", val);
  val = snd_pcm_hw_params_can_resume(params);
  printf("can resume = %d\n", val);
  val = snd_pcm_hw_params_can_sync_start(params);
  printf("can sync start = %d\n", val);
  /*******************************************************************/

  alsa_save_stream.open("/data/audio/alsa_data.pcm",
      std::ofstream::binary);
  if (alsa_save_stream.is_open()) {
    KLOGI(TAG, "audio save for debug in AudioManager is open");
  }
#endif

  return kAMSuccess;
}

AMResult alsa_input_start(AlsaFormat *capture) {
  AMResult result = kAMUnknownError;
  if (capture->audio_state == ALSA_CAPTURESTATE_PAUSED) {
    int resume = 0;
    result = snd_pcm_pause(capture->handle, resume);
    if (result) {
      result = snd_pcm_start(capture->handle);
      if (result < 0) {
        KLOGE(TAG, "snd_pcm_start failed!(%s)", snd_strerror(result));
      } else {
        KLOGD(TAG, "snd_pcm_start sucess!");
      }
    }
  } else if (capture->audio_state == ALSA_CAPTURESTATE_CAPTURING) {
    return kAMSuccess;
  } else {
    result = snd_pcm_start(capture->handle);
    if (result < 0) {
      KLOGE(TAG, "snd_pcm_start failed!(%s)", snd_strerror(result));
    } else {
      KLOGD(TAG, "snd_pcm_start sucess!");
    }
  }
  result = pthread_create(&capture->thread_id, NULL, arecorder, capture);
  if (result) {
    KLOGE(TAG, "pthread_create arecorder failed!");
    return kAMPreconditionsViolated;
  }
  pthread_detach(capture->thread_id);
  capture->audio_state = ALSA_CAPTURESTATE_CAPTURING;
  return kAMSuccess;
}

AMResult alsa_input_pause(AlsaFormat *capture) {
  int pause = 1; // resume = 0
  int result = 0;
  result = snd_pcm_pause(capture->handle, pause);
  if (result) {
    KLOGE(TAG, "snd_pcm_pause failed!(%s)", snd_strerror(result));
    return kAMInternalError;
  }
  capture->audio_state = ALSA_CAPTURESTATE_PAUSED;
  return kAMSuccess;
}

AMResult alsa_input_stop(AlsaFormat *capture) {
  int result = -1;
  result = snd_pcm_drop(capture->handle);
  if (result) {
    KLOGE(TAG, "snd_pcm_drop failed! err_code:%s(%d)",
        snd_strerror(result),
        result);
    return kAMInternalError;
  }
  capture->audio_state = ALSA_CAPTURESTATE_STOPPED;
  return kAMSuccess;
}

AMResult alsa_input_close(AlsaFormat *capture) {
  int result = 0;
  snd_pcm_drop(capture->handle);
  result = snd_pcm_close(capture->handle);
  if (result) {
    KLOGE(TAG, "snd_pcm_close failed! err_code = %s(%d)",
        snd_strerror(result),
        result);
    return kAMInternalError;
  }
  free_circular_buffer(capture->ring_buffer);
  capture->audio_state = ALSA_CAPTURESTATE_CLOSED;
  capture->handle = NULL;

#ifdef _SND_DEBUG_
  if (alsa_save_stream.is_open()) {
    alsa_save_stream.close();
  }
#endif
  return kAMSuccess;
}

AMResult alsa_set_state(AlsaFormat *alsa, int alsa_state) {

}

} // namespace nuiam

