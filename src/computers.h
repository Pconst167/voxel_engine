#ifndef COMPUTERS_H
#define COMPUTERS_H

#include <stdbool.h>
#include <stdarg.h>
#include "texture.h"

#define MAX_NUM_COMPUTERS 10
#define COMPUTER_INPUT_FIFO_SIZE 256

// TERMINAL CODES
#define TERM_ETX 0x03 // CTRL + C
#define TERM_EOT 0x04 // end of input/transmission/EOF
#define TERM_BEL 0x07 // bell
#define TERM_BS  0x08 // backspace
#define TERM_TAB 0x09 // tab
#define TERM_LF  0x0A // line feed
#define TERM_CR  0x0D // carriage return
#define TERM_ESC 0x1B // escape

#define PRINTABLE_ASCII_START 0x20
#define PRINTABLE_ASCII_END   0x7E

// terminal input codes: terminal -> CPU only
#define KEY_UP        0x80 // up arrow
#define KEY_DOWN      0x81 // down arrow
#define KEY_LEFT      0x82 // left arrow
#define KEY_RIGHT     0x83 // right arrow
#define KEY_DEL       0x84 // delete
#define KEY_HOME      0x85 // home key
#define KEY_END       0x86 // end key
#define KEY_PAGE_UP   0x87 // page up
#define KEY_PAGE_DOWN 0x88 // page down
#define KEY_INSERT    0x89 // insert

// terminal command codes: cpu -> terminal only
#define TERM_CMD_CLR_SCR      0xC0
#define TERM_CMD_CURSOR_HOME  0xC1
#define TERM_CMD_CURSOR_END   0xC2
#define TERM_CMD_CLEAR_LINE   0xC3
#define TERM_CMD_CURSOR_LEFT  0xC4
#define TERM_CMD_CURSOR_RIGHT 0xC5
#define TERM_CMD_CURSOR_UP    0xC6
#define TERM_CMD_CURSOR_DOWN  0xC7
// parametrized command codes
#define TERM_CMD_SET_CURSOR_X 0xD0
#define TERM_CMD_SET_CURSOR_Y 0xD1
#define TERM_CMD_SET_COLOR    0xD2


typedef struct{
  uint32_t object_id;               // owner object
  texture_t *screen_texture;        // screen texture that text is written on
  texture_t *blank_screen_texture;  // used to clear screen
  uint32_t cycles_per_frame;        // how many cycles execute for each frame
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
    uint32_t char_hor_pixels;
    uint32_t char_ver_pixels;
  } terminal;
} computer_t;

extern computer_t computers[MAX_NUM_COMPUTERS];
extern uint32_t num_computers;

void computer_declare(
  uint32_t object_id, 
  texture_t *screen_texture,
  texture_t *blank_screen_texture,
  uint32_t cycles_per_frame,
  color_t font_color,
  uint32_t font_size,
  uint32_t horizontal_margin,
  uint32_t vertical_margin
);
void draw_char_to_computer_screen(int x, int y, char character, color_t color, int scale, texture_t *screen);
void computer_print_char(uint32_t computer_id, const uint8_t chr);
void computer_print_line(uint32_t computer_id, const char *text);
void computer_printf(uint32_t computer_id, const char *text, ...);
void computer_clear_screen(computer_t *computer);

bool computer_input_fifo_is_empty(computer_t *computer);
bool computer_input_fifo_is_full(computer_t *computer);
bool computer_input_fifo_push(computer_t *computer, uint8_t value);
bool computer_input_fifo_pop(computer_t *computer, uint8_t *value);

#endif