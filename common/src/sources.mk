##############################################################
# Copyright [2018] <season4675@gmail.com>
# Module:   sources.mk
# File:     AudioManager/common/src/sources.mk
##############################################################

libaudiomanager_common_dir := $(top_src_dir)/common

libaudiomanager_include_dirs := $(libaudiomanager_common_dir)/src \
	$(libaudiomanager_common_dir)/include

libaudiomanager_src_files := $(libaudiomanager_common_dir)/src/audio_manager.cc \
	$(libaudiomanager_common_dir)/src/audio_manager_impl.cc \
	$(libaudiomanager_common_dir)/src/audio_log.cc

