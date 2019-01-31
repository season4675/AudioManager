##############################################################
# Copyright [2018] <season4675@gmail.com>
# Module:   sources.mk
# File:     AudioManager/engine/audio_ALSA/sources.mk
##############################################################

libaudiomanager_engine_dir := $(top_src_dir)/engine

libaudiomanager_include_dirs += $(libaudiomanager_engine_dir)/audio_ALSA

libaudiomanager_src_files += $(libaudiomanager_engine_dir)/audio_ALSA/audio_alsa_impl.cc
libaudiomanager_src_files += $(libaudiomanager_engine_dir)/audio_ALSA/circular_buffer.cc
libaudiomanager_src_files += $(libaudiomanager_engine_dir)/audio_ALSA/alsa_io.cc
