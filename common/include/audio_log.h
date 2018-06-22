/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/common/include/audio_log.h
 */

#ifndef AUDIOMANAGER_COMMON_INCLUDE_AUDIO_LOG_H_
#define AUDIOMANAGER_COMMON_INCLUDE_AUDIO_LOG_H_

#ifdef _ANDROID_PLATFORM_
#include <android/log.h>
#elif defined _IOS_PLATFORM_
#include <stdio.h>
#endif

namespace audiomanager {

#ifdef _ANDROID_PLATFORM_

#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, "[AudioManager]", __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, "[AudioManager]", __VA_ARGS__))
#define LOGD(...) \
  ((void)__android_log_print(ANDROID_LOG_DEBUG, "[AudioManager]", __VA_ARGS__))

#elif defined _IOS_PLATFORM_

#define LOGI(fmt, ...) \
  fprintf(stdout, fmt"\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) \
  fprintf(stderr, fmt"\n", ##__VA_ARGS__)
#define LOGD(fmt, ...) \
  fprintf(stdout, fmt"\n", ##__VA_ARGS__)
  
#endif


} // namespace audiomanager

#endif // AUDIOMANAGER_COMMON_INCLUDE_AUDIO_LOG_H_

