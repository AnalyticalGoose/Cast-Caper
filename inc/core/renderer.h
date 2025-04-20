#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>

void DrawSprite(int sx, int sy, int targetWidth, int targetHeight, const uint8_t sprite[16][13], const uint32_t *palette);
void DrawLine(int x1, int y1, int x2, int y2, uint32_t color);
void DrawThickLine(int x1, int y1, int x2, int y2, uint32_t colour, int thickness);
void DrawGreyscaleRectangle(int x, int y, int width, int height, uint32_t col);
void DrawColourRectangle(int sx, int sy, int width, int height, uint32_t col);
void DrawProgressBar(int sx, int sy, int width, int height, float progress, uint32_t bodyColour, uint32_t borderColour);
void DrawVerticalProgressBar(int sx, int sy, int width, int height, float progress, uint32_t bodyColour, uint32_t borderColour);
void DrawBitMap(const unsigned char bits[8], int sx, int sy, uint32_t colour, int scale);
void DrawBitMap32x32(const unsigned char bits[32][4], int sx, int sy, uint32_t colour, int scale);
void DrawChar(char c, int sx, int sy, uint32_t col, int scale);
void DrawString(const char *string, int sx, int sy, uint32_t col, int scale);
void DrawStringCentered(const char *string, int x1, int x2, int sy, uint32_t colour, int scale);
void DrawStringWrapped(const char *string, int sx, int sy, int wrapWidth, int centre, uint32_t colour, int scale);
void ClearFrame(uint32_t colour);

void DrawGridlines(int xLines, int yLines, uint32_t colour); // temp - delete on release

#endif // RENDERER_H 