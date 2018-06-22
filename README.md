# 目的
音频的播放、录音和控制是和各平台各设备强相关的，这导致很多做跨平台SDK的工程师需要针对每个平台去实现相关接口的学习和调用。这份代码意在隐藏这些音频接口，给上层SDK统一的音频接口。

# AudioManager
现在只简单实现了iOS平台和Android平台及相关测试demo，Linux平台的代码和demo其实也有，晚些更新。

##To build:

###android platform:
  make -f AndroidMakefile

###ios platform:
  make -f iOSMakefile PLATFORM=iPhoneSimulator ARCH=x86_64
  make -f iOSMakefile PLATFORM=iPhoneSimulator ARCH=i386
  make -f iOSMakefile PLATFORM=iPhoneOS ARCH=arm64
  make -f iOSMakefile PLATFORM=iPhoneOS ARCH=armv7
  make -f iOSMakefile PLATFORM=iPhoneOS ARCH=armv7s
  make -f iOSMakefile PLATFORM=MacOSX ARCH=x86_64
  // clean some .o file and save .a file, you should
  make -f iOSMakefile clean
  // clean .o and all .a, you should
  make -f iOSMakefile clean_all
  
  or:
  ./build_iOS.sh

