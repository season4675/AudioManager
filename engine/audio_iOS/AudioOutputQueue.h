//
//  AudioOutputQueue.h
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-3-6.
//  Copyright (c) 2018 season4675. All rights reserved.
//

#include <AudioToolbox/AudioToolbox.h>
#include <Foundation/Foundation.h>
#include <pthread.h>

//#include "CAStreamBasicDescription.h"
//#include "CAXException.h"

#ifndef kNumberPlayBuffers
#define kNumberPlayBuffers 3
#endif
#ifndef kBufferDurationSeconds
#define kBufferDurationSeconds 0.1
#endif
#ifndef MIN_VOL_LEVEL
#define MIN_VOL_LEVEL 0
#endif
#ifndef MAX_VOL_LEVEL
#define MAX_VOL_LEVEL 100
#endif

class AudioOutputQueue
{
 public:
  AudioOutputQueue();
  ~AudioOutputQueue();

  SInt32 InitPlayer();
  SInt32 RemovePlayer();

  SInt32 StartPlay();
  SInt32 PausePlay();
  SInt32 StopPlay();

  SInt32 SetVolume(int vol);
  SInt32 GetVolume();

  Boolean IsRunning() const { return mIsRunning; }

  AudioStreamBasicDescription mPlayFormat;
  circular_buffer *outrb;
  char *outputBuffer;
  pthread_mutex_t output_mutex;
  int bufSizeDurationOneSec;

 private:
  AudioQueueRef mQueue;
  AudioQueueBufferRef mBuffers[kNumberPlayBuffers];
  SInt64 mPlayPacket; // current packet number in record file
  //AudioStreamBasicDescription mPlayFormat;
  Boolean mIsRunning;

  int ComputePlayBufferSize(const AudioStreamBasicDescription *format,
                            float seconds);
  static void OutputBufferCallback(
      void *inUserData,
      AudioQueueRef inAQ,
      AudioQueueBufferRef inBuffer);
};
