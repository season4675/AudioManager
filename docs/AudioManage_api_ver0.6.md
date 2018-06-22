#1. AudioManager接口
所有接口列表：

`AudioManager *Create();`
`AMResult Destroy(AudioManager *audiomanager);`
`AMAudioVersions *audio_IAudioManager_getVersions();`

`AMResult audio_IAudioOutput_open(AMDataFormat *data_format);`
`AMResult audio_IAudioOutput_close(int player_id);`
`AMBufferCount audio_IAudioOutput_write(void *src_buffer, const int buffer_size, int player_id);`
`AMBufferCount audio_IAudioOutput_getBufferCount(int player_id);`
`AMResult audio_IAudioOutput_pause(int player_id);`
`AMResult audio_IAudioOutput_stop(int player_id);`
`AMResult audio_IAudioOutput_getAudioFormat(AMDataFormat *data_format, int player_id);`
`AMResult audio_IAudioOutput_getAudioStatus(AMAudioStatus *audio_status, int player_id);`
`AMResult audio_IAudioOutput_setAudioStatus(AMAudioStatus *audio_status, int player_id);`

`AMResult audio_IAudioOutputFromFile_open(AMFileInfo *file_info, AMDataFormat *data_format);`
`AMResult audio_IAudioOutputFromFile_close(int player_id);`
`AMResult audio_IAudioOutputFromFile_start(int player_id);`
`AMResult audio_IAudioOutputFromFile_pause(int player_id);`
`AMResult audio_IAudioOutputFromFile_stop(int player_id);`
`AMResult audio_IAudioOutputFromFile_getAudioFormat(AMDataFormat *data_format, int player_id);`
`AMResult audio_IAudioOutputFromFile_getAudioStatus(AMAudioStatus *audio_status, int player_id);`
`AMResult audio_IAudioOutputFromFile_setAudioStatus(AMAudioStatus *audio_status, int player_id);`

`AMResult audio_IAudioInput_open(AMDataFormat *data_format);`
`AMResult audio_IAudioInput_close();`
`AMBufferCount audio_IAudioInput_read(void *read_buffer, const int buffer_size);`
`AMResult audio_IAudioInput_pause();`
`AMResult audio_IAudioInput_stop();`
`AMResult audio_IAudioInput_getAudioFormat(AMDataFormat *data_format);`
`AMResult audio_IAudioInput_getAudioStatus(AMAudioStatus *audio_status);`
`AMResult audio_IAudioInput_setAudioStatus(AMAudioStatus *audio_status);`

`AMResult audio_IAudioInputToFile_open(AMFileInfo *file_info, AMDataFormat *data_format);`
`AMResult audio_IAudioInputToFile_close();`
`AMResult audio_IAudioInputToFile_start();`
`AMResult audio_IAudioInputToFile_pause();`
`AMResult audio_IAudioInputToFile_stop();`
`AMResult audio_IAudioInputToFile_getAudioFormat(AMDataFormat *data_format);`
`AMResult audio_IAudioInputToFile_getAudioStatus(AMAudioStatus *audio_status);`
`AMResult audio_IAudioInputToFile_setAudioStatus(AMAudioStatus *audio_status);`

- **创建AudioManager模块并初始化**
-- 函数描述：AudioManager的初始化，主要是平台的检测。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AudioManager *Create();`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in,out] NULL
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AudioManager

- **反初始化AudioManager模块**
-- 函数描述：AudioManager的反初始化。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult Destroy(AudioManager *audiomanager);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in] audiomanager: 需要释放的AudioManager 
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码

- **获取AudioManager模块的版本信息**
-- 函数描述：获取AudioManager模块的版本信息。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMAudioVersions * audio_AudioManager_getVersions();`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in,out] NULL
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMAudioVersions，内含AudioManager和AudioEngine的版本信息。

- **打开AudioOutput设备**
-- 函数描述：对AudioOutput进行初始化，生效各参数，如采样率、输入通道、输出通道、bufferframes等等。audiomanager获得engine的各操作接口。例如需要输出一个44100HZ单通道的PCM音频数据，则在此函数传入详细数据参数即可实现。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioOutput_open(AMDataFormat *data_format);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in] data_format：输出音频的参数，包括数据格式、通道数、采样率、位深等
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 当>=0时，为打开player的编号，现支持0~7 8个Player。
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 当<0时，为错误码

- **关闭AudioOutput设备**
-- 函数描述：对AudioOutput进行反初始化，释放资源。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioOutput_close(int player_id);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in] player_id：需要操作的player编号
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码
  
- **将Buffer写入AudioOutput设备中**
-- 函数描述：将buffer推送入AudioOutput进行播放，即播放pcm格式。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMBufferCount audio_AudioOutput_write(void *src_buffer, int buffer_size, int player_id);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in] src_buffer：pcm音频数据
  &nbsp; &nbsp; &nbsp; &nbsp; [in] buffer_size：一次写入音频数据的字节数
  &nbsp; &nbsp; &nbsp; &nbsp; [in] player_id：需要操作的player编号
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMBufferCount: 写入的字节数

- **获取写入AudioOutput设备中的Buffer数**
-- 函数描述：获取AudioOutput设备open()以来写入的buffer数。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMBufferCount audio_AudioOutput_getBufferCount(int player_id);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in] player_id：需要操作的player编号
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMBufferCount 写入AudioOutput设备的字节数

- **获取AudioOutput音频格式**
-- 函数描述：获取AudioOutput最终输出的音频参数，包括数据格式、通道数、采样率、位深等。注意，open()操作中代入了src和sink两个音频格式信息，这里获取的是AudioManager内部engine最终输出的音频参数。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioOutput_getAudioFormat(AMDataFormat * data_format, int player_id);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [out] data_format：音频的参数，包括数据格式、通道数、采样率、位深等
  &nbsp; &nbsp; &nbsp; &nbsp; [in] player_id：需要操作的player编号
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码

- **获取AudioOutput音频状态参数**
-- 函数描述：获取AudioOutput各参数，包括音量、静音、暂停、循环等。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioOutput_getAudioStatus(AMAudioStatus *audio_status, int player_id);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [out] audio_status：音频状态参数，包括音量、静音、暂停、循环等
  &nbsp; &nbsp; &nbsp; &nbsp; [in] player_id：需要操作的player编号
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码

- **设置AudioOutput音频状态**
-- 函数描述：设置AudioOutput各参数，包括音量、静音、暂停、循环等。暂停或停止或播放可以在此置状态，其中播放可以直接调用write()改变状态。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioOutput_setAudioStatus(AMAudioStatus *audio_status, int player_id);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in] audio_status：音频状态参数，包括音量、静音、暂停、循环等
  &nbsp; &nbsp; &nbsp; &nbsp; [in] player_id：需要操作的player编号
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码
  
- **打开AudioOutput用于播放音频文件**
-- 函数描述：打开AudioOutput用播放pcm、URI、Assets文件。由setAudioStatus函数控制播放、暂停和停止。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioOutputFromFile_open(AMFileInfo *file_info, AMDataFormat *data_format);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in] file_info：包含文件类型、路径信息、播放器类型等
  &nbsp; &nbsp; &nbsp; &nbsp; [in] data_format：音频的参数，包括数据格式、通道数、采样率、位深等
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 当>=0时，为打开player的编号，现支持0~7 8个Player。
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 当<0时，为错误码
 
- **关闭用于播放音频文件的AudioOutput**
-- 函数描述：关闭用于播放pcm、URI、Assets文件的AudioOutput。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioOutputFromFile_close(int player_id);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in] player_id：需要操作的player编号
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码

- **获取播放音频文件的AudioOutput的音频格式**
-- 函数描述：获取AudioOutput的音频参数，包括数据格式、通道数、采样率、位深等。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioOutputFromFile_getAudioFormat(AMDataFormat * data_format, int player_id);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [out] data_format：音频的参数，包括数据格式、通道数、采样率、位深等
  &nbsp; &nbsp; &nbsp; &nbsp; [in] player_id：需要操作的player编号
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码

- **获取播放音频文件的AudioOutput的音频状态参数**
-- 函数描述：获取AudioOutput各参数，包括音量、静音、暂停、循环等。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioOutputFromFile_getAudioStatus(AMAudioStatus *audio_status, int player_id);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [out] audio_status：音频状态参数，包括音量、静音、暂停、循环等
  &nbsp; &nbsp; &nbsp; &nbsp; [in] player_id：需要操作的player编号
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码
  
- **设置播放音频文件的AudioOutput的音频状态**
-- 函数描述：设置AudioOutput各参数，包括音量、静音、暂停、循环等。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioOutputFromFile_setAudioStatus(AMAudioStatus *audio_status, int player_id);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in] audio_status：音频状态参数，包括音量、静音、暂停、循环等
  &nbsp; &nbsp; &nbsp; &nbsp; [in] player_id：需要操作的player编号
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码
  
- **打开AudioInput设备**
-- 函数描述：对AudioInput进行初始化，生效各参数，如采样率、输入通道、输出通道、bufferframes等等。audiomanager获得engine的各操作接口。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioInput_open(AMDataFormat *data_format);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in] data_format：音频的数据参数，包括数据格式、通道数、采样率、位深等
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码

- **关闭AudioInput设备**
-- 函数描述：对AudioInput进行反初始化，释放资源。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioInput_close();`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in,out] NULL
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码 

- **从AudioInput读取Buffer**
-- 函数描述：从AudioInput读取音频数据，即pcm格式。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMBufferCount audio_AudioInput_read(void * read_buffer, int buffer_size);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [out] read_buffer：pcm音频数据
  &nbsp; &nbsp; &nbsp; &nbsp; [in] buffer_size：一次读出音频数据的字节数
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMBufferCount: 读取的字节数

- **获取AudioInput音频格式**
-- 函数描述：获取AudioInput的音频参数，包括数据格式、通道数、采样率、位深等。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioInput_getAudioFormat(AMDataFormat * data_format);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [out] data_format：音频的参数，包括数据格式、通道数、采样率、位深等
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码

- **获取AudioInput音频状态参数**
-- 函数描述：获取AudioInput各参数，包括音量、静音等。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioInput_getAudioStatus(AMAudioStatus *audio_status);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [out] audio_status：音频状态参数，包括音量、静音、暂停等
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码

- **设置AudioInput音频状态**
-- 函数描述：设置AudioInput各参数，包括音量、静音等。暂停或停止或录音可以在此置状态，其中录音可以直接调用read()改变状态。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioInput_setAudioStatus(AMAudioStatus *audio_status);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in] audio_status：音频状态参数，包括音量、静音、暂停等
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码
  
- **打开AudioInput用于录制音频文件**
-- 函数描述：打开AudioInput用于接收音频数据到文件。由setAudioStatus函数控制录音、暂停和停止。
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioInputToFile_open(AMFileInfo *file_info, AMDataFormat *data_format);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in] file_info：包含文件类型、路径信息等
  &nbsp; &nbsp; &nbsp; &nbsp; [in] data_format：音频的参数，包括数据格式、通道数、采样率、位深等
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码

- **关闭用于录制音频文件的AudioInput**
-- 函数描述：关闭用于录制音频文件的AudioInput
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioInputToFile_close();`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in,out] NULL
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码
  
- **获取录制音频文件的AudioInput的音频格式**
-- 函数描述：获取AudioInput的音频参数，包括数据格式、通道数、采样率、位深等
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioInputToFile_getAudioFormat(AMDataFormat * data_format);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [out] data_format：音频的参数，包括数据格式、通道数、采样率、位深等
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码

- **获取录制音频文件的AudioInput的音频状态参数**
-- 函数描述：获取AudioInput各参数，包括音量、静音、暂停等
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioInputToFile_getAudioStatus(AMAudioStatus *audio_status);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [out] audio_status：音频状态参数，包括音量、静音、暂停、循环等
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码
  
- **设置录制音频文件的AudioInput的音频状态**
-- 函数描述：设置AudioInput各参数，包括音量、静音、暂停、循环等
-- 函数原型：
  &nbsp; &nbsp; &nbsp; &nbsp; `AMResult audio_AudioInputToFile_setAudioStatus(AMAudioStatus *audio_status);`
  -- 参数：
  &nbsp; &nbsp; &nbsp; &nbsp; [in] audio_status：音频状态参数，包括音量、静音、暂停、循环等
  -- 返回值:
   &nbsp; &nbsp; &nbsp; &nbsp; AMResult 错误码
  
#3. 错误码
`enum AMResult {`
&nbsp; `kAMSuccess = 0,`

&nbsp; `kAMPreconditionsViolated = 1,`
&nbsp; `kAMParameterInvalid = 2,`
&nbsp; `kAMMemoryFailure = 3,`
&nbsp; `kAMResourceError = 4,`
&nbsp; `kAMResourceLost = 5,`
&nbsp; `kAMIoError = 6,`
&nbsp; `kAMBufferInsufficient = 7,`
&nbsp; `kAMContentCorrupted = 8,`
&nbsp; `kAMContentUnsupported = 9,`
&nbsp; `kAMContentNotFound = 10,`
&nbsp; `kAMPermissionDenied = 11,`
&nbsp; `kAMFeatureUnsupported = 12,`
&nbsp; `kAMInternalError = 13,`
&nbsp; `kAMUnknownError = 14,`
&nbsp; `kAMOperationAborted = 15,`
&nbsp; `kAMControlLost = 16,`
&nbsp; `kAMReadonly = 17,`
&nbsp; `kAMEngineoptionUnsupported = 18,`
&nbsp; `kAMSourceSinkIncompatible = 19,`
`};`

#4. 关键结构体
`typedef struct AMFileInfo_ {`
&nbsp; `int64_t fd;    /*文件描述符*/`
&nbsp; `const char *file_path;    /*文件路径*/`
&nbsp; `uint32_t file_type;    /*文件类型，PCM、URI等*/`
&nbsp; `uint32_t file_mode;    /*文件模式，即读写覆盖*/`
&nbsp; `off_t start;    /*文件起始偏移*/`
&nbsp; `off_t length;    /*文件偏移*/`
`} AMFileInfo;`

`typedef struct AMDataFormat_ {`
&nbsp; `uint32 format_type;    /*数据格式，默认DATAFORMAT_PCM*/`
&nbsp; `uint32 num_channels;    /*通道数，android平台默认输入mono，输出stereo*/`
&nbsp; `uint32 sample_rate;    /*采样率，默认48000*/`
&nbsp; `uint32 bits_per_sample;    /*位深，默认PCMSAMPLEFORMAT_FIXED_16*/`
&nbsp; `uint32 channel_mask;    /*通道选择，默认SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT*/`
&nbsp; `uint32 endianness;    /*字节顺序，默认BYTEORDER_LITTLEENDIAN*/`
`} AMDataFormat;`

`typedef struct AMAudioStatus_ {`
&nbsp; `uint32 volume;    /*音量*/`
&nbsp; `uint32 state;    /*playing、recording、paused、stopped*/`
&nbsp; `uint32 mute_channel_mask    /*禁音通道掩码*/`
`} AMAudioStatus;`

`typedef struct AMAudioVersions_ {`
&nbsp; `const char *audio_manager_version;    /*Audio Manager版本号*/`
&nbsp; `const char *audio_engine_version;    /*Audio Engine的版本号，比如openSLES版本信息*/`
`} AMAudioVersions;`

#6. 宏定义/状态码
`typedef uint64_t AMBufferCount;`
`typedef int32_t  AMResult;`
`typedef long     off_t;`

- 文件类型(fileType)
`enum AMFileType {`
&nbsp; `kAMFileTypeNONE = 0,`
&nbsp; `kAMFileTypePCM = 1,`
&nbsp; `kAMFileTypeURI = 2,`
&nbsp; `kAMFileTypeASSETS = 3,`
&nbsp; `kAMFileTypeMAX = 4,`
`};`

- 文件模式(fileMode)
`enum AMFileMode {`
&nbsp; `kAMFileModeOverwrite = 1,`
&nbsp; `kAMFileModeAppend = 2,`
`};`

- 播放/录音状态（state）
`enum AMAudioState {`
&nbsp; `kAMAudioStateStopped = 1,`
&nbsp; `kAMAudioStatePaused = 2,`
&nbsp; `kAMAudioStatePlaying = 3,`
&nbsp; `kAMAudioStateRecording = 4,`
`};`

- 音频格式类型(formatType)
`enum AMFormatType {`
&nbsp; `kAMDataFormatNONE = 0,`
&nbsp; `kAMDataFormatMIME = 1,`
&nbsp; `kAMDataFormatPCMNonInterleaved = 2,`
&nbsp; `kAMDataFormatPCMInterleaved = 3,`
&nbsp; `kAMDataFormatRESERVED = 4,`
&nbsp; `kAMDataFormatPCMEX = 5,`
&nbsp; `kAMDataFormatMAX = 6,`
`};`

- 采样率(samplesPerSec)
`enum AMSampleRate {`
&nbsp; `kAMSampleRate8K = 1,`
&nbsp; `kAMSampleRate11K025 = 2,`
&nbsp; `kAMSampleRate12K = 3,`
&nbsp; `kAMSampleRate16K = 4,`
&nbsp; `kAMSampleRate22K05 = 5,`
&nbsp; `kAMSampleRate24K = 6,`
&nbsp; `kAMSampleRate32K = 7,`
&nbsp; `kAMSampleRate44K1 = 8,`
&nbsp; `kAMSampleRate48K = 9,`
&nbsp; `kAMSampleRate64K = 10,`
&nbsp; `kAMSampleRate88K2 = 11,`
&nbsp; `kAMSampleRate96K = 12,`
&nbsp; `kAMSampleRate192K = 13,`
`};`

- 数据位深(bitsPerSample)
`enum AMPCMSampleFormat {`
&nbsp; `kAMPCMSampleFormatFixed8 = 1,`
&nbsp; `kAMPCMSampleFormatFixed16 = 2,`
&nbsp; `kAMPCMSampleFormatFixed20 = 3,`
&nbsp; `kAMPCMSampleFormatFixed24 = 4,`
&nbsp; `kAMPCMSampleFormatFixed28 = 5,`
&nbsp; `kAMPCMSampleFormatFixed32 = 6,`
&nbsp; `kAMPCMSampleFormatFixed64 = 7,`
`};`

- 通道选择(channelMask)
`const uint32 kAMSpeakerFrontLeft = 0x00000001;`
`const uint32 kAMSpeakerFrontRight = 0x00000002;`
`const uint32 kAMSpeakerFrontCenter = 0x00000004;`
`const uint32 kAMSpeakerLowFrequency = 0x00000008;`
`const uint32 kAMSpeakerBackLeft = 0x00000010;`
`const uint32 kAMSpeakerBackRight = 0x00000020;`
`const uint32 kAMSpeakerFrontLeftOfCenter = 0x00000040;`
`const uint32 kAMSpeakerFrontRightOfCenter = 0x00000080;`
`const uint32 kAMSpeakerBackCenter = 0x00000100;`
`const uint32 kAMSpeakerSideLeft = 0x00000200;`
`const uint32 kAMSpeakerSideRight = 0x00000400;`
`const uint32 kAMSpeakerTopCenter = 0x00000800;`
`const uint32 kAMSpeakerTopFrontLeft = 0x00001000;`
`const uint32 kAMSpeakerTopFrontCenter = 0x00002000;`
`const uint32 kAMSpeakerTopFrontRight = 0x00004000;`
`const uint32 kAMSpeakerTopBackLeft = 0x00008000;`
`const uint32 kAMSpeakerTopBackCenter = 0x00010000;`
`const uint32 kAMSpeakerTopBackRight = 0x00020000;`

- 字节顺序(endianness)
`enum AMPCMByteOrder {`
&nbsp; `kAMByteOrderBigEndian = 1,`
&nbsp; `kAMByteOrderLittleEndian = 2,`
`};`

#2. AudioManager于各平台
- iOS平台
iOS平台代码上使用AudioQueue接口，但是考虑到AudioQueue的实时性较差的问题，后面我又改成了AudioUnit，当然也还是保存了原先的AudioQueue的代码。

- Android平台
Android平台AudioManager设计使用OpenSL ES来实现native层高效音频功能。
openSL ES特性：
（1）C 语言接口，兼容 C++，需要在 NDK 下开发，能更好地集成在 native 应用中
（2）运行于 native 层，需要自己管理资源的申请与释放，没有 Dalvik 虚拟机的垃圾回收机制
（3）支持 PCM 数据的采集，支持的配置：16bit 位宽，16000 Hz采样率，单通道。（其他的配置不能保证兼容所有平台）
（4）支持 PCM 数据的播放，支持的配置：8bit/16bit 位宽，单通道/双通道，小端模式，采样率（8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 Hz）
（5）支持播放的音频数据来源：res 文件夹下的音频、assets 文件夹下的音频、sdcard 目录下的音频、在线网络音频、代码中定义的音频二进制数据等等

  openSL ES不支持：
（1）不支持版本低于 Android 2.3 (API 9) 的设备
（2）没有全部实现 OpenSL ES 定义的特性和功能
（3）不支持 MIDI 
（4）不支持直接播放 DRM 或者 加密的内容
（5）不支持音频数据的编解码，如需编解码，需要使用 MediaCodec API 或者第三方库
（6）在音频延时方面，相比于上层 API，并没有特别明显地改进

  openSL ES位于Android系统的层次：
  ![opensles_framework.jpg](file:./opensles_framework.jpg)
  
- Linux平台
Linux平台AudioManager设计使用pulseaudio来实现音频功能。
