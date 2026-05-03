#ifndef FONT_H
#define FONT_H

#include <stdint.h>

#define FONT_WIDTH 6
#define FONT_HEIGHT 8
#define FONT_SPACING 0

const uint8_t *get_font_terminal_char(char character);

#endif
