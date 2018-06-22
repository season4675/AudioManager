/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/common/src/audio_manager.cc
 */

#include "audio_manager.h"
#include "audio_manager_impl.h"
#include "audio_log.h"

namespace audiomanager {

AudioManager *AudioManager::Create() {
  AudioManagerImpl *am_impl = new AudioManagerImpl();
  return am_impl;
}

AMResult AudioManager::Destroy(AudioManager *am) {
  AMStatus *status = NULL;

  if (NULL == am) {
    LOGE("AudioManager is inexistent, please create it firtst!");
    return -kAMPreconditionsViolated;
  }
  AudioManagerImpl *am_impl = reinterpret_cast<AudioManagerImpl *>(am);
  status = am_impl->audio_IAudioManager_getStatus();
  if (NULL == status) {
    LOGE("get status is null when destory AudioManager.");
    return -kAMResourceLost;
  }
  if (0 == status->player_num && 0 == status->recorder_num) {
    delete am_impl;
    am_impl = NULL;
    am = NULL;
  } else {
    return -kAMIoError;
  }

  return kAMSuccess;
}

AudioManager::AudioManager() {}

AudioManager::~AudioManager() {}

} // namespace audiomanager

