//
//  audio_ios_common.h
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-3-21.
//  Copyright (c) 2018 season4675. All rights reserved.
//

#ifndef AUDIOMANAGER_ENGINE_AUDIO_IOS_AUDIO_IOS_COMMON_H_
#define AUDIOMANAGER_ENGINE_AUDIO_IOS_AUDIO_IOS_COMMON_H_

#define kSampleRate8    8000
#define kSampleRate16   16000
#define kSampleRate44K1 44100
#define kSampleRate48   48000

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
  AudioUnitOutput *player;
  int output_state;
} AudioStream;

#endif // AUDIOMANAGER_ENGINE_AUDIO_IOS_AUDIO_IOS_COMMON_H_
