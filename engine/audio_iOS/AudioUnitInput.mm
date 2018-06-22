//
//  AudioUnitInput.mm
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-4-14.
//  Copyright (c) 2018 season4675. All rights reserved.
//

#include "audio_log.h"
#include "circular_buffer.h"
#include "AudioUnitInput.h"

#ifdef _USE_PLAYER_
#undef _USE_PLAYER_
#endif

//______________________________________________________________________________
// Determine the size, in bytes,
// of a buffer necessary to represent the supplied number
// of seconds of audio data.
int AudioUnitInput::ComputeRecordBufferSize(
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
OSStatus AudioUnitInput::RecordCallback(
    void *inRefCon,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData)
{
  int count = 0;
  AudioUnitInput *ars = (AudioUnitInput *)inRefCon;
  AudioUnitRender(ars->mRecordUnit,
                  ioActionFlags,
                  inTimeStamp,
                  inBusNumber,
                  inNumberFrames,
                  ars->recordBufList);
  pthread_mutex_lock(&ars->input_mutex);
  count = write_circular_buffer_bytes(ars->inrb,
                                      (char *)ars->recordBufList->mBuffers[0].mData,
                                      ars->recordBufList->mBuffers[0].mDataByteSize);
  pthread_mutex_unlock(&ars->input_mutex);
  //NSLog(@"count=%d %dbytes", count, ars->recordBufList->mBuffers[0].mDataByteSize);
  return noErr;
}

#ifdef _USE_PLAYER_
OSStatus AudioUnitInput::PlayCallback(
    void *inRefCon,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData) {
  AudioUnitInput *ars = (AudioUnitInput *)inRefCon;
  AudioUnitRender(ars->mRecordUnit, ioActionFlags, inTimeStamp, 1, inNumberFrames, ioData);
  return noErr;
}
#endif

AudioUnitInput::AudioUnitInput() {
  mRecordUnit = NULL;
}

AudioUnitInput::~AudioUnitInput() {
  mRecordUnit = NULL;
}

SInt32 AudioUnitInput::InitRecorder() {
  OSStatus status = -1;
  UInt32 bufferByteSize = 0;

  if (NULL == mRecordUnit) {
    // 1.Init AudioUnit
    AudioUnitInitialize(mRecordUnit);

    // 2.init AudioSession
    NSError *error;
    AVAudioSession *audioSession = [AVAudioSession sharedInstance];
    //[audioSession setCategory:AVAudioSessionCategoryPlayAndRecord error:&error];
    [audioSession setCategory:AVAudioSessionCategoryRecord error:&error];
    //[audioSession setPreferredSampleRate:16000 error:&error];
    [audioSession setPreferredSampleRate:mRecordFormat.mSampleRate error:&error];
    //[audioSession setPreferredInputNumberOfChannels:1 error:&error];
    [audioSession setPreferredInputNumberOfChannels:mRecordFormat.mChannelsPerFrame error:&error];
    //[audioSession setPreferredIOBufferDuration:0.02 error:&error];
    [audioSession setPreferredIOBufferDuration:kBufferDurationSeconds error:&error];

    // 3.init Buffer
    bufferByteSize = ComputeRecordBufferSize(&mRecordFormat,
                                             kBufferDurationSeconds);
    NSLog(@"Compute recorder %f seconds data buffer size is %d.",
        kBufferDurationSeconds,
        bufferByteSize);
    UInt32 flag = 0;
    status = AudioUnitSetProperty(mRecordUnit,
                                  kAudioUnitProperty_ShouldAllocateBuffer,
                                  kAudioUnitScope_Output,
                                  kInputBus,
                                  &flag,
                                  sizeof(flag));
    recordBufList = (AudioBufferList *)malloc(sizeof(AudioBufferList));
    recordBufList->mNumberBuffers = 1;
    recordBufList->mBuffers[0].mNumberChannels = mRecordFormat.mChannelsPerFrame;
    //recordBufList->mBuffers[0].mDataByteSize = 2048 * sizeof(short);
    //recordBufList->mBuffers[0].mData = (short *)malloc(2048 * sizeof(short));
    recordBufList->mBuffers[0].mDataByteSize = bufferByteSize;
    recordBufList->mBuffers[0].mData = (short *)malloc(bufferByteSize);

    // 4.init Audio Component
    AudioComponentDescription recorderDesc;
    recorderDesc.componentType = kAudioUnitType_Output;
    recorderDesc.componentSubType = kAudioUnitSubType_RemoteIO;
    //recorderDesc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
    recorderDesc.componentFlags = 0;
    recorderDesc.componentFlagsMask = 0;
    recorderDesc.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent inputComponent = AudioComponentFindNext(NULL, &recorderDesc);
    status = AudioComponentInstanceNew(inputComponent, &mRecordUnit);
    if (status) NSLog(@"AudioComponentInstanceNew failed(%d)",status);

    // 5.init Format
    AudioUnitSetProperty(mRecordUnit,
                         kAudioUnitProperty_StreamFormat,
                         kAudioUnitScope_Output,
                         kInputBus,
                         &mRecordFormat,
                         sizeof(mRecordFormat));

    AudioUnitSetProperty(mRecordUnit,
                         kAudioUnitProperty_StreamFormat,
                         kAudioUnitScope_Input,
                         kOutputBus,
                         &mRecordFormat,
                         sizeof(mRecordFormat));

    // 6.init Audio Property
    UInt32 recordFlag = 1; 
    AudioUnitSetProperty(mRecordUnit,
                         kAudioOutputUnitProperty_EnableIO,
                         kAudioUnitScope_Input,
                         kInputBus,
                         &recordFlag,
                         sizeof(recordFlag));
    UInt32 playFlag = 0;
    AudioUnitSetProperty(mRecordUnit,
                         kAudioOutputUnitProperty_EnableIO,
                         kAudioUnitScope_Input,
                         kOutputBus,
                         &playFlag,
                         sizeof(playFlag));

    // 7.init RecordCallback
    AURenderCallbackStruct recordCallback;
    recordCallback.inputProc = RecordCallback;
    recordCallback.inputProcRefCon = this;
    status = AudioUnitSetProperty(mRecordUnit,
                                  kAudioOutputUnitProperty_SetInputCallback,
                                  kAudioUnitScope_Global,
                                  kInputBus,
                                  &recordCallback,
                                  sizeof(recordCallback));
    if (status) NSLog(@"AudioUnit init RecordCallback failed(%d)!",status);

#ifdef _USE_PLAYER_
    // 8.init PlayCallback
    AURenderCallbackStruct playCallback;
    playCallback.inputProc = PlayCallback;
    playCallback.inputProcRefCon = this;
    AudioUnitSetProperty(mRecordUnit,
                         kAudioUnitProperty_SetRenderCallback,
                         kAudioUnitScope_Global,
                         kOutputBus,
                         &playCallback,
                         sizeof(playCallback));
#endif

    // 9.calloc circular buffers
    inrb = create_circular_buffer(bufferByteSize * 4);
    if (NULL == inrb)
      NSLog(@"Recorder calloc buffer failed!");
  }

  return status;
}

SInt32 AudioUnitInput::RemoveRecorder() {
  OSStatus status = -1;
  free(recordBufList->mBuffers[0].mData);
  free(recordBufList);
  status = AudioUnitUninitialize(mRecordUnit);
  if (inrb != NULL) {
    free_circular_buffer(inrb);
    inrb = NULL;
  }
  return status;
}

SInt32 AudioUnitInput::StartRecord() {
  OSStatus status = -1;
  status = AudioOutputUnitStart(mRecordUnit);
  //NSLog(@"AudioUnit StartRecord(%d)",status);
  return status;
}

SInt32 AudioUnitInput::PauseRecord() {
  OSStatus status = -1;
  return status;
}

SInt32 AudioUnitInput::StopRecord() {
  OSStatus status = -1;
  status = AudioOutputUnitStop(mRecordUnit);
  //if (inrb != NULL)
    //clean_circular_buffer(inrb);
  //NSLog(@"AudioUnit StopRecord(%d)",status);
  return status;
}
