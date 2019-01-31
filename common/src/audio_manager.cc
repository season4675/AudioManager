/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/common/src/audio_manager.cc
 */

#define TAG "AudioManager"

#include "audio_manager.h"
#include "audio_manager_impl.h"
#include "audio_log.h"

namespace audiomanager {

static AudioManagerImpl *audio_manager_impl = NULL;

AudioManager *AudioManager::Create() {
  if (audio_manager_impl == NULL) {
    audio_manager_impl = new AudioManagerImpl();
  }
  return audio_manager_impl;
}

AMResult AudioManager::Destroy(AudioManager *am) {
  AMStatus *status = NULL;

  if (NULL == am || NULL == audio_manager_impl) {
    KLOGE(TAG, "AudioManager is inexistent, please create it firtst!");
    return -kAMPreconditionsViolated;
  }
  AudioManagerImpl *am_impl = reinterpret_cast<AudioManagerImpl *>(am);
  status = am_impl->audio_IAudioManager_getStatus();
  if (NULL == status) {
    KLOGE(TAG, "get status is null when destory AudioManager.");
    return -kAMResourceLost;
  }
  if (0 == status->player_num && 0 == status->recorder_num) {
    delete am_impl;
    am_impl = NULL;
    am = NULL;
    audio_manager_impl = NULL;
  } else {
    KLOGE(TAG, "player or recorder has not closed yet, please close it first!");
    return -kAMIoError;
  }

  return kAMSuccess;
}

AudioManager::AudioManager() {}

AudioManager::~AudioManager() {}

} // namespace audiomanager

