//
//  AudioInputQueue.mm
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-3-1.
//  Copyright (c) 2018 season4675. All rights reserved.
//

#include "audio_log.h"
#include "circular_buffer.h"
#include "AudioInputQueue.h"

//______________________________________________________________________________
// Determine the size, in bytes,
// of a buffer necessary to represent the supplied number
// of seconds of audio data.
int AudioInputQueue::ComputeRecordBufferSize(
    const AudioStreamBasicDescription *format,
    float seconds) {
  int packets, frames, bytes = 0;
  try {
    frames = (int)ceil(seconds * format->mSampleRate);

    if (format->mBytesPerFrame > 0)
      bytes = frames * format->mBytesPerFrame;
    else {
      UInt32 maxPacketSize = 0;
      if (format->mBytesPerPacket > 0)
        maxPacketSize = format->mBytesPerPacket;  // constant packet size
      if (format->mFramesPerPacket > 0)
        packets = frames / format->mFramesPerPacket;
      else
        packets = frames;  // worst-case scenario: 1 frame in a packet
      if (0 == packets)    // sanity check
        packets = 1;
      bytes = packets * maxPacketSize;
    }
  } catch (NSException *e) {
    NSLog(@"Error: %@ (%@)", e.name, e.reason);
    return 0;
  }  
  return bytes;
}

//______________________________________________________________________________
// AudioQueue callback function, called when an input buffers has been filled.
void AudioInputQueue::InputBufferHandler(
    void * inUserData,
    AudioQueueRef inAQ,
    AudioQueueBufferRef inBuffer,
    const AudioTimeStamp * inStartTime,
    UInt32 inNumPackets,
    const AudioStreamPacketDescription*  inPacketDesc) {
  AudioInputQueue *aqr = (AudioInputQueue *)inUserData;
  SInt32 byteSize = 0;

  try {
    if (inNumPackets > 0 &&
        inBuffer != NULL &&
        inBuffer->mAudioDataByteSize > 0) {
      pthread_mutex_lock(&aqr->input_mutex);
      byteSize = write_circular_buffer_bytes(aqr->inrb,
                                             (char *)inBuffer->mAudioData,
                                             inBuffer->mAudioDataByteSize);
      pthread_mutex_unlock(&aqr->input_mutex);
    } else {
      NSLog(@"InputBufferHandler Error: inNumPackets=%d audioDataByteSize=%d",
          inNumPackets,
          inBuffer->mAudioDataByteSize);
    }
   
    // if we're not stopping, re-enqueue the buffe so that it gets filled again
    if (aqr->IsRunning()) {
      AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
    } else {
      NSLog(@"InputBufferHandler Error: byteSize=%d isRunning=%d",
          byteSize, aqr->IsRunning());
    }
  } catch (NSException *e) {
    NSLog(@"Error: %@ (%@)", e.name, e.reason);
  }
}

AudioInputQueue::AudioInputQueue() {
  mIsRunning = false;
  inputBuffer = NULL;
  mQueue = NULL;
  memset(&mRecordFormat, 0, sizeof(mRecordFormat));
}

AudioInputQueue::~AudioInputQueue() {
  if (mQueue != NULL) {
    AudioQueueDispose(mQueue, true);
    mIsRunning = false;
    mQueue = NULL;
  }
}

SInt32 AudioInputQueue::InitRecorder() {
  int i, bufferByteSize;
  OSStatus status = -1; 
  try {
    // create the queue
    if (NULL == mQueue) {
      status = AudioQueueNewInput(&mRecordFormat,
                                  InputBufferHandler,
                                  this /* userData */,
                                  NULL /* run loop */,
                                  kCFRunLoopCommonModes,
                                  0 /* flags */,
                                  &mQueue);
      if (!status) {
        LOGD("Create Audio_Queue_New_Input success.");
      } else {
        LOGE("Create Audio_Queue_New_Input failed(%d).", status);
      }
      // allocate and enqueue buffers
      // enough bytes for half a second$
      bufferByteSize = ComputeRecordBufferSize(&mRecordFormat,
                                               kBufferDurationSeconds);
      LOGD("Compute recorder %f seconds data buffer size is %d.",
          kBufferDurationSeconds,
          bufferByteSize);
      for (i = 0; i < kNumberRecordBuffers; ++i) {
        status = AudioQueueAllocateBuffer(mQueue, bufferByteSize, &mBuffers[i]);
        if (status) {
          LOGE("RecordBufferNum %d: Audio Queue Allocate Buffer failed(%d).",
              i,
              status);
        }
        status = AudioQueueEnqueueBuffer(mQueue, mBuffers[i], 0, NULL);
        if (status) {
          LOGE("RecordBufferNum %d: Audio Queue Enqueue Buffer failed(%d).",
              i,
              status);
        }
      }
    } //if (NULL == mQueue)

    // create circular buffer
    inrb = create_circular_buffer(bufferByteSize * 4);
    inputBuffer = (char *)calloc(bufferByteSize, sizeof(char));
    if (NULL == inrb || NULL == inputBuffer)
      LOGE("Recorder calloc buffer failed!");
  } catch (NSException *e) {
    NSLog(@"Error: %@ (%@)", e.name, e.reason);
  } catch (...) {
    fprintf(stderr, "An unknown error occurred\n");;
  }

  return status;
}

SInt32 AudioInputQueue::RemoveRecorder() {
  OSStatus status = -1;
  mIsRunning = false;
  if (mQueue != NULL) {
    status = AudioQueueDispose(mQueue, true);
    mQueue = NULL;
  }
  if (inputBuffer != NULL) {
    free(inputBuffer);
    inputBuffer = NULL;
  }
  if (inrb != NULL) {
    free_circular_buffer(inrb);
    inrb = NULL;
  }
  return status;
}

SInt32 AudioInputQueue::StartRecord() {
  OSStatus status = -1;
  int i = 0;
  // start the queue
  mIsRunning = true;
  status = AudioQueueStart(mQueue, NULL);
  if (!status) {
    AudioQueueFlush(mQueue);
    status = AudioQueueStart(mQueue, NULL);
  }
  return status;
}

SInt32 AudioInputQueue::PauseRecord() {
  OSStatus status = -1;
  // pause the queue
  mIsRunning = false;
  status = AudioQueuePause(mQueue);
  return status;
}

SInt32 AudioInputQueue::StopRecord() {
  OSStatus status = -1;
  // end recording
  //AudioQueueReset(mQueue);
  //mIsRunning = false;
  status = AudioQueueStop(mQueue, true);
  mIsRunning = false;
  clean_circular_buffer(inrb);
  return status;
}
