#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <math.h>

#define SAMPLE_RATE 8000  // or 44100 etc.
#define BUFFER_SIZE 4096  // Adjust as needed.

// Global audio device handle.
HWAVEOUT hWaveOut = NULL;
WAVEHDR waveHeader;
static char audioBuffer[BUFFER_SIZE];

// Audio thread handle
static HANDLE hAudioThread = NULL;
static volatile BOOL audioRunning = FALSE;

typedef unsigned char (*BytebeatFunc)(unsigned long t);
static BytebeatFunc currentBytebeat = NULL;

static unsigned char Track0(unsigned long t);
static unsigned char Track1(unsigned long t);
static unsigned char Track2(unsigned long t);

static void SetBytebeatFunction(BytebeatFunc func) {
    currentBytebeat = func;
}

//static void FillAudioBuffer(char *buffer, size_t size, unsigned long startT) 
//{
//    static int prevSample = 0;
//    if (!currentBytebeat) return;
//
//    unsigned long t = startT;
//    for (size_t i = 0; i < size; i++) 
//    {
//        int rawSample = (int)currentBytebeat(t);
//
//        int filtered = rawSample - prevSample;
//        // Bring it back into unsigned range (simple re-offset)
//        filtered += 128;
//        // Clamp the result to the 8-bit range [0, 255]
//        if (filtered < 0)
//        {
//            filtered = 0;
//        }
//
//        if (filtered > 255)
//        {
//            filtered = 255;
//        }
//
//        buffer[i] = (char)filtered;
//        prevSample = rawSample;
//        t++;
//    }
//}


static void FillAudioBuffer(char *buffer, size_t size, unsigned long startT) {
    if (!currentBytebeat) return;

    unsigned long t = startT;
    for (size_t i = 0; i < size; i++) {
        buffer[i] = currentBytebeat(t);
        t++;
    }
}


// A function to initialize the audio device.
int InitAudio() 
{
    WAVEFORMATEX wfx = { 0 };
    wfx.nSamplesPerSec = SAMPLE_RATE;
    wfx.wBitsPerSample = 8;
    wfx.nChannels = 1;
    wfx.cbSize = 0;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    MMRESULT result = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    return (result == MMSYSERR_NOERROR);
}

static void SubmitAudioBuffer(char *buffer, size_t size) 
{
    ZeroMemory(&waveHeader, sizeof(WAVEHDR));
    waveHeader.lpData = buffer;
    waveHeader.dwBufferLength = (DWORD)size;
    waveHeader.dwFlags = 0;

    waveOutPrepareHeader(hWaveOut, &waveHeader, sizeof(WAVEHDR));
    waveOutWrite(hWaveOut, &waveHeader, sizeof(WAVEHDR));
}

// Audio thread procedure. This runs continuously to fill and submit audio buffers.
static DWORD WINAPI AudioThreadProc(LPVOID lpParam) 
{
    unsigned long t = 0; // Global time counter.
    while (audioRunning) {
        FillAudioBuffer(audioBuffer, BUFFER_SIZE, t);
        SubmitAudioBuffer(audioBuffer, BUFFER_SIZE);
        // Sleep duration: (1000 ms * Buffer Size) / Sample Rate
        Sleep((DWORD)(1000.0 * BUFFER_SIZE / (float)SAMPLE_RATE));
        t += BUFFER_SIZE;
    }
    return 0;
}


BOOL StartAudioThread() 
{
    audioRunning = TRUE;
    hAudioThread = CreateThread(NULL, 0, AudioThreadProc, NULL, 0, NULL);
    return (hAudioThread != NULL);
}

void StopAudioThread() 
{
    audioRunning = FALSE;
    if (hAudioThread) {
        WaitForSingleObject(hAudioThread, INFINITE);
        CloseHandle(hAudioThread);
        hAudioThread = NULL;
    }
    // Also reset or close the audio device if needed:
    if (hWaveOut) {
        waveOutReset(hWaveOut);
        waveOutClose(hWaveOut);
        hWaveOut = NULL;
    }
}

void PlayMusic(const int index)
{
    switch (index)
    {
        case 0:
        {
            SetBytebeatFunction(Track0);
        } break;
        case 1:
        {
            SetBytebeatFunction(Track1);
        } break;
        case 2:
        {
            SetBytebeatFunction(Track2);
        } break;
    }
}

static unsigned char Track0(unsigned long t) {
    return (t >> 10 ^ t >> 11) % 5 * ((t >> 14 & 3 ^ t >> 15 & 1) + 1) * t % 99 + ((3 + (t >> 14 & 3) - (t >> 16 & 1)) / 3 * t % 99 & 64);
}


static unsigned char Track1(unsigned long t) {
    return 131072 > t % 262144 ? t / 64 >> 3 & 2 * t & 10 * t | t >> 5 & 6 * t & (t >> 4 | t >> 5) : 131072 < t % 262144 & 163840 > t % 262144 ? t >> 4 & 8 * t & (t >> 5 | t >> 4) | 3 * t & 10 * t : 163840 < t % 262144 & 196608 > t % 262144 ? t >> 4 & 8 * t & (t >> 5 | t >> 4) | 3 * t & 6 * t : 196608 < t % 262144 & 229376 > t % 262144 ? t >> 4 & 8 * t & (t >> 5 | t >> 4) | 4 * t & 6 * t : 229376 < t % 262144 & 245760 > t % 262144 ? t >> 4 & 8 * t & (t >> 5 | t >> 4) | 4 * t & 2 * t : t >> 4 & 8 * t & t >> 4 | (4 * t & 2 * t) >> 20;
}

static unsigned char Track2(unsigned long t) {
    return (t * (t >> 5 | t >> 8)) >> 3;
}


