//
//  AudioOutputQueue.mm
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-3-7.
//  Copyright (c) 2018 season4675. All rights reserved.
//

#include "audio_log.h"
#include "circular_buffer.h"
#include "AudioOutputQueue.h"

//______________________________________________________________________________
// Determine the size, in bytes,
// of a buffer necessary to represent the supplied number
// of seconds of audio data.
int AudioOutputQueue::ComputePlayBufferSize(
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
// AudioQueue callback function, called when an output buffers has been filled.
void AudioOutputQueue::OutputBufferCallback(
    void * inUserData,
    AudioQueueRef inAQ,
    AudioQueueBufferRef inBuffer) {
  AudioOutputQueue *aqr = (AudioOutputQueue *)inUserData;
  SInt32 byteSize = 0;

  try {
    if (inBuffer != NULL && inBuffer->mAudioDataByteSize > 0) {
      pthread_mutex_lock(&aqr->output_mutex);
      byteSize = read_circular_buffer_bytes(aqr->outrb,
                                            (char *)inBuffer->mAudioData,
                                            inBuffer->mAudioDataByteSize);
      pthread_mutex_unlock(&aqr->output_mutex);
      //NSLog(@"outputcallback read=%d",byteSize);
    }
    
    // if we're not stopping, re-enqueue the buffe so that it gets filled again
    if (aqr->IsRunning() && byteSize > 0) {
      inBuffer->mAudioDataByteSize = byteSize;
      AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
    }
  } catch (NSException *e) {
    NSLog(@"Error: %@ (%@)", e.name, e.reason);
  }
}

AudioOutputQueue::AudioOutputQueue() {
  mIsRunning = false;
  outputBuffer = NULL;
  memset(&mPlayFormat, 0, sizeof(mPlayFormat));
}

AudioOutputQueue::~AudioOutputQueue() {
  if (mQueue != NULL) {
    AudioQueueDispose(mQueue, true);
    mQueue = NULL;
    mIsRunning = false;
  }
}

SInt32 AudioOutputQueue::InitPlayer() {
  int i, bufferByteSize;
  //UInt32 size;
  OSStatus status = -1; 
  try {    
    // create the queue
    status = AudioQueueNewOutput(&mPlayFormat,
                                 OutputBufferCallback,
                                 this /* userData */,
                                 NULL /* run loop */,
                                 NULL /* run loop mode */,
                                 0 /* flags */,
                                 &mQueue);

    if (!status) {
      LOGD("Create Audio_Queue_New_Output success.");
    } else {
      LOGE("Create Audio_Queue_New_Output failed(%d).", status);
    }
    // allocate and enqueue buffers
    // enough bytes for half a second$
    bufferByteSize = ComputePlayBufferSize(&mPlayFormat,
                                           kBufferDurationSeconds);
    LOGD("Compute player buffer size is %d.", bufferByteSize);
    for (i = 0; i < kNumberPlayBuffers; ++i) {
      status = AudioQueueAllocateBuffer(mQueue, bufferByteSize, &mBuffers[i]);
      if (status) {
        LOGE("PlayBufferNum %d: Audio Queue Allocate Buffer failed(%d).",
            i,
            status);
      }
      memset(mBuffers[i]->mAudioData, 0, bufferByteSize);
      mBuffers[i]->mAudioDataByteSize = bufferByteSize;
      status = AudioQueueEnqueueBuffer(mQueue, mBuffers[i], 0, NULL);
      if (status) {
        LOGE("PlayBufferNum %d: Audio Queue Enqueue Buffer failed(%d).",
            i,
            status);
      }
    }
    // create circular buffer
    outrb = create_circular_buffer(bufferByteSize * 4);
    outputBuffer = (char *)calloc(bufferByteSize, sizeof(char));

    // start the queue
    //mIsRunning = true;
    //XThrowIfError(AudioQueueStart(mQueue, NULL), "AudioQueueStart failed");
  } catch (NSException *e) {
    NSLog(@"Error: %@ (%@)", e.name, e.reason);
  }
  catch (...) {
    fprintf(stderr, "An unknown error occurred\n");;
  }

  return status;
}

SInt32 AudioOutputQueue::RemovePlayer() {
  OSStatus status = -1;
  mIsRunning = false;
  if (mQueue) {
    status = AudioQueueDispose(mQueue, true);
    mQueue = NULL;
  }
  if (outputBuffer != NULL) {
    free(outputBuffer);
    outputBuffer = NULL;
  }
  if (outrb != NULL) {
    free_circular_buffer(outrb);
    outrb = NULL;
  }

  return status;
}

SInt32 AudioOutputQueue::StartPlay() {
  OSStatus status = -1;
  // start the queue
  mIsRunning = true;
  status = AudioQueueStart(mQueue, NULL);
  if (!status) {
    AudioQueueFlush(mQueue);
    status = AudioQueueStart(mQueue, NULL);
  }
  return status;
}

SInt32 AudioOutputQueue::PausePlay() {
  OSStatus status = -1;
  // pause playing
  mIsRunning = false;
  status = AudioQueuePause(mQueue);
  return status;
}

SInt32 AudioOutputQueue::StopPlay() {
  OSStatus status = -1;
  // end playing
  //mIsRunning = false;
  //AudioQueueReset(mQueue);
  status = AudioQueueStop(mQueue, true);
  mIsRunning = false;
  clean_circular_buffer(outrb);
  return status;
}

SInt32 AudioOutputQueue::SetVolume(int volume) {
  OSStatus status = -1;
  AudioQueueParameterValue inValue = 1.0;

  inValue = (AudioQueueParameterValue)volume /
      (MAX_VOL_LEVEL - MIN_VOL_LEVEL) * 1.0;

  status = AudioQueueSetParameter(mQueue, kAudioQueueParam_Volume, inValue);
  return status;
}

SInt32 AudioOutputQueue::GetVolume() {
  AudioQueueParameterValue value = 1.0;
  int volume = 0;

  AudioQueueGetParameter(mQueue, kAudioQueueParam_Volume, &value);
  volume = (int)(value * (MAX_VOL_LEVEL - MIN_VOL_LEVEL));

  return volume;
}

