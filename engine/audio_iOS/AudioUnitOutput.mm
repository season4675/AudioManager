//
//  AudioUnitOutput.mm
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-4-14.
//  Copyright (c) 2018 season4675. All rights reserved.
//

#define TAG "AudioUnitOutput"

#include "audio_log.h"
#include "audio_common.h"
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
    const char *err_name = [e.name UTF8String];
    const char *err_reason = [e.reason UTF8String];
    KLOGE(TAG, "Error: %s (%s)", err_name, err_reason);
    return 0;
  }  
  return bytes;
}

//______________________________________________________________________________
// AudioUnit callback function.
static int empty_count = 0;
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
  if (ars->mIsPaused) {
    return noErr;
  }
  pthread_mutex_lock(&ars->output_mutex);

  if (ars->play_type == audiomanager::kAMFileTypePCM && ars->fd) {
    bytecount = fread((char *)ioData->mBuffers[0].mData,
                      1,
                      ioData->mBuffers[0].mDataByteSize,
                      ars->fd);
    if (bytecount <= 0) {
      pthread_mutex_unlock(&ars->output_mutex);
      return noErr;
    }
  } else {
    bytecount = read_circular_buffer_bytes(ars->outrb,
                                           (char *)ioData->mBuffers[0].mData,
                                           ioData->mBuffers[0].mDataByteSize);
  }
  ioData->mBuffers[0].mDataByteSize = bytecount;
  if (0 == bytecount) {
    KLOGW(TAG, "there is no data in the buffer");
    if (empty_count++ > 2 && ars->audio_listener_.audio_event_callback != NULL) {
      ars->audio_listener_.audio_event_callback(
          audiomanager::kAMPlayerStateChange,
          audiomanager::kAMAudioStateFinished);
    }
    ars->is_drained = true;
  } else {
    empty_count = 0;
  }
  pthread_mutex_unlock(&ars->output_mutex);
  //KLOGV(TAG, "playcallback bytecount=%d expect %d", bytecount, ioData->mBuffers[0].mDataByteSize);

  return noErr;
}

AudioUnitOutput::AudioUnitOutput() {
  mPlayUnit = NULL;
  audio_listener_.audio_event_callback = NULL;
  audio_listener_.user_data = NULL;
}

AudioUnitOutput::~AudioUnitOutput() {
  mPlayUnit = NULL;
}

void AudioUnitOutput::AudioUnitRegisterListener(const audiomanager::AMEventListener &listener) {
  audio_listener_.audio_event_callback = listener.audio_event_callback;
  audio_listener_.user_data = listener.user_data;
}

SInt32 AudioUnitOutput::InitPlayer() {
  OSStatus status = -1;
  UInt32 bufferByteSize = 0;
  mIsPaused = false;
  if (NULL == mPlayUnit) {
    // 1.init AudioSession
    NSError *error = nil;
    AVAudioSession *audioSession = [AVAudioSession sharedInstance];
    [audioSession setCategory:AVAudioSessionCategoryPlayback withOptions:AVAudioSessionCategoryOptionDefaultToSpeaker error:&error];
    [audioSession setPreferredIOBufferDuration:0.08 error:&error];
    //KLOGV(TAG, "setPreferredIOBufferDuration 0.08!");
    if (error) {
      //NSLog(@"setPreferredIOBufferDuration failed! error:%@", error);
      KLOGE(TAG, "setPreferredIOBufferDuration failed! error:%s", error.userInfo);
    }
    [audioSession setActive:YES error:&error];
    if (error) {
      //NSLog(@"setActive YES failed! error:%@", error);
      KLOGE(TAG, "setActive YES failed! error:%s", error.userInfo);
    }

    // 2.init Audio Component
    AudioComponentDescription playerDesc;
    playerDesc.componentType = kAudioUnitType_Output;
    // 如果你的应用程序需要去除回声将componentSubType
    // 设置为kAudioUnitSubType_VoiceProcessingIO
    //playerDesc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
    playerDesc.componentSubType = kAudioUnitSubType_RemoteIO;
    playerDesc.componentFlags = 0;
    playerDesc.componentFlagsMask = 0;
    playerDesc.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent outputComponent = AudioComponentFindNext(NULL, &playerDesc);
    status = AudioComponentInstanceNew(outputComponent, &mPlayUnit);
    if (status) KLOGE(TAG, "AudioComponentInstanceNew failed(%d)", status);

    // 3.init Buffer
    bufSizeDurationOneSec = ComputePlayBufferSize(&mPlayFormat, 1);
    bufferByteSize = ComputePlayBufferSize(&mPlayFormat,
                                           kBufferDurationSeconds);
    KLOGI(TAG, "Compute player %f seconds data buffer size is %d.",
        kBufferDurationSeconds,
        bufferByteSize);
    playBufList = (AudioBufferList *)malloc(sizeof(AudioBufferList));
    playBufList->mNumberBuffers = 1;
    playBufList->mBuffers[0].mNumberChannels = mPlayFormat.mChannelsPerFrame;
    playBufList->mBuffers[0].mDataByteSize = bufferByteSize;
    playBufList->mBuffers[0].mData = malloc(bufferByteSize);


    // 4.init Audio Property
    // 音频从Output Scope的Bus0输出
    UInt32 playFlag = 1;
    status = AudioUnitSetProperty(mPlayUnit,
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Output,
                                  kOutputBus,
                                  &playFlag,
                                  sizeof(playFlag));
    if (status) {
        KLOGE(TAG, "AudioUnitSetProperty error with status:%d", status);
    }

    // 5.init Format
    // mFormatFlags错了会导致无法播放
    // 对Input Scope的Bus0设置StreamFormat属性
    mPlayFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger; // 整型
    status = AudioUnitSetProperty(mPlayUnit,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input,
                                  kOutputBus,
                                  &mPlayFormat,
                                  sizeof(mPlayFormat));
    if (status)
      KLOGE(TAG, "kAudioUnitProperty_StreamFormat set failed(%d)", status);

    // 6.init PlayCallback
    // 在Input Scope的Bus0设置OutputCallBack
    AURenderCallbackStruct playCallback;
    playCallback.inputProc = PlayCallback;
    playCallback.inputProcRefCon = this;
    status = AudioUnitSetProperty(mPlayUnit,
                                  kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Input,
                                  kOutputBus,
                                  &playCallback,
                                  sizeof(playCallback));
    if (status) KLOGE(TAG, "AudioUnit init PlayCallback failed(%d)!", status);

    // 7.calloc circular buffers
    outrb = create_circular_buffer(bufferByteSize * 4);
    if (NULL == outrb)
      KLOGE(TAG, "Player calloc buffer failed!");

    // 8.init AudioUnit
    status = AudioUnitInitialize(mPlayUnit);
    if (status) KLOGE(TAG, "AudioUnitInitialize mPlayUnit failed(%d)", status);
  }

  // audio file
  if (play_type == audiomanager::kAMFileTypePCM) {
    if (locator != NULL) {
      fd = fopen(locator, "r");
      if (fd) {
        fseek(fd, 0, SEEK_SET);
      } else {
        KLOGE(TAG, "audio file open failed!");
      }
    } else {
      KLOGE(TAG, "file path is null!");
    }
  }

  return status;
}

SInt32 AudioUnitOutput::RemovePlayer() {
  OSStatus status = -1;
  NSError *error = nil;
  AVAudioSession *audioSession = [AVAudioSession sharedInstance];
  status = AudioOutputUnitStop(mPlayUnit);
  if (status) KLOGE(TAG, "AudioUnit PausePlay(%d)", status);
  clean_circular_buffer(outrb);

  mIsPaused = false;
  free(playBufList->mBuffers[0].mData);
  free(playBufList);
  if (mPlayUnit != NULL) {
    status = AudioUnitUninitialize(mPlayUnit);
    if (status)
      KLOGE(TAG, "AudioUnitUninitialize failed! status(%d)", status);
    status = AudioComponentInstanceDispose(mPlayUnit);
    if (status)
      KLOGE(TAG, "RemovePlayer AudioComponentInstanceDispose failed!(%d)", status);
    else
      mPlayUnit = NULL;
  }

  if (outrb != NULL) {
    free_circular_buffer(outrb);
    outrb = NULL;
  }

  // audio file
  if (play_type == audiomanager::kAMFileTypePCM) {
    if (locator != NULL) {
      status = fclose(fd);
      if (status) {
        KLOGE(TAG, "audio file close failed!(%d)", status);
      }
    } else {
      KLOGE(TAG, "file path is null!");
    }
  }

  return status;
}

SInt32 AudioUnitOutput::StartPlay() {
  OSStatus status = -1;
  is_drained = false;
  KLOGV(TAG, "StartPlay");
  if (mPlayUnit != NULL) {
    //if on paused state, we dont have to call start
    if (mIsPaused) {
      mIsPaused = false;
      status = 0;
    } else {
      NSError *error = nil;
      AVAudioSession *audioSession = [AVAudioSession sharedInstance];
      [audioSession setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:AVAudioSessionCategoryOptionDefaultToSpeaker error:&error];
      if (error) KLOGE(TAG, "AVAudioSessionCategoryPlayAndRecord failed! error:%s", error.userInfo);
      [audioSession setPreferredIOBufferDuration:0.08 error:&error];
      if (error) KLOGE(TAG, "setPreferredIOBufferDuration failed! error:%s", error.userInfo);
      clean_circular_buffer(outrb);
      status = AudioOutputUnitStart(mPlayUnit);
      if (status) KLOGE(TAG, "AudioUnit StartPlay failed! status(%d)", status);
    }
  } else {
    KLOGE(TAG, "StartPlay mPlayUnit is null!");
  }
  return status;
}

SInt32 AudioUnitOutput::PausePlay() {
  //FIXME: AudioUnitOutput do not support pause yet, 
  //so we do it by flag.
  KLOGV(TAG, "PausePlay enter");
  mIsPaused = true;
  return 0;
#if 0
  OSStatus status = -1;
  if (mPlayUnit != NULL) {
    status = AudioOutputUnitStop(mPlayUnit);
    if (status) KLOGE(TAG, "AudioUnit PausePlay(%d)", status);
  } else {
    KLOGE(TAG, "PausePlay mPlayUnit is null!");
  }
  return status;
#endif
}

SInt32 AudioUnitOutput::StopPlay(bool drain) {
  OSStatus status = -1;
  mIsPaused = false;
  if (mPlayUnit != NULL) {
    KLOGV(TAG, "stop play drain %d", drain);
    if (drain) {
      int cnt = 0;
      int cir_buf_max_time_ms = kBufferDurationSeconds * 1000 * 4;
      int sleep_time_ms = 10;
      while (!is_drained) {
        usleep(sleep_time_ms * 1000);
        if (cnt ++ > cir_buf_max_time_ms/sleep_time_ms) {
          KLOGW(TAG, "Buffer should have drained, should not be here.");
          break;
        }
      }
    }
    status = AudioOutputUnitStop(mPlayUnit);
    if (status) {
      KLOGE(TAG, "AudioUnit StopPlay(%d)", status);
    } else {
      audio_listener_.audio_event_callback(
          audiomanager::kAMPlayerStateChange,
          audiomanager::kAMAudioStateFinished);
    }
  } else {
    KLOGE(TAG, "StopPlay mPlayUnit is null!");
  }
  // audio file
  if (play_type == audiomanager::kAMFileTypePCM) {
    if (fd) {
      fseek(fd, 0, SEEK_SET);
    } else {
      KLOGE(TAG, "audio file fseek failed!");
    }
  }
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
