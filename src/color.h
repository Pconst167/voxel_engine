#ifndef COLOR_H
#define COLOR_H

#include <stdint.h>

typedef struct{
  union{
    uint32_t uint32;
    struct{
      uint8_t red;
      uint8_t green;
      uint8_t blue;
      uint8_t alpha;
    } rgb;
  };
} color_t;

typedef enum{
  DEFAULT_COLOR_BLACK  = 0xFF000000,
  DEFAULT_COLOR_WHITE  = 0xFFFFFFFF,
  DEFAULT_COLOR_GREY   = 0xFF808080,
  DEFAULT_COLOR_BROWN  = 0xFF4040FF,
  DEFAULT_COLOR_BLUE   = 0xFFFF0000,
  DEFAULT_COLOR_GREEN  = 0xFF00FF00,
  DEFAULT_COLOR_YELLOW = 0xFF00FFFF,
  DEFAULT_COLOR_RED    = 0xFF0000FF,
  DEFAULT_COLOR_PINK   = 0xFFFF00FF,
  DEFAULT_COLOR_CYAN   = 0xFFFFFF00,
} default_color_t;

color_t color_from_float_rgb(float red, float green, float blue);

#endif