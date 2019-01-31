#ifdef _ANDROID_PLATFORM_
#ifndef ROKID_LOG_ANDROID
#define ROKID_LOG_ANDROID
#endif
#endif

#ifdef SEASON_LOG_ANDROID
#include <android/log.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <chrono>
#endif
#include <string>
#include <memory>
#include <fstream>
#include "audio_log.h"

using std::shared_ptr;
using std::string;

namespace audiomanager {
namespace log {
bool Log::debug_to_file = false;
LogLevel Log::silence_log_level = LogLevel::Verbose;
FILE *Log::debug_fp = nullptr;

Log::Log() {
}

void Log::v(const char* tag, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  instance().p(LogLevel::Verbose, tag, fmt, ap);
  va_end(ap);
}

void Log::d(const char* tag, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  instance().p(LogLevel::Debug, tag, fmt, ap);
  va_end(ap);
}

void Log::i(const char* tag, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  instance().p(LogLevel::Info, tag, fmt, ap);
  va_end(ap);
}

void Log::w(const char* tag, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  instance().p(LogLevel::Warning, tag, fmt, ap);
  va_end(ap);
}

void Log::e(const char* tag, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  instance().p(LogLevel::Error, tag, fmt, ap);
  va_end(ap);
}
#ifdef SEASON_LOG_ANDROID
static uint32_t AndroidLogLevels[] = {
  ANDROID_LOG_VERBOSE,
  ANDROID_LOG_DEBUG,
  ANDROID_LOG_INFO,
  ANDROID_LOG_WARN,
  ANDROID_LOG_ERROR
};
#endif

static char PosixLogLevels[] = {
  'V',
  'D',
  'I',
  'W',
  'E'
};

shared_ptr<string> timestamp_str() {
  char buf[64];
  struct timeval tv;
  struct tm ltm;

  gettimeofday(&tv, NULL);
  localtime_r(&tv.tv_sec, &ltm);
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%ld",
           ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
           ltm.tm_hour, ltm.tm_min, ltm.tm_sec,
           tv.tv_usec);
  shared_ptr<string> res(new string(buf));
  return res;
}

void Log::InitLogSave(std::string save_path) {
  if (save_path.empty()) {
    return;
  }
  debug_fp = fopen(save_path.c_str(), "a+");
}

void Log::Release() {
  if (debug_fp != nullptr) {
    fclose(debug_fp);
    debug_fp = nullptr;
  }
}

void Log::p(LogLevel llevel, const char* tag, const char* fmt, va_list ap) {
  if (llevel < Log::silence_log_level) {
    return;
  }
  int32_t level = static_cast<int32_t>(llevel);
  std::string tag_("IDST::");
  if (tag != nullptr) {
    tag_ += tag;
  }
#ifdef SEASON_LOG_ANDROID
  __android_log_vprint(AndroidLogLevels[level], tag_.c_str(), fmt, ap);
  std::lock_guard<std::mutex> locker(mutex_);
#else  // posix log
  std::lock_guard<std::mutex> locker(mutex_);
  shared_ptr<string> ts = timestamp_str();
  printf("%c %u %s [%s] ",
         PosixLogLevels[level],
         getpid(),
         ts->c_str(),
         tag_.c_str());
  vprintf(fmt, ap);
  printf("\n");
#endif
  if (!debug_to_file || debug_fp == nullptr) {
    return;
  }
  shared_ptr<string> ts2 = timestamp_str();
  fprintf(debug_fp, "%c %s [%s] ",
          PosixLogLevels[level],
          ts2->c_str(),
          tag_.c_str());
  vfprintf(debug_fp, fmt, ap);
  fprintf(debug_fp, "\n");
}
} // namespace log
} // namespace audiomanager
