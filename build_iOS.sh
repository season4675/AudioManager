#!/bin/bash

make -f iOSMakefile clean_all

make -f iOSMakefile PLATFORM=iPhoneSimulator ARCH=x86_64 -j4
make -f iOSMakefile clean

make -f iOSMakefile PLATFORM=iPhoneSimulator ARCH=i386 -j4
make -f iOSMakefile clean

make -f iOSMakefile PLATFORM=iPhoneOS ARCH=arm64 -j4
make -f iOSMakefile clean

make -f iOSMakefile PLATFORM=iPhoneOS ARCH=armv7s -j4
make -f iOSMakefile clean

make -f iOSMakefile PLATFORM=iPhoneOS ARCH=armv7 -j4

lipo -create out/arm64/libaudiomanager_iPhoneOS_arm64.a \
       out/armv7s/libaudiomanager_iPhoneOS_armv7s.a \
       out/armv7/libaudiomanager_iPhoneOS_armv7.a \
       out/x86_64/libaudiomanager_iPhoneSimulator_x86_64.a \
       out/i386/libaudiomanager_iPhoneSimulator_i386.a \
       -output out/libaudiomanager.a

cp out/libaudiomanager.a ./test/iOS/AudioManagerTest/AudioTest/libaudiomanager.a

