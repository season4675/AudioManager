//
//  RecordViewController.m
//  AudioTest
//
//  Created by webseat2 on 13-10-15.
//  Copyright (c) 2013年 WebSeat. All rights reserved.
//

#import "RecordViewController.h"
#import "Record.h"
#import "Play.h"
#import "Echo.h"

@interface RecordViewController ()
{
    Record *recorder;
    Play *player;
    Echo *echo;
    BOOL initFlag;
    BOOL flag;
    BOOL echoFlag;
    BOOL recorderFlag;
    BOOL playerFlag;
}
- (IBAction)btnRecord:(id)sender;
- (IBAction)btnComplete:(id)sender;
- (IBAction)btnPlay:(id)sender;
- (IBAction)btnEcho:(id)sender;

@end

@implementation RecordViewController
- (void)viewDidLoad
{
    [super viewDidLoad];
    echo = [[Echo alloc]init];
    initFlag = NO;
    flag = NO;
    echoFlag = NO;
    recorderFlag = NO;
    playerFlag = NO;
}

- (IBAction)btnRecord:(id)sender {
    if (recorderFlag == NO) {
        [echo openRecorder];
        recorderFlag = YES;
    } else {
        [echo stopRecorder];
        recorderFlag = NO;
    }
}

- (IBAction)btnComplete:(id)sender {
    [echo stopRecorder];
}

- (IBAction)btnPlay:(id)sender {
    if (playerFlag == NO) {
        [echo openPlayer];
        playerFlag = YES;
    } else {
        [echo stopPlayer];
        playerFlag = NO;
    }
}

- (IBAction)btnEcho:(id)sender {
    if (echoFlag == NO) {
        [echo openEcho];
        [echo start];
        echoFlag = YES;
    } else {
        //[echo stop];
        [echo closeEcho];
        echoFlag = NO;
    }
}
@end
