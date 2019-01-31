//
//  AudioUnitInput.mm
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-4-14.
//  Copyright (c) 2018 season4675. All rights reserved.
//

#define TAG "AudioUnitInput"

#include <fstream>
#include <string>

#include "audio_log.h"
#include "circular_buffer.h"
#include "AudioUnitInput.h"

#define _USE_PLAYER_
#ifdef _USE_PLAYER_
#undef _USE_PLAYER_
#endif

#define _AUDIO_AEC_
//#undef _AUDIO_AEC_

//#define _AUDIO_DEBUG_
#ifdef _AUDIO_DEBUG_
std::string savePath;
std::ofstream audio_save_stream;
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
  OSStatus status = noErr;

  if (inNumberFrames > 0) {
    AudioUnitInput *ars = (AudioUnitInput *)inRefCon;
    AudioBufferList bufferList;
    UInt16 numSamples = inNumberFrames * 1;
    UInt16 samples[numSamples]; // just for 16bit sample
    memset(&samples, 0, sizeof(samples));
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0].mData = samples;
    bufferList.mBuffers[0].mNumberChannels = ars->mRecordFormat.mChannelsPerFrame;
    bufferList.mBuffers[0].mDataByteSize = numSamples * sizeof(UInt16);

    status = AudioUnitRender(ars->mRecordUnit,
                             ioActionFlags,
                             inTimeStamp,
                             inBusNumber,
                             inNumberFrames,
                             &bufferList);
//    if (status != noErr) {
//      NSLog(@"RecordCallback AudioUnitRender failed! status(%d)",
//          status);
//      return noErr;
//    }
    pthread_mutex_lock(&ars->input_mutex);
    count = write_circular_buffer_bytes(
        ars->inrb,
        (char *)bufferList.mBuffers[0].mData,
        bufferList.mBuffers[0].mDataByteSize);
    pthread_mutex_unlock(&ars->input_mutex);
//    NSLog(@"inNumberFrames=%d count=%d %dbytes",
//        inNumberFrames, count, bufferList.mBuffers[0].mDataByteSize);
  } else {
    NSLog(@"inNumberFrames is %d", inNumberFrames);
  }
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
  OSStatus status = noErr;
  UInt32 bufferByteSize = 0;

  if (NULL == mRecordUnit) {
    // 1.init AudioSession
    NSError *error = nil;
    AVAudioSession *audioSession = [AVAudioSession sharedInstance];
    [audioSession setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:AVAudioSessionCategoryOptionDefaultToSpeaker error:&error];
    if (error) NSLog(@"AVAudioSessionCategoryPlayAndRecord failed! error:%@", error);

    [audioSession setPreferredIOBufferDuration:0.08 error:&error];
    if (error) NSLog(@"setPreferredIOBufferDuration failed! error:%@", error);

    // 2.init Buffer
    bufferByteSize = ComputeRecordBufferSize(&mRecordFormat,
                                             kBufferDurationSeconds);
    NSLog(@"Compute recorder %f seconds data buffer size is %d.",
        kBufferDurationSeconds,
        bufferByteSize);

    // 3.init Audio Component
    AudioComponentDescription recorderDesc;
    memset(&recorderDesc, 0, sizeof(AudioComponentDescription));
    recorderDesc.componentType = kAudioUnitType_Output;
#ifdef _AUDIO_AEC_
    recorderDesc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
#else
    recorderDesc.componentSubType = kAudioUnitSubType_RemoteIO;
#endif
    recorderDesc.componentFlags = 0;
    recorderDesc.componentFlagsMask = 0;
    recorderDesc.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent inputComponent = AudioComponentFindNext(NULL, &recorderDesc);
    status = AudioComponentInstanceNew(inputComponent, &mRecordUnit);
    if (status) NSLog(@"AudioComponentInstanceNew failed(%d)", status);

#ifdef _AUDIO_AEC_
    UInt32 echoCancellation = 0; // 0 is open echo cancel
    status = AudioUnitSetProperty(mRecordUnit,
                                  kAUVoiceIOProperty_BypassVoiceProcessing,
                                  kAudioUnitScope_Global,
                                  0,
                                  &echoCancellation,
                                  sizeof(echoCancellation));
    if (status) NSLog(@"kAUVoiceIOProperty_BypassVoiceProcessing set failed(%d)",status);
#endif

    // 4.init Format
    // 对Input Scope的Bus0设置StreamFormat属性
    status = AudioUnitSetProperty(mRecordUnit,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input,
                                  kOutputBus,
                                  &mRecordFormat,
                                  sizeof(mRecordFormat));
    if (status) NSLog(@"kAudioUnitProperty_StreamFormat set input failed(%d)", status);

    // 对Output Scope的Bus1设置StreamFormat属性
    status = AudioUnitSetProperty(mRecordUnit,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Output,
                                  kInputBus,
                                  &mRecordFormat,
                                  sizeof(mRecordFormat));
    if (status) NSLog(@"kAudioUnitProperty_StreamFormat set output failed(%d)", status);

    // 5.init Audio Property
    // mic采集的声音数据从Input Scope的Bus1输入
    UInt32 enableFlag = 1; 
    status = AudioUnitSetProperty(mRecordUnit,
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Input,  // 开启输入
                                  kInputBus,
                                  &enableFlag,
                                  sizeof(enableFlag));
    if (status) NSLog(@"kAudioOutputUnitProperty_EnableIO set input failed(%d)", status);

#ifdef _USE_PLAYER_
    UInt32 playFlag = 1;
#else
    UInt32 playFlag = 0;
#endif
    AudioUnitSetProperty(mRecordUnit,
                         kAudioOutputUnitProperty_EnableIO,
                         kAudioUnitScope_Output,
                         kOutputBus,
                         &playFlag,
                         sizeof(playFlag));

#ifdef _AUDIO_AEC_
    //设置录音的agc增益，这个用的是ios自带agc
    UInt32 enable_agc = 1;
    status = AudioUnitSetProperty(mRecordUnit,
                                  kAUVoiceIOProperty_VoiceProcessingEnableAGC,
                                  kAudioUnitScope_Global,
                                  kInputBus,
                                  &enable_agc,
                                  sizeof(enable_agc));
  if (status) NSLog(@"kAUVoiceIOProperty_VoiceProcessingEnableAGC set failed(%d)", status);
#endif

    // 6.init RecordCallback
    // 在Output Scope的Bus1设置InputCallBack，
    // 在该CallBack中我们需要获取到音频数据
    AURenderCallbackStruct recordCallback;
    recordCallback.inputProc = RecordCallback;
    recordCallback.inputProcRefCon = this;
    status = AudioUnitSetProperty(mRecordUnit,
                                  kAudioOutputUnitProperty_SetInputCallback,
#ifdef _USE_PLAYER_
                                  kAudioUnitScope_Global,
#else
                                  kAudioUnitScope_Output,
#endif
                                  kInputBus,
                                  &recordCallback,
                                  sizeof(recordCallback));

#ifdef _USE_PLAYER_
    // 7.init PlayCallback
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

    // 8.calloc circular buffers
    // Disable buffer allocation for the recorder (optional - do this if we want to pass in our own)
    UInt32 flag = 0;
    status = AudioUnitSetProperty(mRecordUnit,
                                  kAudioUnitProperty_ShouldAllocateBuffer,
                                  kAudioUnitScope_Output,
                                  kInputBus,
                                  &flag,
                                  sizeof(flag));
    if (status) NSLog(@"kAudioUnitProperty_ShouldAllocateBuffer set failed(%d)!", status);

    inrb = create_circular_buffer(bufferByteSize * 8);
    if (NULL == inrb)
      NSLog(@"Recorder calloc buffer failed!");

    // 9.init AudioUnit
    status = AudioUnitInitialize(mRecordUnit);
    if (status) NSLog(@"AudioUnitInitialize mRecordUnit failed(%d)", status);

#ifdef _AUDIO_DEBUG_
    // 10. just for debug
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsPath = [paths objectAtIndex:0];
    NSLog(@"AudioManager debug: %@", documentsPath);
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *testDirectory = [documentsPath stringByAppendingPathComponent:@"voices"];
    // 创建目录
    BOOL res=[fileManager createDirectoryAtPath:testDirectory withIntermediateDirectories:YES attributes:nil error:nil];
    if (res) {
        NSLog(@"文件夹创建成功");
    }else
        NSLog(@"文件夹创建失败");

    savePath = [testDirectory UTF8String];
    audio_save_stream.open(savePath + "/audio_manager_debug.pcm", std::ofstream::binary);
    if (audio_save_stream.is_open()) {
      NSLog(@"audio save for debug is open");
    }
#endif
  }

  return status;
}

SInt32 AudioUnitInput::RemoveRecorder() {
  OSStatus status = -1;
  NSError *error = nil;
  AVAudioSession *audioSession = [AVAudioSession sharedInstance];
  status = AudioOutputUnitStop(mRecordUnit);
  if (status) NSLog(@"AudioUnit StopRecord(%d)", status);
  if (inrb != NULL)
    clean_circular_buffer(inrb);

  status = AudioUnitUninitialize(mRecordUnit);
  status = AudioComponentInstanceDispose(mRecordUnit);
  if (status)
    NSLog(@"RemoveRecorder AudioComponentInstanceDispose failed!(%d)", status);
  if (inrb != NULL) {
    free_circular_buffer(inrb);
    inrb = NULL;
  }

#ifdef _AUDIO_DEBUG_
  if (audio_save_stream.is_open()) {
    audio_save_stream.close();
  }
#endif
  return status;
}

SInt32 AudioUnitInput::StartRecord() {
  OSStatus status = -1;
  NSError *error = nil;
  AVAudioSession *audioSession = [AVAudioSession sharedInstance];
  [audioSession setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:AVAudioSessionCategoryOptionDefaultToSpeaker error:&error];
  if (error) NSLog(@"AVAudioSessionCategoryPlayAndRecord failed! error:%@", error);
  [audioSession setPreferredIOBufferDuration:0.08 error:&error];
  if (error) NSLog(@"setPreferredIOBufferDuration failed! error:%@", error);


#if 1
  // 寻找期望的输入端口
  BOOL isHeadsetMic = false;
  NSArray* inputs = [audioSession availableInputs];
  AVAudioSessionPortDescription *preBuiltInMic = nil;
  for (AVAudioSessionPortDescription* port in inputs) {
    if ([port.portType isEqualToString:AVAudioSessionPortBuiltInMic]) {
      preBuiltInMic = port;
    } else if ([port.portType isEqualToString:AVAudioSessionPortHeadsetMic]) {
      isHeadsetMic = true;
    }
  }
  // 寻找期望的麦克风
  AVAudioSessionPortDescription *builtInMic = nil;
  if (!isHeadsetMic) {
    if (preBuiltInMic != nil)
      builtInMic = preBuiltInMic;
    for (AVAudioSessionDataSourceDescription* descriptions in builtInMic.dataSources) {
      if ([descriptions.orientation isEqual:AVAudioSessionOrientationFront]) {
        [builtInMic setPreferredDataSource:descriptions error:nil];
        [audioSession setPreferredInput:builtInMic error:nil];
        NSLog(@"mic in %@ %@", builtInMic.portType, descriptions.description);
        break;
      }
    }
  } else {
    NSLog(@"mic isHeadsetMic %@", builtInMic.portType);
  }
#endif
#ifdef _AUDIO_AEC_
  // check componentSubType
  // kAudioUnitSubType_RemoteIO -> 'rioc' 0x72696f63 
  // kAudioUnitSubType_VoiceProcessingIO -> 'vpio' 0x7670696f
  AudioComponent currentComponent = AudioComponentInstanceGetComponent(mRecordUnit);
  AudioComponentDescription currentDesc;
  AudioComponentGetDescription(currentComponent, &currentDesc);
  if (currentDesc.componentSubType != 0x7670696f) {
    NSLog(@"StartRecord componentSubType=0x%x", currentDesc.componentSubType);
    currentDesc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;

    AudioComponent inputComponent = AudioComponentFindNext(NULL, &currentDesc);
    AudioComponentInstanceNew(inputComponent, &mRecordUnit);
  }
#endif

  status = AudioOutputUnitStart(mRecordUnit);
  if (status) {
    NSLog(@"AudioOutputUnitStart failed! status(%d)", status);
    // AVAudioSessionCategoryPlay cannot recording, set again
    if (AVAudioSessionCategoryPlayAndRecord != [AVAudioSession sharedInstance].category) {
      NSLog(@"StartRecord Current category: %@", [AVAudioSession sharedInstance].category);
      NSError *error = nil;
      [audioSession setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:AVAudioSessionCategoryOptionMixWithOthers error:&error];
      [audioSession overrideOutputAudioPort:AVAudioSessionPortOverrideSpeaker error:&error];
      //[audioSession setCategory:AVAudioSessionCategoryPlayAndRecord error:&error];
    }
    status = AudioOutputUnitStart(mRecordUnit);
  }

  return status;
}

SInt32 AudioUnitInput::PauseRecord() {
  OSStatus status = -1;
  status = AudioOutputUnitStop(mRecordUnit);
  if (status) NSLog(@"AudioUnit PauseRecord(%d)", status);
  return status;
}

SInt32 AudioUnitInput::StopRecord() {
  OSStatus status = -1;
  status = AudioOutputUnitStop(mRecordUnit);
  if (status) NSLog(@"AudioUnit StopRecord(%d)", status);
  if (inrb != NULL)
    clean_circular_buffer(inrb);
  return status;
}

