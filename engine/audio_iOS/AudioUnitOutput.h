//
//  AudioUnitOutput.h
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-4-23.
//  Copyright (c) 2018 season4675. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>
#include <pthread.h>
#include "audio_common.h"

#ifndef kNumberPlayBuffers
#define kNumberPlayBuffers 3
#endif
#ifndef kBufferDurationSeconds
#define kBufferDurationSeconds 0.125
#endif
#ifndef kInputBus
#define kInputBus 1
#endif
#ifndef kOutputBus
#define kOutputBus 0
#endif

class AudioUnitOutput
{
 public:
  AudioUnitOutput();
  ~AudioUnitOutput();

  void AudioUnitRegisterListener(const audiomanager::AMEventListener &listener);
  SInt32 InitPlayer();
  SInt32 RemovePlayer();
  SInt32 StartPlay();
  SInt32 PausePlay();
  SInt32 StopPlay(bool drain = false);

  SInt32 SetVolume(int vol);
  SInt32 GetVolume();

  Boolean IsRunning() const { return mIsRunning; }

  AudioStreamBasicDescription mPlayFormat;
  circular_buffer *outrb;
  pthread_mutex_t output_mutex;
  int bufSizeDurationOneSec;

  //audio file descriptor
  int play_type;
  const char *locator;
  FILE *fd;
  int start;
  int length;
  bool is_drained;
  bool mIsPaused;
  audiomanager::AMEventListener audio_listener_;
 private:
  AudioUnit mPlayUnit;
  AudioBufferList *playBufList;
  Boolean mIsRunning;

  int ComputePlayBufferSize(const AudioStreamBasicDescription *format,
                            float seconds);
  static OSStatus PlayCallback(
      void *inRefCon,
      AudioUnitRenderActionFlags *ioActionFlags,
      const AudioTimeStamp *inTimeStamp,
      UInt32 inBusNumber,
      UInt32 inNumberFrames,
      AudioBufferList *ioData);
};
