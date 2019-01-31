//
//  AudioInputQueue.h
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-3-1.
//  Copyright (c) 2018 season4675. All rights reserved.
//

#include <AudioToolbox/AudioToolbox.h>
#include <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#include <pthread.h>

//#include "CAStreamBasicDescription.h"
//#include "CAXException.h"

#ifndef kNumberRecordBuffers
#define kNumberRecordBuffers 3
#endif
#ifndef kBufferDurationSeconds
#define kBufferDurationSeconds 0.08
#endif

class AudioInputQueue
{
 public:
  AudioInputQueue();
  ~AudioInputQueue();

  SInt32 InitRecorder();
  SInt32 RemoveRecorder();
  SInt32 StartRecord();
  SInt32 PauseRecord();
  SInt32 StopRecord();
  Boolean IsRunning() const { return mIsRunning; }

  AudioStreamBasicDescription mRecordFormat;
  circular_buffer *inrb;
  char *inputBuffer;
  pthread_mutex_t input_mutex;

 private:
  AudioQueueRef mQueue;
  AudioQueueBufferRef mBuffers[kNumberRecordBuffers];
  SInt64 mRecordPacket; // current packet number in record file
  //AudioStreamBasicDescription mRecordFormat;
  Boolean mIsRunning;

  int ComputeRecordBufferSize(const AudioStreamBasicDescription *format,
                              float seconds);
  static void InputBufferHandler(
      void *inUserData,
      AudioQueueRef inAQ,
      AudioQueueBufferRef inBuffer,
      const AudioTimeStamp * inStartTime,
      UInt32 inNumPackets,
      const AudioStreamPacketDescription* inPacketDesc);
};
