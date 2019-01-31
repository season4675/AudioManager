/* Copyright [2018] <season4675@gmail.com>
 * File: AudioManager/common/include/audio_common.h
 */

#ifndef AUDIOMANAGER_COMMON_INCLUDE_AUDIO_COMMON_H_
#define AUDIOMANAGER_COMMON_INCLUDE_AUDIO_COMMON_H_

#include <sys/types.h>
#include <string>
#include <string.h>

#include "stdint.h"

namespace audiomanager {

#ifndef PLAYER_MAX
#define PLAYER_MAX 8
#endif
#ifndef PLAYER_MIN
#define PLAYER_MIN 0
#endif

#ifndef RECORDER_MAX
#define RECORDER_MAX 1
#endif

#ifndef MIN_VOL_LEVEL
#define MIN_VOL_LEVEL 0
#endif
#ifndef MAX_VOL_LEVEL
#define MAX_VOL_LEVEL 100
#endif

typedef uint64_t AMBufferCount;
typedef int32_t  AMResult;
typedef int64_t  off_t;

/*---------------------------------------------------------------------------*/
/* Error Code                                                                */
/*---------------------------------------------------------------------------*/
enum AMError {
  kAMSuccess = 0,
  kAMPreconditionsViolated = 1,
  kAMParameterInvalid = 2,
  kAMMemoryFailure = 3,
  kAMResourceError = 4,
  kAMResourceLost = 5,
  kAMIoError = 6,
  kAMBufferInsufficient = 7,
  kAMContentCorrupted = 8,
  kAMContentUnsupported = 9,
  kAMContentNotFound = 10,
  kAMPermissionDenied = 11,
  kAMFeatureUnsupported = 12,
  kAMInternalError = 13,
  kAMUnknownError = 14,
  kAMOperationAborted = 15,
  kAMControlLost = 16,
  kAMReadonly = 17,
  kAMEngineoptionUnsupported = 18,
  kAMSourceSinkIncompatible = 19,
};

/*---------------------------------------------------------------------------*/
/* Event Code                                                                */
/*---------------------------------------------------------------------------*/
enum AMEvent {
  kAMNone = 0,
  kAMWritePromise = 1,
  kAMWriteWait = 2,
  kAMWriteTooLong = 3,

  kAMPlayerStateChange = 4,
  kAMRecorderStateChange = 5,
};

/*---------------------------------------------------------------------------*/
/* Play/Record Status                                                        */
/*---------------------------------------------------------------------------*/
enum AMAudioState {
  kAMAudioStateNone = 0,
  kAMAudioStateOpened = 1,
  kAMAudioStateStopped = 2,
  kAMAudioStatePaused = 3,
  kAMAudioStateClosed = 4,
  kAMAudioStatePlaying = 5,
  kAMAudioStateRecording = 6,
  kAMAudioStateFinished = 7,
};

/*---------------------------------------------------------------------------*/
/* File Type And Operate Mode                                                */
/*---------------------------------------------------------------------------*/
enum AMFileType {
  kAMFileTypeNONE = 0,
  kAMFileTypePCM = 1,
  kAMFileTypeURI = 2,
  kAMFileTypeASSETS = 3,
  kAMFileTypeWAV = 4,
  kAMFileTypeMP3 = 5,
  kAMFileTypeMAX = 6,
};

enum AMFileMode {
  kAMFileModeOverwrite = 1,
  kAMFileModeAppend = 2,
};

typedef struct AMFileInfo_ {
  int64_t fd;
  const char *file_path;  // or file name
  uint32_t file_type;
  uint32_t file_mode;
  off_t start;
  off_t length;
} AMFileInfo;

/*---------------------------------------------------------------------------*/
/* Audio Data Format                                                         */
/*---------------------------------------------------------------------------*/
typedef struct AMDataFormat_ {
  uint32_t format_type;
  uint32_t num_channels;
  uint32_t sample_rate;
  uint32_t bits_per_sample;
  uint32_t channel_mask;
  uint32_t endianness;
  uint32_t micType;
  uint32_t card;
  uint32_t device;
} AMDataFormat;

enum AMFormatType {
  kAMDataFormatNONE = 0,
  kAMDataFormatMIME = 1,
  kAMDataFormatPCMNonInterleaved = 2,
  kAMDataFormatPCMInterleaved = 3,
  kAMDataFormatRESERVED = 4,
  kAMDataFormatPCMEX = 5,
  kAMDataFormatMAX = 6,
};

enum AMSampleRate {
  kAMSampleRate8K = 1,
  kAMSampleRate11K025 = 2,
  kAMSampleRate12K = 3,
  kAMSampleRate16K = 4,
  kAMSampleRate22K05 = 5,
  kAMSampleRate24K = 6,
  kAMSampleRate32K = 7,
  kAMSampleRate44K1 = 8,
  kAMSampleRate48K = 9,
  kAMSampleRate64K = 10,
  kAMSampleRate88K2 = 11,
  kAMSampleRate96K = 12,
  kAMSampleRate192K = 13,
};

enum AMPCMSampleFormat {
  kAMSampleFormatFixed8 = 1,
  kAMSampleFormatFixed16 = 2,
  kAMSampleFormatFixed20 = 3,
  kAMSampleFormatFixed24 = 4,
  kAMSampleFormatFixed28 = 5,
  kAMSampleFormatFixed32 = 6,
  kAMSampleFormatFixed64 = 7,
};

enum AMPCMByteOrder {
  kAMByteOrderBigEndian = 1,
  kAMByteOrderLittleEndian = 2,
};

/*---------------------------------------------------------------------------*/
/* Audio Channel Mask                                                        */
/*---------------------------------------------------------------------------*/
const uint32_t kAMSpeakerFrontLeft = 0x00000001;
const uint32_t kAMSpeakerFrontRight = 0x00000002;
const uint32_t kAMSpeakerFrontCenter = 0x00000004;
const uint32_t kAMSpeakerLowFrequency = 0x00000008;
const uint32_t kAMSpeakerBackLeft = 0x00000010;
const uint32_t kAMSpeakerBackRight = 0x00000020;
const uint32_t kAMSpeakerFrontLeftOfCenter = 0x00000040;
const uint32_t kAMSpeakerFrontRightOfCenter = 0x00000080;
const uint32_t kAMSpeakerBackCenter = 0x00000100;
const uint32_t kAMSpeakerSideLeft = 0x00000200;
const uint32_t kAMSpeakerSideRight = 0x00000400;
const uint32_t kAMSpeakerTopCenter = 0x00000800;
const uint32_t kAMSpeakerTopFrontLeft = 0x00001000;
const uint32_t kAMSpeakerTopFrontCenter = 0x00002000;
const uint32_t kAMSpeakerTopFrontRight = 0x00004000;
const uint32_t kAMSpeakerTopBackLeft = 0x00008000;
const uint32_t kAMSpeakerTopBackCenter = 0x00010000;
const uint32_t kAMSpeakerTopBackRight = 0x00020000;

/*---------------------------------------------------------------------------*/
/* Audio Status and Paramters                                                */
/*---------------------------------------------------------------------------*/
typedef struct AMAudioStatus_ {
  uint32_t volume;
  uint32_t state;
  bool     mute;              // now just for player
  uint32_t mute_channel_mask; // now just for recorder channel
} AMAudioStatus;

typedef struct AMStatus_ {
  int player_num;
  uint8_t player_channel_mask;
  AMAudioStatus player_status[PLAYER_MAX];

  int recorder_num;
  uint8_t recorder_channel_mask;
  AMAudioStatus recorder_status;
} AMStatus;

/*---------------------------------------------------------------------------*/
/* Audio Manager Versions                                                    */
/*---------------------------------------------------------------------------*/
typedef struct AMAudioVersions_ {
  const char *audio_manager_version;
  const char *audio_engine_version;
} AMAudioVersions;

/*---------------------------------------------------------------------------*/
/* Audio Manager Event */
/*---------------------------------------------------------------------------*/
typedef void (*FuncAudioEventListener) (AMEvent event, int extra_data);

struct AMEventListener {
  AMEventListener() :
    audio_event_callback(nullptr),
    user_data(nullptr) {}
  FuncAudioEventListener audio_event_callback;
  void *user_data;
};

} // namespace audiomanager

#endif // AUDIOMANAGER_COMMON_INCLUDE_AUDIO_COMMON_H_

