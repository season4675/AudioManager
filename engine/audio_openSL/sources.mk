##############################################################
# Copyright [2018] <season4675@gmail.com>
# Module:   sources.mk
# File:     AudioManager/engine/audio_openSL/sources.mk
##############################################################

libaudiomanager_engine_dir := $(top_src_dir)/engine

libaudiomanager_include_dirs += $(libaudiomanager_engine_dir)/audio_openSL

libaudiomanager_src_files += $(libaudiomanager_engine_dir)/audio_openSL/audio_opensl_impl.cc
libaudiomanager_src_files += $(libaudiomanager_engine_dir)/audio_openSL/opensl_io.c

