APP_STL := gnustl_static
APP_ABI := armeabi armeabi-v7a arm64-v8a x86 x86_64
APP_CPPFLAGS := -fexceptions -fpermissive  -std=c++11 -frtti
APP_PLATFORM := android-25
NDK_TOOLCHAIN_VERSION := 4.9
NDK_MODULE_PATH := $(top_src_dir)
