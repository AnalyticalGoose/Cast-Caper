#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <windows.h>

#define FRAME_WIDTH 1020
#define FRAME_HEIGHT 540
#define FRAME_SCALE 1

extern UINT32 frameBuffer[];
extern BITMAPINFO bitmapInfo;

void ClearFrameBuffer(UINT32 col);

#endif // FRAMEBUFFER_H