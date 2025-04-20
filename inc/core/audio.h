#ifndef AUDIO_H
#define AUDIO_H

#include <windows.h>

typedef unsigned char (*BytebeatFunc)(unsigned long t);

BOOL InitAudio(void);
BOOL StartAudioThread(void);
void StopAudioThread(void);

void PlayMusic(const int index);

#endif // AUDIO_H


