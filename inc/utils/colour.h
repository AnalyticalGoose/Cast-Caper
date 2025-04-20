#ifndef COLOUR_H
#define COLOUR_H

#include <stdint.h>

uint32_t InterpolateColour(uint32_t colour1, uint32_t colour2, float t);
uint32_t DarkenColour(uint32_t colour, float factor);
void ConvertPaletteToGreyscale(const uint32_t *palette, uint32_t *greyPalette);

#endif // COLOUR_H
