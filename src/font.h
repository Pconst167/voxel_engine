#ifndef FONT_H
#define FONT_H

#include <stdint.h>

#define FONT_WIDTH 5
#define FONT_HEIGHT 7
#define FONT_SPACING 1

const uint8_t *get_font_5x7_char(char character);

#endif