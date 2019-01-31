//
//  circular_buffer.h
//  AudioManager
//
//  Created by Shichen.Fu<shichen.fsc@alibaba-inc.com> on 18        -3-1.
//  Copyright (c) 2018 Shichen.Fu. All rights reserved.
//

#ifndef AUDIOMANAGER_ENGINE_AUDIO_IOS_CIRCULAR_BUFFER_H_ 
#define AUDIOMANAGER_ENGINE_AUDIO_IOS_CIRCULAR_BUFFER_H_

#include <stdlib.h>

typedef struct _circular_buffer {
  char *buffer;
  int wp;
  int rp;
  int size;
} circular_buffer;

circular_buffer* create_circular_buffer(int bytes);
void clean_circular_buffer(circular_buffer *p);
int checkspace_circular_buffer(circular_buffer *p, int writeCheck);
int read_circular_buffer_bytes(circular_buffer *p, char *out, int bytes);
int write_circular_buffer_bytes(circular_buffer *p, char *in, int bytes);
void free_circular_buffer (circular_buffer *p);

#endif // AUDIOMANAGER_ENGINE_AUDIO_IOS_CIRCULAR_BUFFER_H_
