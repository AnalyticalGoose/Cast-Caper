#include "font.h"
#include "framebuffer.h"
#include "renderer.h"

#include <math.h>

#define MAX_LINE_LENGTH 512


void DrawSprite(int sx, 
                int sy, 
                int targetWidth, 
                int targetHeight,
                const uint8_t sprite[16][13], 
                const uint32_t *palette)
{
    const int sourceWidth = 13;
    const int sourceHeight = 16;

    // Calculate scaling factors for mapping target coordinates to source coordinates.
    float scaleX = (float)targetWidth / sourceWidth;
    float scaleY = (float)targetHeight / sourceHeight;

    for (int ty = 0; ty < targetHeight; ty++) 
    {
        int fb_y = sy + ty;
        for (int tx = 0; tx < targetWidth; tx++) 
        {
            int fb_x = sx + tx;

            // Determine the corresponding sprite coordinates.
            int sprite_x = (int)(tx / scaleX);
            int sprite_y = (int)(ty / scaleY);

            // Get the palette index from the sprite.
            uint8_t index = sprite[sprite_y][sprite_x];
            uint32_t col = palette[index];

            if (index != 0) {
                frameBuffer[fb_y * 1020 + fb_x] = col;
            }
        }
    }
}



void DrawLine(int x1, int y1, int x2, int y2, uint32_t colour) {
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        frameBuffer[y1 * FRAME_WIDTH + x1] = colour;

        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

// Draw a filled square centered at (x, y) with side length = thickness.
static void DrawThickPixel(int x, int y, int thickness, uint32_t colour) {
    // Compute an offset so the square is centered at (x,y).
    int offset = thickness / 2;
    for (int j = 0; j < thickness; j++) {
        for (int i = 0; i < thickness; i++) {
            int drawX = x - offset + i;
            int drawY = y - offset + j;
            // Optionally, check bounds here if necessary.
            frameBuffer[drawY * FRAME_WIDTH + drawX] = colour;
        }
    }
}

void DrawThickLine(int x1, int y1, int x2, int y2, uint32_t colour, int thickness) {
    int prevX = -1, prevY = -1; // invalid initial coordinates
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        // If we haven't drawn any block yet or the current point is outside the
        // previously drawn thick pixel's square, draw a new thick pixel.
        if (prevX == -1 || prevY == -1 ||
            (x1 < prevX - thickness / 2 || x1 > prevX + thickness / 2 ||
                y1 < prevY - thickness / 2 || y1 > prevY + thickness / 2)) {
            DrawThickPixel(x1, y1, thickness, colour);
            prevX = x1;
            prevY = y1;
        }

        if (x1 == x2 && y1 == y2)
            break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}


void DrawGreyscaleRectangle(int sx, int sy, int width, int height, uint32_t colour)
{
    uint32_t *rowStart = &frameBuffer[sy * FRAME_WIDTH + sx];

    for (int y = 0; y < height; y++) 
    {
        memset(rowStart, colour, width * sizeof(uint32_t));
        rowStart += FRAME_WIDTH;
    }
}


void DrawColourRectangle(int sx, int sy, int width, int height, uint32_t colour)
{
    for (int y = sy; y < sy + height; y++) 
    {
        uint32_t *row = &frameBuffer[y * FRAME_WIDTH + sx];
        for (int x = 0; x < width; x++) 
        {
            row[x] = colour;
        }
    }
}


void DrawProgressBar(int sx, 
                     int sy, 
                     int width, 
                     int height, 
                     float progress, 
                     uint32_t bodyColour, 
                     uint32_t borderColour)
{
    progress = (float)fmax(0, fmin(100, progress));
    progress = progress / 100;

    DrawColourRectangle(sx, sy, width, height, borderColour);
    DrawColourRectangle(sx + 1, sy + 1, (int)(width * progress) - 2, height - 2, bodyColour);
    return;
}

void DrawVerticalProgressBar(int sx,
    int sy,
    int width,
    int height,
    float progress,
    uint32_t bodyColour,
    uint32_t borderColour)
{
    // Clamp progress to between 0 and 100.
    progress = fmax(0, fmin(100, progress));

    // Convert progress to fraction (0.0 to 1.0)
    float fraction = progress / 100.0f;

    // Draw the outer border rectangle.
    DrawColourRectangle(sx, sy, width, height, borderColour);

    // Calculate the fill height based on progress.
    int fillHeight = (int)(height * fraction);

    // Avoid negative height for fill (fillHeight-2 should be at least 0).
    if (fillHeight < 2)
        fillHeight = 2;

    // Draw the fill rectangle:
    // We start drawing from the bottom (sy + height) upward,
    // and account for the border (hence +1 offset and subtract 2 from width/height).
    DrawColourRectangle(sx + 1, sy + height - fillHeight + 1, width - 2, fillHeight - 2, bodyColour);
}


void DrawBitMap(const unsigned char bits[8], int sx, int sy, uint32_t colour, int scale)
{
    for (int row = 0; row < 8; row++)
    {
        for (int dy = 0; dy < scale; dy++) // Scale vertically
        {
            uint32_t *dest = &frameBuffer[(sy + row * scale + dy) * FRAME_WIDTH + sx];
            for (int col = 0; col < 8; col++)
            {
                if (bits[row] & (1 << col))
                {
                    for (int dx = 0; dx < scale; dx++) // Scale horizontally
                    {
                        dest[col * scale + dx] = colour;
                    }
                }
            }
        }
    }
}

void DrawBitMap32x32(const unsigned char bits[32][4], int sx, int sy, uint32_t colour, int scale)
{
    // Loop over each row of the 32x32 sprite.
    for (int row = 0; row < 32; row++) {
        // For vertical scaling, repeat each row 'scale' times.
        for (int dy = 0; dy < scale; dy++) {
            // Compute the destination pointer for the current scaled row.
            uint32_t *dest = &frameBuffer[(sy + row * scale + dy) * FRAME_WIDTH + sx];
            // Loop over each pixel in the row (0 to 31).
            for (int col = 0; col < 32; col++) {
                // Determine which byte and bit correspond to this pixel.
                int byteIndex = col / 8;
                int bitIndex = col % 8;
                // Check if the corresponding bit is set.
                // Assuming the MSB is the leftmost pixel in the byte:
                if (bits[row][byteIndex] & (1 << (7 - bitIndex))) {
                    // Draw this pixel with horizontal scaling.
                    for (int dx = 0; dx < scale; dx++) {
                        dest[col * scale + dx] = colour;
                    }
                }
            }
        }
    }
}

void DrawChar(char c, int sx, int sy, uint32_t colour, int scale)
{
    if (c < 32 || c > 127 || scale < 1)
    {
        return;
    }
    int index = c - 32;
    for (int row = 0; row < 8; row++)
    {
        unsigned char bits = font8x8[index][row];
        for (int dy = 0; dy < scale; dy++) // Scale vertically
        {
            uint32_t *dest = &frameBuffer[(sy + row * scale + dy) * FRAME_WIDTH + sx];
            for (int col = 0; col < 8; col++)
            {
                if (bits & (1 << col))
                {
                    for (int dx = 0; dx < scale; dx++) // Scale horizontally
                    {
                        dest[col * scale + dx] = colour;
                    }
                }
            }
        }
    }
}


void DrawString(const char *string, int sx, int sy, uint32_t colour, int scale)
{
    while (*string)
    {
        DrawChar(*string, sx, sy, colour, scale);
        sx += (8 * scale);
        string++;
    }
}


void DrawStringCentered(const char *string, int x1, int x2, int sy, uint32_t colour, int scale)
{
    int centreX = (x1 + x2) / 2;

    int length = (int)strlen(string);
    int textWidth = length * (8 * scale);

    int sx = centreX - (textWidth / 2) + 1;

    DrawString(string, sx, sy, colour, scale);
}


void DrawStringWrapped(const char *text, int sx, int sy, int wrapWidth, int center, uint32_t colour, int scale)
{
    int currentY = sy;
    char lineBuffer[MAX_LINE_LENGTH] = { 0 };
    int bufferIndex = 0;
    const char *p = text;

    while (*p)
    {
        // If we hit a newline or end of text, finalize the current line.
        if (*p == '\n' || *p == '\0')
        {
            // Null-terminate the line.
            lineBuffer[bufferIndex] = '\0';

            // Compute the width of the line.
            int lineWidth = bufferIndex * 8;  // each char is 8 pixels wide.
            int offset = center ? ((wrapWidth - lineWidth) / 2) : 0;

            // Draw the line starting at sx+offset.
            DrawString(lineBuffer, sx + offset, currentY, colour, scale);

            // Prepare for the next line.
            currentY += 10;
            bufferIndex = 0;
            if (*p == '\n')
                p++;
            continue;
        }

        // Look ahead to see if we can add the current word.
        // We'll assume words are delimited by spaces.
        const char *wordStart = p;
        int wordLen = 0;
        while (*p != ' ' && *p != '\n' && *p != '\0')
        {
            wordLen++;
            p++;
        }

        // Check if the current line (stored in lineBuffer) plus the new word would exceed the wrapWidth.
        int currentLineWidth = bufferIndex * 8;
        int wordWidth = wordLen * 8;
        if (currentLineWidth + wordWidth > wrapWidth && bufferIndex > 0)
        {
            // We need to break the line here. Finish the current line.
            lineBuffer[bufferIndex] = '\0';
            int offset = center ? ((wrapWidth - currentLineWidth) / 2) : 0;
            DrawString(lineBuffer, sx + offset, currentY, colour, scale);

            // Start a new line.
            currentY += 10;
            bufferIndex = 0;
        }

        // Copy the word into the line buffer.
        for (int i = 0; i < wordLen; i++)
        {
            if (bufferIndex < MAX_LINE_LENGTH - 1)
                lineBuffer[bufferIndex++] = wordStart[i];
        }

        // If the word is followed by a space, add that space.
        if (*p == ' ')
        {
            if (bufferIndex < MAX_LINE_LENGTH - 1)
                lineBuffer[bufferIndex++] = ' ';
            p++;
        }
    }

    // Draw any remaining text in the buffer.
    if (bufferIndex > 0)
    {
        lineBuffer[bufferIndex] = '\0';
        int lineWidth = bufferIndex * 8;
        int offset = center ? ((wrapWidth - lineWidth) / 2) : 0;
        DrawString(lineBuffer, sx + offset, currentY, colour, scale);
    }
}


// temp - delete on release
void DrawGridlines(int xLines, int yLines, uint32_t colour)
{
    if (xLines < 2 || yLines < 2)
        return;

    int xSpacing = FRAME_WIDTH / xLines;
    int ySpacing = FRAME_HEIGHT / yLines;

    for (int i = 1; i < yLines; i++)
    {
        int y = i * ySpacing;
        if (y < 0) y = 0;
        if (y >= FRAME_HEIGHT) y = FRAME_HEIGHT - 1;
        DrawLine(0, y, FRAME_WIDTH - 1, y, colour);
    }

    for (int i = 1; i < xLines; i++)
    {
        int x = i * xSpacing;
        if (x < 0) x = 0;
        if (x >= FRAME_WIDTH) x = FRAME_WIDTH - 1;
        DrawLine(x, 0, x, FRAME_HEIGHT - 1, colour);
    }
}


void ClearFrame(uint32_t colour)
{
    ClearFrameBuffer(colour);
}