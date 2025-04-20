#include "colour.h"

static uint32_t GreyscaleColour(uint32_t color);

uint32_t InterpolateColour(uint32_t colour1, uint32_t colour2, float t)
{
    uint8_t r1 = (colour1 >> 16) & 0xFF;
    uint8_t g1 = (colour1 >> 8) & 0xFF;
    uint8_t b1 = colour1 & 0xFF;

    uint8_t r2 = (colour2 >> 16) & 0xFF;
    uint8_t g2 = (colour2 >> 8) & 0xFF;
    uint8_t b2 = colour2 & 0xFF;

    uint8_t r = (uint8_t)(r1 + t * (r2 - r1));
    uint8_t g = (uint8_t)(g1 + t * (g2 - g1));
    uint8_t b = (uint8_t)(b1 + t * (b2 - b1));

    return (r << 16) | (g << 8) | b;
}

uint32_t DarkenColour(uint32_t colour, float factor) {
    // Clamp factor between 0 and 1.
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    uint8_t r = (colour >> 16) & 0xFF;
    uint8_t g = (colour >> 8) & 0xFF;
    uint8_t b = colour & 0xFF;

    // Multiply each channel by the factor.
    r = (uint8_t)(r * factor);
    g = (uint8_t)(g * factor);
    b = (uint8_t)(b * factor);

    return (r << 16) | (g << 8) | b;
}

// Convert a 0xRRGGBB colour to greyscale using a weighted average.
static uint32_t GreyscaleColour(uint32_t color)
{
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    uint8_t grey = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
    return (grey << 16) | (grey << 8) | grey;
}

void ConvertPaletteToGreyscale(const uint32_t *palette, uint32_t *greyPalette)
{
    for (int i = 0; i < 7; i++) {
        greyPalette[i] = GreyscaleColour(palette[i]);
    }
}