##############################################################
# Copyright [2018] <season4675@gmail.com>
# Module:   Android.mk
# File:     AudioManager/Android.mk
##############################################################

include $(top_src_dir)/common/src/sources.mk
include $(top_src_dir)/engine/sources.mk
include $(top_src_dir)/engine/audio_openSL/sources.mk

libaudiomanager_cflags = $(common_cflags)
libaudiomanager_ldflags = $(common_ldflags)

include $(CLEAR_VARS)
LOCAL_MODULE := libaudiomanager
LOCAL_C_INCLUDES += $(libaudiomanager_include_dirs)
LOCAL_SRC_FILES := $(libaudiomanager_src_files)
LOCAL_CFLAGS := $(libaudiomanager_cflags)
LOCAL_LDLIBS := $(libaudiomanager_ldflags)
include $(BUILD_SHARED_LIBRARY)
