/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/engine/audio_openSL/opensl_io.h
 */

#ifndef AUDIOMANAGER_ENGINE_AUDIO_OPENSL_OPENSL_IO_H_
#define AUDIOMANAGER_ENGINE_AUDIO_OPENSL_OPENSL_IO_H_

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PLAYER_MAX
#define PLAYER_MAX 8
#endif

#ifndef MIN_VOL_LEVEL
#define MIN_VOL_LEVEL 0
#endif
#ifndef MAX_VOL_LEVEL
#define MAX_VOL_LEVEL 100
#endif

enum SLFileType {
  kSLFileTypeNONE = 0,
  kSLFileTypePCM = 1,
  kSLFileTypeURI = 2,
  kSLFileTypeASSETS = 3,
  kSLFileTypeMAX = 4,
};

typedef struct circular_buffer_ {
  char *buffer;
  int write_pos;
  int read_pos;
  int size;
} circular_buffer;

typedef struct threadLock_{
  pthread_mutex_t m;
  pthread_cond_t c;
  unsigned char s;
} threadLock;

typedef struct OpenslStream_ {
  // engine interfaces
  SLObjectItf engine_obj;
  SLEngineItf engine_itf;

  // engine capabilities interface
  SLEngineCapabilitiesItf engine_cap_itf;

  // output mix interfaces
  SLObjectItf output_mix_obj;
  SLEnvironmentalReverbItf output_mix_env_reverb;

  // buffer queue player interfaces
  SLObjectItf bq_player_obj[PLAYER_MAX];
  SLPlayItf bq_player_play[PLAYER_MAX];
  SLAndroidSimpleBufferQueueItf bq_player_buf_que[PLAYER_MAX];
  SLVolumeItf bq_player_vol[PLAYER_MAX];
  //SLEffectSendItf bq_player_effect_send;

  // recorder interfaces
  SLObjectItf recorder_obj;
  SLRecordItf recorder_record;
  SLAndroidSimpleBufferQueueItf recorder_buf_que;

  // buffers
  char *output_buf[PLAYER_MAX][2];
  char *input_buf[2];

  // size of buffers
  int out_buf_samples[PLAYER_MAX];
  int in_buf_samples;

  double time;

  // output parameters
  int out_channels[PLAYER_MAX];
  SLDataFormat_PCM out_format_pcm[PLAYER_MAX];
  SLuint32 output_state[PLAYER_MAX];
  SLuint32 play_type[PLAYER_MAX];
  SLuint8 player_channel_mask;
  int player_num;
  SLmillibel max_vol_level;

  // input parameters
  int in_channels;
  SLDataFormat_PCM in_format_pcm;
  SLuint32 input_state;
  SLuint32 rec_type;

  // audio file descriptor
  const char *locator;
  int fd;
  int start;
  int length;

  // locks
  void *inlock;
  void *outlock[PLAYER_MAX];

  // current buffer half (0, 1)
  int cur_output_buf[PLAYER_MAX];
  int cur_input_buf;

  // buffer indexes
  int cur_input_index;
  int cur_output_index[PLAYER_MAX];
} OpenslStream;

void* createThreadLock(void);
int waitThreadLock(void *lock);
void notifyThreadLock(void *lock);
void destroyThreadLock(void *lock);

SLresult openSL_create_engine(OpenslStream *opensl_stream);
SLresult openSL_destroy_engine(OpenslStream *opensl_stream);
SLresult openSL_create_output_mix(OpenslStream *opensl_stream);
SLresult openSL_destroy_output_mix(OpenslStream *opensl_stream);
SLresult openSL_init_player(OpenslStream *opensl_stream, int player_id);
SLresult openSL_remove_player(OpenslStream *opensl_stream, int player_id);
SLresult openSL_init_recorder(OpenslStream *opensl_stream);
SLresult openSL_remove_recorder(OpenslStream *opensl_stream);

SLresult openSL_get_version(OpenslStream *opensl_stream, char *engine_version);

SLresult openSL_set_player_state(OpenslStream *opensl_stream,
                                 int player_id,
                                 SLuint32 audio_state);
SLresult openSL_set_recorder_state(OpenslStream *opensl_stream,
                                   SLuint32 audio_state);
SLresult openSL_set_player_vol(OpenslStream *opensl_stream,
                               int vol,
                               int player_id);
SLresult openSL_get_player_vol(OpenslStream *opensl_stream, int player_id);
SLresult openSL_set_player_mute(OpenslStream *opensl_stream,
                                int player_id,
                                bool mute);

int openSL_write_enqueue(OpenslStream *opensl_stream,
                         void *buffer,
                         int size,
                         int player_id);
int openSL_read_enqueue(OpenslStream *opensl_stream,
                        void *buffer,
                        int size);
#ifdef __cplusplus
}
#endif

#endif // AUDIOMANAGER_ENGINE_AUDIO_OPENSL_OPENSL_IO_H_
