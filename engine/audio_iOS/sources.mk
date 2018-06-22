##############################################################
# Copyright [2018] <season4675@gmail.com>
# Module:   sources.mk
# File:     AudioManager/engine/audio_iOS/sources.mk
##############################################################

libaudiomanager_engine_dir := $(top_src_dir)/engine

libaudiomanager_include_dirs += $(libaudiomanager_engine_dir)/audio_iOS

# use Audio Queue
#libaudiomanager_src_files += $(libaudiomanager_engine_dir)/audio_iOS/AudioInputQueue.mm
#libaudiomanager_src_files += $(libaudiomanager_engine_dir)/audio_iOS/AudioOutputQueue.mm

# use AudioUnit
libaudiomanager_src_files += $(libaudiomanager_engine_dir)/audio_iOS/AudioUnitInput.mm
libaudiomanager_src_files += $(libaudiomanager_engine_dir)/audio_iOS/AudioUnitOutput.mm

libaudiomanager_src_files += $(libaudiomanager_engine_dir)/audio_iOS/audio_ios_impl.mm
libaudiomanager_src_files += $(libaudiomanager_engine_dir)/audio_iOS/circular_buffer.cc
