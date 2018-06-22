//
//  AudioUnitOutput.mm
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-4-14.
//  Copyright (c) 2018 season4675. All rights reserved.
//

#include "audio_log.h"
#include "circular_buffer.h"
#include "AudioUnitOutput.h"

//______________________________________________________________________________
// Determine the size, in bytes,
// of a buffer necessary to represent the supplied number
// of seconds of audio data.
int AudioUnitOutput::ComputePlayBufferSize(
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
// AudioUnit callback function.
OSStatus AudioUnitOutput::PlayCallback(
    void *inRefCon,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData) {
  AudioUnitOutput *ars = (AudioUnitOutput *)inRefCon;
  int bytecount = 0;
  memset(ioData->mBuffers[0].mData, 0, ioData->mBuffers[0].mDataByteSize);

  pthread_mutex_lock(&ars->output_mutex);
  bytecount = read_circular_buffer_bytes(ars->outrb,
                                         (char *)ioData->mBuffers[0].mData,
                                         ioData->mBuffers[0].mDataByteSize);
  pthread_mutex_unlock(&ars->output_mutex);

  ioData->mBuffers[0].mDataByteSize = bytecount;

  return noErr;
}

AudioUnitOutput::AudioUnitOutput() {
  mPlayUnit = NULL;
}

AudioUnitOutput::~AudioUnitOutput() {
  mPlayUnit = NULL;
}

SInt32 AudioUnitOutput::InitPlayer() {
  OSStatus status = -1;
  UInt32 bufferByteSize = 0;

  if (NULL == mPlayUnit) {

    // 1.init AudioSession
    NSError *error;
    AVAudioSession *audioSession = [AVAudioSession sharedInstance];
    [audioSession setCategory:AVAudioSessionCategoryPlayAndRecord error:&error];

    // 2.init Audio Component
    AudioComponentDescription playerDesc;
    playerDesc.componentType = kAudioUnitType_Output;
    playerDesc.componentSubType = kAudioUnitSubType_RemoteIO;
    // 如果你的应用程序需要去除回声将componentSubType
    // 设置为kAudioUnitSubType_VoiceProcessingIO
    //playerDesc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
    playerDesc.componentFlags = 0;
    playerDesc.componentFlagsMask = 0;
    playerDesc.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent outputComponent = AudioComponentFindNext(NULL, &playerDesc);
    status = AudioComponentInstanceNew(outputComponent, &mPlayUnit);
    if (status) NSLog(@"AudioComponentInstanceNew failed(%d)", status);

    // 3.init Buffer
    bufferByteSize = ComputePlayBufferSize(&mPlayFormat,
                                           kBufferDurationSeconds);
    NSLog(@"Compute player %f seconds data buffer size is %d.",
        kBufferDurationSeconds,
        bufferByteSize);
    playBufList = (AudioBufferList *)malloc(sizeof(AudioBufferList));
    playBufList->mNumberBuffers = 1;
    playBufList->mBuffers[0].mNumberChannels = mPlayFormat.mChannelsPerFrame;
    playBufList->mBuffers[0].mNumberChannels = 1;
    playBufList->mBuffers[0].mDataByteSize = bufferByteSize;
    playBufList->mBuffers[0].mData = malloc(bufferByteSize);


    // 4.init Audio Property
    UInt32 playFlag = 1;
    status = AudioUnitSetProperty(mPlayUnit,
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Output,
                                  kOutputBus,
                                  &playFlag,
                                  sizeof(playFlag));
    if (status) {
        NSLog(@"AudioUnitSetProperty error with status:%d", status);
    }

    // 5.init Format
    // mFormatFlags错了会导致无法播放
    mPlayFormat.mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger; // 整形
    AudioUnitSetProperty(mPlayUnit,
                         kAudioUnitProperty_StreamFormat,
                         kAudioUnitScope_Input,
                         kOutputBus,
                         &mPlayFormat,
                         sizeof(mPlayFormat));

    // 6.init PlayCallback
    AURenderCallbackStruct playCallback;
    playCallback.inputProc = PlayCallback;
    playCallback.inputProcRefCon = this;
    AudioUnitSetProperty(mPlayUnit,
                         kAudioUnitProperty_SetRenderCallback,
                         //kAudioUnitScope_Global,
                         kAudioUnitScope_Input,
                         kOutputBus,
                         &playCallback,
                         sizeof(playCallback));

    // 7.calloc circular buffers
    outrb = create_circular_buffer(bufferByteSize * 4);
    if (NULL == outrb)
      NSLog(@"Player calloc buffer failed!");

    // 8.init AudioUnit
    AudioUnitInitialize(mPlayUnit);
  }

  return status;
}

SInt32 AudioUnitOutput::RemovePlayer() {
  OSStatus status = -1;
  free(playBufList->mBuffers[0].mData);
  free(playBufList);
  status = AudioUnitUninitialize(mPlayUnit);
  AudioComponentInstanceDispose(mPlayUnit);
  if (outrb != NULL) {
    free_circular_buffer(outrb);
    outrb = NULL;
  }
  return status;
}

SInt32 AudioUnitOutput::StartPlay() {
  OSStatus status = -1;
  status = AudioOutputUnitStart(mPlayUnit);
  //NSLog(@"AudioUnit StartPlay(%d)",status);
  return status;
}

SInt32 AudioUnitOutput::PausePlay() {
  OSStatus status = -1;
  return status;
}

SInt32 AudioUnitOutput::StopPlay() {
  OSStatus status = -1;
  status = AudioOutputUnitStop(mPlayUnit);
  //NSLog(@"AudioUnit StopPlay(%d)",status);
  return status;
}

SInt32 AudioUnitOutput::SetVolume(int volume) {
  OSStatus status = -1;
  return status;
}

SInt32 AudioUnitOutput::GetVolume() {
  int volume = 0;
  return volume;
}
