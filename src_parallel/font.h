#ifndef FONT_H
#define FONT_H

#include <stdint.h>

#define FONT_HOR_SPACING 1
#define FONT_VER_SPACING 1

typedef enum{
  FONT_5X5,
  FONT_6X8
} font_type_t;

typedef struct{
  int width;
  int height;
  uint8_t font_chars[95][8]; // max font width = 8 bits for now
  uint8_t font_unknown[8];
} font_t;

extern font_t fonts[];

const uint8_t *get_font_char(char character, font_type_t type);

#endif
