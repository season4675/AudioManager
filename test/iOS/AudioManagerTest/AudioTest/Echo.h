//
//  Echo.h
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-3-22.
//  Copyright (c) 2018 season4675. All rights reserved.
//


@interface Echo : NSObject
{
    long audioDataLength;
    long audioDataIndex;
}
- (void) openEcho;
- (void) closeEcho;
- (void) start;
- (void) stop;
- (void) openRecorder;
- (void) stopRecorder;
- (void) openPlayer;
- (void) stopPlayer;
@end




