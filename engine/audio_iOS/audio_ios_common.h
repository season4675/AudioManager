//
//  audio_ios_common.h
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-3-21.
//  Copyright (c) 2018 season4675. All rights reserved.
//

#ifndef AUDIOMANAGER_ENGINE_AUDIO_IOS_AUDIO_IOS_COMMON_H_
#define AUDIOMANAGER_ENGINE_AUDIO_IOS_AUDIO_IOS_COMMON_H_

#define kSampleRate8    8000.0
#define kSampleRate16   16000.0
#define kSampleRate24   24000.0
#define kSampleRate44K1 44100.0
#define kSampleRate48   48000.0

#ifndef PLAYER_MAX
#define PLAYER_MAX 8
#endif

enum AudioState {
  kAudioStateStopped   = 1,
  kAudioStatePaused    = 2,
  kAudioStatePlaying   = 3,
  kAudioStateRecording = 4,
};

typedef struct AudioStream_ {
  //AudioInputQueue *recorder;
  AudioUnitInput *recorder;
  int input_state;

  //AudioOutputQueue *player;
  AudioUnitOutput *player[PLAYER_MAX];
  UInt8 player_channel_mask;
  int output_state[PLAYER_MAX];

  int max_buf_size;
  int output_event;
  int output_interval;
} AudioStream;

#endif // AUDIOMANAGER_ENGINE_AUDIO_IOS_AUDIO_IOS_COMMON_H_
