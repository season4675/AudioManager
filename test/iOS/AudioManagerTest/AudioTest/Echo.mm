//
//  Echo.mm
//  AudioManager
//
//  Created by season4675<season4675@gmail.com> on 18-3-22.
//  Copyright (c) 2018 season4675. All rights reserved.
//

#include <pthread.h>
#import "Echo.h"
#include "audio_manager.h"

#define VECSAMPS_STEREO 640
#define BIG_BUFFERS (1024 * 1024 * 16)

static audiomanager::AudioManager *global_handler = NULL;
static int global_player_id0 = -1;
static int isEcho = 0;
static int isRecording = 0;
static int isPlaying = 0;
static char inbuffer[VECSAMPS_STEREO] = {0,};
static char bigbuffer[BIG_BUFFERS] = {0,};
static int recordingBuffers = 0;
static int playingBuffers = 0;

@implementation Echo

void saveWave(char *data, int len)
{
    NSLog(@"Data len is: %lu", len);
    // 获取Document目录下的Log文件夹，若没有则新建
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *logDirectory = [[paths objectAtIndex:0] stringByAppendingPathComponent:@"originPcm"];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    BOOL fileExists = [fileManager fileExistsAtPath:logDirectory];
    if (!fileExists)
    {
        [fileManager createDirectoryAtPath:logDirectory  withIntermediateDirectories:YES attributes:nil error:nil];
    }
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setLocale:[[NSLocale alloc] initWithLocaleIdentifier:@"zh_CN"]];
    [formatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
    //每次启动后都保存一个新的日志文件中
    NSString *dateStr = [formatter stringFromDate:[NSDate date]];
    NSString *fileName = [dateStr stringByAppendingString:@".pcm"];
    NSString *filePath = [logDirectory stringByAppendingFormat:@"%@", fileName];
    
    if([[NSFileManager defaultManager] fileExistsAtPath:filePath] == NO)
    {
        [[NSFileManager defaultManager] createFileAtPath:filePath contents:nil attributes:nil];
    }
    FILE *file = fopen([filePath UTF8String], [@"ab+" UTF8String]);
    if(file != NULL)
    {
        fseek(file, 0, SEEK_END);
        int readSize = len;
        int result = 0;
        result = fwrite((const void *)data, 1, readSize, file);
        fclose(file);
        NSLog(@"fwrite %d bytes.", result);
    }
    else
    {
        NSLog(@"open %@ error!", fileName);
    }
}

void *startEcho_process(void *context) {
    audiomanager::AudioManager *audiomanager = global_handler;
    static int buffers;
    static int result;

    while (isEcho) {
        if (isEcho) {
            buffers = (int)audiomanager->audio_IAudioInput_read((void *)inbuffer, VECSAMPS_STEREO);

            if (buffers > 0) {
                result = (int)audiomanager->audio_IAudioOutput_write((void *)inbuffer, buffers, global_player_id0);
                //NSLog(@"Audio Manager Echo Buffers is %d! result is %d!",buffers, result);
            } else {
                //NSLog(@"Audio Manager Echo Buffers is %d! result is %d!",buffers, result);
            }
        }
        usleep(100);
    }
    return NULL;
}

void *startRecording_process(void *context) {
    audiomanager::AudioManager *audiomanager = global_handler;
    static int buffers;

    while (isRecording) {
        memset(inbuffer, 0, VECSAMPS_STEREO);
        buffers = (int)audiomanager->audio_IAudioInput_read((void *)inbuffer, VECSAMPS_STEREO);
        if (buffers > 0) {
            if ((recordingBuffers + buffers) < BIG_BUFFERS) {
                memcpy((void *)&bigbuffer[recordingBuffers], (void *)inbuffer, buffers);
                recordingBuffers += buffers;
                //NSLog(@"Recording buffers=%d total=%d", buffers, recordingBuffers);
            } else {
                isRecording = 0;
                //NSLog(@"Audio Manager Recorder Buffers is full!");
                audiomanager->audio_IAudioInput_stop();
                //audiomanager->audio_IAudioInput_close();
            }
            //NSLog(@"Audio Manager Recorder final Buffers is %d!", recordingBuffers);
        } else {
            //NSLog(@"Audio Manager Recorder Buffers is %d!", buffers);
        }
        usleep(10*1000);
    }
    return NULL;
}

void *startPlaying_process(void *context) {
    audiomanager::AudioManager *audiomanager = global_handler;
    static int buffers;

    while (isPlaying) {
        if ((playingBuffers + VECSAMPS_STEREO) < recordingBuffers) {
            buffers = (int)audiomanager->audio_IAudioOutput_write((void *)&bigbuffer[playingBuffers], VECSAMPS_STEREO, global_player_id0);
            playingBuffers += buffers;
            if (buffers > 0)
                NSLog(@"Audio Manager Player final Buffers is %d! buffers=%d", playingBuffers, buffers);
        } else {
            isPlaying = 0;
            audiomanager->audio_IAudioOutput_stop(global_player_id0);
            //audiomanager->audio_IAudioOutput_close(global_player_id0);
            NSLog(@"Audio Manager Player Buffers is empty! has player %d bytes.", playingBuffers);
            playingBuffers = 0;
            recordingBuffers = 0;
        }
        usleep(10*1000);
    }
    return NULL;
}

- (void) start
{
    isEcho = 1;
    pthread_t thread_echo;
    pthread_create(&thread_echo, NULL, startEcho_process, (void *) NULL);
    pthread_detach(thread_echo);
}

- (void) openRecorder
{
    
    NSLog(@"Audio Manager Recorder Open >>>>>");
    audiomanager::AMDataFormat in_data_format;
    audiomanager::AudioManager *audiomanager = NULL;
    if (NULL == global_handler) {
        audiomanager = audiomanager::AudioManager::Create();
        if (NULL == audiomanager) {
            return;
        }
        global_handler = audiomanager;
        
        in_data_format.format_type = audiomanager::kAMDataFormatPCMInterleaved;
        in_data_format.num_channels = 1;
        in_data_format.sample_rate = audiomanager::kAMSampleRate16K;
        in_data_format.bits_per_sample = audiomanager::kAMSampleFormatFixed16;
        audiomanager->audio_IAudioInput_open(&in_data_format);
        
    } else {
        audiomanager = global_handler;
    }
    
    isRecording = 1;
    pthread_t thread_recording;
    pthread_create(&thread_recording, NULL, startRecording_process, (void *) NULL);
    pthread_detach(thread_recording);

}

- (void) stopRecorder
{
    audiomanager::AudioManager *audiomanager = global_handler;
    isRecording = 0;
    audiomanager->audio_IAudioInput_stop();
    //audiomanager->audio_IAudioInput_close();
    //audiomanager::AudioManager::Destroy(audiomanager);
    //global_handler = NULL;
    NSLog(@"Audio Manager Recorder Stop >>>>> recordingBuffers=%d", recordingBuffers);
    if (recordingBuffers > (1024*300))
        saveWave(bigbuffer, recordingBuffers);
}

- (void) openPlayer
{
    NSLog(@"Audio Manager Player Open >>>>>");
    int result = -1;
    audiomanager::AudioManager *audiomanager = NULL;
    if (NULL == global_handler) {
        audiomanager = audiomanager::AudioManager::Create();
        if (NULL == audiomanager) {
            return;
        }
        global_handler = audiomanager;
    } else {
        audiomanager = global_handler;
    }
    audiomanager::AMDataFormat sink_data_format;
    sink_data_format.format_type = audiomanager::kAMDataFormatPCMInterleaved;
    sink_data_format.num_channels = 1;
    sink_data_format.sample_rate = audiomanager::kAMSampleRate16K;
    sink_data_format.bits_per_sample = audiomanager::kAMSampleFormatFixed16;
    result = audiomanager->audio_IAudioOutput_open(&sink_data_format);
    if (result >= 0) {
        global_player_id0 = result;
    } else {
        NSLog(@"Audio Manager Player Open failed! ret(%d)", result);
        return;
    }
    isPlaying = 1;
    pthread_t thread_playing;
    pthread_create(&thread_playing, NULL, startPlaying_process, (void *) NULL);
    pthread_detach(thread_playing);
    
}

- (void) stopPlayer
{
    audiomanager::AudioManager *audiomanager = global_handler;
    audiomanager->audio_IAudioOutput_stop(global_player_id0);
    audiomanager->audio_IAudioOutput_close(global_player_id0);
    audiomanager::AudioManager::Destroy(audiomanager);
    global_handler = NULL;
    isPlaying = 0;
    NSLog(@"Audio Manager Player Stop >>>>> playingBuffers = %d", playingBuffers);
    playingBuffers = 0;
    recordingBuffers = 0;
}

- (void) stop
{
    audiomanager::AudioManager *audiomanager = global_handler;
    isEcho = 0;
    audiomanager->audio_IAudioInput_stop();
    audiomanager->audio_IAudioOutput_stop(global_player_id0);
}

- (void) openEcho
{
    NSLog(@"Audio Manager Echo Open >>>>>");
    audiomanager::AMDataFormat sink_data_format;
    audiomanager::AMDataFormat in_data_format;
    audiomanager::AudioManager *audiomanager = NULL;
    int result = -1;
    
    if (NULL == global_handler) {
        audiomanager = audiomanager::AudioManager::Create();
        if (NULL == audiomanager) {
            return;
        }
        global_handler = audiomanager;
    } else {
        audiomanager = global_handler;
    }
    
    // init audio data format
    sink_data_format.format_type = audiomanager::kAMDataFormatPCMInterleaved;
    sink_data_format.num_channels = 1;
    sink_data_format.sample_rate = audiomanager::kAMSampleRate16K;
    sink_data_format.bits_per_sample = audiomanager::kAMSampleFormatFixed16;
    
    in_data_format.format_type = audiomanager::kAMDataFormatPCMInterleaved;
    in_data_format.num_channels = 1;
    in_data_format.sample_rate = audiomanager::kAMSampleRate16K;
    in_data_format.bits_per_sample = audiomanager::kAMSampleFormatFixed16;
    
    NSLog(@"Audio Manager Player Open >>>>>");
    result = audiomanager->audio_IAudioOutput_open(&sink_data_format);
    if (result >= 0) {
        global_player_id0 = result;
    } else {
        NSLog(@"Audio Manager Player Open failed! ret(%d)", result);
        return;
    }
    
    NSLog(@"Audio Manager Recorder Open >>>>>");
    audiomanager->audio_IAudioInput_open(&in_data_format);
}

- (void) closeEcho
{
    audiomanager::AudioManager *audiomanager = global_handler;
    isEcho = 0;
    int result = -audiomanager::kAMUnknownError;
    
    if (audiomanager != NULL) {
        result = audiomanager->audio_IAudioInput_close();
        if (result != audiomanager::kAMSuccess) {
            return;
        }
        result = audiomanager->audio_IAudioOutput_close(global_player_id0);
        if (result != audiomanager::kAMSuccess) {
            return;
        }
        result = audiomanager::AudioManager::Destroy(audiomanager);
        if (result != audiomanager::kAMSuccess) {
            return;
        }
    }
    global_handler = NULL;
}

@end
