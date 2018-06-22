cmake_minimum_required(VERSION 3.4.1)

set(lib_DIR ${CMAKE_SOURCE_DIR}/../jniLibs)

add_library(lib_audiomanager SHARED IMPORTED)
set_target_properties(lib_audiomanager PROPERTIES IMPORTED_LOCATION
    ${lib_DIR}/${ANDROID_ABI}/libaudiomanager.so)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -Wall")

add_library(native-audio-jni SHARED
            native-audio-jni.cpp)

# Include libraries needed for native-audio-jni lib
target_link_libraries(native-audio-jni
                      android
                      log
                      lib_audiomanager)
