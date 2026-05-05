#include <stdint.h>
#include <stdlib.h>
#include "computers.h"
#include "font.h"
#include "log.h"

computer_t computers[MAX_NUM_COMPUTERS];
uint32_t num_computers;

void computer_push(
  uint32_t object_id, 
  texture_t *screen_texture,
  texture_t *blank_screen_texture,
  uint32_t horizontal_margin,
  uint32_t vertical_margin,
  uint32_t font_size,
  color_t font_color
){
  computer_t *new_computer = &computers[num_computers];

  new_computer->object_id                       = object_id;
  new_computer->screen_texture                  = screen_texture;
  new_computer->blank_screen_texture            = blank_screen_texture;
  new_computer->terminal.max_num_chars_per_line = (screen_texture->texture_width  - (2 * horizontal_margin)) / ((FONT_WIDTH  + FONT_HOR_SPACING) * font_size);
  new_computer->terminal.max_num_lines          = (screen_texture->texture_height - (2 * vertical_margin)  ) / ((FONT_HEIGHT + FONT_VER_SPACING) * font_size);
  new_computer->terminal.horizontal_margin      = horizontal_margin;
  new_computer->terminal.vertical_margin        = vertical_margin;
  new_computer->terminal.font_size              = font_size;
  new_computer->terminal.cursor_line            = 0;
  new_computer->terminal.cursor_column          = 0;
  new_computer->terminal.font_color             = font_color;
  new_computer->terminal.char_hor_pixels        = (FONT_WIDTH  + FONT_HOR_SPACING) * font_size;
  new_computer->terminal.char_ver_pixels        = (FONT_HEIGHT + FONT_VER_SPACING) * font_size;

  num_computers++;
}

void draw_char_to_computer_screen(int x, int y, char character, color_t color, int scale, texture_t *screen){
  const uint8_t *glyph = get_font_terminal_char(character);

  for(int row = 0; row < FONT_HEIGHT; row++){
    uint8_t bits = glyph[row];

    for(int col = 0; col < FONT_WIDTH; col++){
      if(bits & (1 << (FONT_WIDTH - 1 - col))){
        for(int scale_y = 0; scale_y < scale; scale_y++){
          int pixel_y = y + row * scale + scale_y;

          if(pixel_y < 0 || pixel_y >= screen->texture_height) continue;

          for(int scale_x = 0; scale_x < scale; scale_x++){
            int pixel_x = x + col * scale + scale_x;
            if(pixel_x < 0 || pixel_x >= screen->texture_width) continue;
            screen->texture[pixel_y * screen->texture_width + pixel_x] = color;
          }
        }
      }
    }
  }
}

void computer_print_char(uint32_t computer_id, const char chr){
  computer_t *computer            = &computers[computer_id];
  texture_t *screen_texture       = computer->screen_texture;
  texture_t *blank_screen_texture = computer->blank_screen_texture;

  uint32_t max_lines              = computer->terminal.max_num_lines;
  uint32_t max_chars_per_line     = computer->terminal.max_num_chars_per_line;
  uint32_t font_size              = computer->terminal.font_size;
  color_t font_color              = computer->terminal.font_color;

  switch(chr){
    case '\n':
      computer->terminal.cursor_column = 0;
      computer->terminal.cursor_line++;
      if(computer->terminal.cursor_line >= max_lines){
        computer->terminal.cursor_line = 0;
        computer_clear_screen(screen_texture, blank_screen_texture);
      }
      break;

    case '\r':
      computer->terminal.cursor_column = 0;
      break;
    
    case 0x08:
      if(computer->terminal.cursor_column > 0) computer->terminal.cursor_column--;
      break;
    
    default:
      uint32_t cursor_x = computer->terminal.cursor_column * computer->terminal.char_hor_pixels + computer->terminal.horizontal_margin;
      uint32_t cursor_y = computer->terminal.cursor_line   * computer->terminal.char_ver_pixels + computer->terminal.vertical_margin;

      draw_char_to_computer_screen(cursor_x, cursor_y, chr, font_color, font_size, screen_texture);
      computer->terminal.cursor_column++;

      if(computer->terminal.cursor_column >= max_chars_per_line){
        computer->terminal.cursor_column = 0;
        computer->terminal.cursor_line++;
        if(computer->terminal.cursor_line >= max_lines){
          computer->terminal.cursor_line = 0;
          computer_clear_screen(screen_texture, blank_screen_texture);
        }
      }
  }
}

void computer_printf(uint32_t computer_id, const char *text, ...){
  va_list args;
  char formatted_text[512] = {0};

  va_start(args, text);
  vsprintf(formatted_text, text, args);
  va_end(args);

  computer_print_line(computer_id, formatted_text);
}

void computer_print_line(uint32_t computer_id, const char *text){
  if(computer_id >= num_computers){
    print_log("[ERROR] computer id out of bounds: %u", computer_id);
    exit(1);
  }

  for(;;){
    if(*text == '\0') return;
    computer_print_char(computer_id, *text);
    text++;
  }
}

void computer_clear_screen(texture_t *screen_texture, texture_t *blank_screen_texture){
  uint32_t total_screen_pixels = screen_texture->texture_width * screen_texture->texture_height;

  for(uint32_t pixel_i = 0; pixel_i < total_screen_pixels; pixel_i++){
    screen_texture->texture[pixel_i] = blank_screen_texture->texture[pixel_i];
  }
}

bool computer_input_fifo_is_empty(computer_t *computer){
  return computer->input_fifo.read_index == computer->input_fifo.write_index;
}

bool computer_input_fifo_is_full(computer_t *computer){
  uint32_t next_write_index = (computer->input_fifo.write_index + 1) & (COMPUTER_INPUT_FIFO_SIZE - 1);

  return next_write_index == computer->input_fifo.read_index;
}

bool computer_input_fifo_push(computer_t *computer, uint8_t value){
  uint32_t next_write_index = (computer->input_fifo.write_index + 1) & (COMPUTER_INPUT_FIFO_SIZE - 1);

  if(next_write_index == computer->input_fifo.read_index) return false;

  computer->input_fifo.data[computer->input_fifo.write_index] = value;
  computer->input_fifo.write_index = next_write_index;

  return true;
}

bool computer_input_fifo_pop(computer_t *computer, uint8_t *value){
  if(computer->input_fifo.read_index == computer->input_fifo.write_index) return false;

  *value = computer->input_fifo.data[computer->input_fifo.read_index];
  computer->input_fifo.read_index = (computer->input_fifo.read_index + 1) & (COMPUTER_INPUT_FIFO_SIZE - 1);

  return true;
}