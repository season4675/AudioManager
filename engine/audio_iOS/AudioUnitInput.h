//
//  AudioUnitInput.h
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-4-14.
//  Copyright (c) 2018 season4675. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>
#include <pthread.h>

#ifndef kNumberRecordBuffers
#define kNumberRecordBuffers 3
#endif
#ifndef kBufferDurationSeconds
#define kBufferDurationSeconds 0.02
#endif
#ifndef kInputBus
#define kInputBus 1
#endif
#ifndef kOutputBus
#define kOutputBus 0
#endif

class AudioUnitInput
{
 public:
  AudioUnitInput();
  ~AudioUnitInput();

  SInt32 InitRecorder();
  SInt32 RemoveRecorder();
  SInt32 StartRecord();
  SInt32 PauseRecord();
  SInt32 StopRecord();
  Boolean IsRunning() const { return mIsRunning; }

  AudioStreamBasicDescription mRecordFormat;
  circular_buffer *inrb;
  pthread_mutex_t input_mutex;

 private:
  AudioUnit mRecordUnit;
  AudioBufferList *recordBufList;
  Boolean mIsRunning;

  int ComputeRecordBufferSize(const AudioStreamBasicDescription *format,
                              float seconds);
  static OSStatus RecordCallback(
      void *inRefCon,
      AudioUnitRenderActionFlags *ioActionFlags,
      const AudioTimeStamp *inTimeStamp,
      UInt32 inBusNumber,
      UInt32 inNumberFrames,
      AudioBufferList *ioData);
  static OSStatus PlayCallback(
      void *inRefCon,
      AudioUnitRenderActionFlags *ioActionFlags,
      const AudioTimeStamp *inTimeStamp,
      UInt32 inBusNumber,
      UInt32 inNumberFrames,
      AudioBufferList *ioData);
};
