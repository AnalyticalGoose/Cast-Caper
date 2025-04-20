#include "framebuffer.h"


UINT32 frameBuffer[FRAME_WIDTH * FRAME_HEIGHT];
BITMAPINFO bitmapInfo;


void ClearFrameBuffer(UINT32 col) {
	for (int i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; i++)
	{
		frameBuffer[i] = col;
	}
}