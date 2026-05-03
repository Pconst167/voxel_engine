#ifndef COMPUTERS_H
#define COMPUTERS_H

#include "texture.h"

#define MAX_NUM_COMPUTERS 10

typedef struct{
  uint32_t object_id;
  texture_t *screen_texture;
  struct{
    uint32_t font_size;
    uint32_t horizontal_margin;
    uint32_t vertical_margin;
    uint32_t max_num_chars_per_line;
    uint32_t max_num_lines;
    uint32_t cursor_line;
    uint32_t cursor_column;
  } terminal;
} computer_t;

extern computer_t computers[MAX_NUM_COMPUTERS];
extern uint32_t num_computers;

void computer_push(
  uint32_t object_id, 
  texture_t *screen_texture,
  uint32_t horizontal_margin,
  uint32_t vertical_margin,
  uint32_t font_size
);
void draw_char_to_computer_screen(int x, int y, char character, color_t color, int scale, texture_t *screen);
void computer_print_line(
  uint32_t computer_id,
  const char *text, color_t color
);
void computer_clear_screen(texture_t *screen, color_t color);

#endif