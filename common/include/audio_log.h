/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/common/include/audio_log.h
 */

#ifndef AUDIOMANAGER_COMMON_INCLUDE_AUDIO_LOG_H_
#define AUDIOMANAGER_COMMON_INCLUDE_AUDIO_LOG_H_

#include <stdarg.h>
#include <string>
#include <mutex>

namespace audiomanager {
namespace log {
enum class LogLevel {
  Verbose,
  Debug,
  Info,
  Warning,
  Error,
  MaxLogLevel
};

class Log {
 public:
  static void v(const char* tag, const char* fmt, ...);
  static void d(const char* tag, const char* fmt, ...);
  static void i(const char* tag, const char* fmt, ...);
  static void w(const char* tag, const char* fmt, ...);
  static void e(const char* tag, const char* fmt, ...);
  static LogLevel silence_log_level;
  static bool debug_to_file;
  static void InitLogSave(std::string save_path);
  static void Release();
 private:
  Log();

  static Log& instance() {
    static Log log;
    return log;
  }

  void p(LogLevel level, const char* tag, const char* fmt, va_list ap);

 private:
  std::mutex mutex_;
  static FILE *debug_fp;
};
}
} // namespace audiomanager

#define KLOGV(tag, fmt, ...) audiomanager::log::Log::v(tag, fmt, ##__VA_ARGS__)

#define KLOGD(tag, fmt, ...) audiomanager::log::Log::d(tag, fmt, ##__VA_ARGS__)

#define KLOGI(tag, fmt, ...) audiomanager::log::Log::i(tag, fmt, ##__VA_ARGS__)

#define KLOGW(tag, fmt, ...) audiomanager::log::Log::w(tag, fmt, ##__VA_ARGS__)

#define KLOGE(tag, fmt, ...) audiomanager::log::Log::e(tag, fmt, ##__VA_ARGS__)

#endif // AUDIOMANAGER_COMMON_INCLUDE_AUDIO_LOG_H_

