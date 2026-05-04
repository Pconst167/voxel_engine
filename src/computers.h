#ifndef COMPUTERS_H
#define COMPUTERS_H

#include <stdbool.h>
#include "texture.h"

#define MAX_NUM_COMPUTERS 10
#define COMPUTER_INPUT_FIFO_SIZE 256

typedef struct{
  uint32_t object_id;
  texture_t *screen_texture;
  texture_t *blank_screen_texture;
  struct{
    uint8_t data[COMPUTER_INPUT_FIFO_SIZE];
    uint32_t read_index;
    uint32_t write_index;
  } input_fifo;
  struct{
    color_t font_color;
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
  texture_t *blank_screen_texture,
  uint32_t horizontal_margin,
  uint32_t vertical_margin,
  uint32_t font_size,
  color_t font_color
);
void draw_char_to_computer_screen(int x, int y, char character, color_t color, int scale, texture_t *screen);
void computer_print_line(uint32_t computer_id, const char *text);
void computer_clear_screen(texture_t *screen_texture, texture_t *blank_screen_texture);

bool computer_input_fifo_is_empty(computer_t *computer);
bool computer_input_fifo_is_full(computer_t *computer);
bool computer_input_fifo_push(computer_t *computer, uint8_t value);
bool computer_input_fifo_pop(computer_t *computer, uint8_t *value);

#endif