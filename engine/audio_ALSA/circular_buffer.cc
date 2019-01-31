//
//  circular_buffer.cc
//  AudioManager
//
//  Created by Shichen.Fu<shichen.fsc@alibaba-inc.com> on 18    -3-1.
//  Copyright (c) 2018 Shichen.Fu. All rights reserved.
//

#define TAG "CircularBuffer"

#include "audio_log.h"
#include "circular_buffer.h"
#include <string.h>

circular_buffer* create_circular_buffer(int bytes) {
  circular_buffer *p;
  if ((p = (circular_buffer *)calloc(1, sizeof(circular_buffer))) == NULL) {
    return NULL;
  }
  p->size = bytes;
  p->wp = p->rp = 0;

  if ((p->buffer = (char *)calloc(bytes, sizeof(char))) == NULL) {
    free(p);
    return NULL;
  }
  return p;
}

void clean_circular_buffer(circular_buffer *p) {
  if(p == NULL) return;
  char *buffer = p->buffer;
  int size = p->size;
  memset(buffer, 0, size);
  p->wp = p->rp = 0;
  return;
}

int checkspace_circular_buffer(circular_buffer *p, int writeCheck) {
  int wp = p->wp, rp = p->rp, size = p->size;
  if (writeCheck) {
    if (wp > rp) return rp - wp + size;
    else if (wp < rp) return rp - wp;
    else return size;
  } else {
    if (wp > rp) return wp - rp;
    else if (wp < rp) return wp - rp + size;
    else return 0;
  }
}

int read_circular_buffer_bytes(circular_buffer *p, char *out, int bytes) {
  int remaining;
  int bytesread, size = p->size;
  int i = 0, rp = p->rp;
  char *buffer = p->buffer;
  if ((remaining = checkspace_circular_buffer(p, 0)) == 0) {
    return 0;
  }
  //bytesread = bytes > remaining ? remaining : bytes;
  if (bytes > remaining) {
    bytesread = 0;
  } else {
    bytesread = bytes;
  }
  for (i = 0; i < bytesread; i++) {
    out[i] = buffer[rp++];
    if (rp == size) rp = 0;
  }
  p->rp = rp;
  return bytesread;
}

int write_circular_buffer_bytes(circular_buffer *p, char *in, int bytes) {
  int remaining;
  int byteswrite, size = p->size;
  int i = 0, wp = p->wp;
  char *buffer = p->buffer;
  if ((remaining = checkspace_circular_buffer(p, 1)) == 0) {
    return 0;
  }
  byteswrite = bytes > remaining ? remaining : bytes;
  for (i = 0; i < byteswrite; i++) {
    buffer[wp++] = in[i];
    if (wp == size) wp = 0;
  }
  p->wp = wp;
  return byteswrite;
}

void free_circular_buffer(circular_buffer *p) {
  if (p == NULL) {
    KLOGE(TAG, "free_circular_buffer error, because of p is null.");
    return;
  }
  free(p->buffer);
  p->buffer = NULL;
  free(p);
  p = NULL;
}


