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
  uint32_t horizontal_margin,
  uint32_t vertical_margin,
  uint32_t font_size
){
  computer_t *new_computer = &computers[num_computers];

  new_computer->object_id                       = object_id;
  new_computer->screen_texture                  = screen_texture;
  new_computer->terminal.max_num_chars_per_line = (screen_texture->texture_width  - (2 * horizontal_margin)) / ((FONT_WIDTH + FONT_SPACING) * font_size);
  new_computer->terminal.max_num_lines          = (screen_texture->texture_height - (2 * vertical_margin)  ) / ((FONT_HEIGHT + FONT_SPACING) * font_size);
  new_computer->terminal.horizontal_margin      = horizontal_margin;
  new_computer->terminal.vertical_margin        = vertical_margin;
  new_computer->terminal.font_size              = font_size;
  new_computer->terminal.cursor_line            = 0;
  new_computer->terminal.cursor_column          = 0;

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

void computer_print_line(
  uint32_t computer_id,
  const char *text, color_t color
){
  if(computer_id >= num_computers){
    print_log("[ERROR] computer id out of bounds: %u", computer_id);
    exit(1);
  }
  computer_t *computer = &computers[computer_id];
  texture_t    *screen = computer->screen_texture;

  uint32_t num_chars_in_this_line = 0;
  uint32_t max_lines              = computer->terminal.max_num_lines;
  uint32_t max_chars_per_line     = computer->terminal.max_num_chars_per_line;
  uint32_t font_size              = computer->terminal.font_size;

  uint32_t cursor_x = computer->terminal.cursor_column * font_size * (FONT_WIDTH + FONT_SPACING)  + computer->terminal.horizontal_margin;
  uint32_t cursor_y = computer->terminal.cursor_line   * font_size * (FONT_HEIGHT + FONT_SPACING) + computer->terminal.vertical_margin;

  while(*text != '\0'){
    if(*text == '\n'){
      num_chars_in_this_line = 0;
      cursor_x = computer->terminal.horizontal_margin;
      cursor_y += (FONT_HEIGHT + FONT_SPACING) * font_size;
      if(cursor_y >= max_lines * (FONT_HEIGHT + FONT_SPACING) * font_size){
        cursor_y = computer->terminal.vertical_margin;
        computer_clear_screen(screen, (color_t){.uint32 = 0xFF001020});
      }
      text++;
    }
    else if(num_chars_in_this_line >= max_chars_per_line){
      num_chars_in_this_line = 0;
      cursor_x = computer->terminal.horizontal_margin;
      cursor_y += (FONT_HEIGHT + FONT_SPACING) * font_size;
      if(cursor_y >= max_lines * (FONT_HEIGHT + FONT_SPACING) * font_size){
        cursor_y = computer->terminal.vertical_margin;
        computer_clear_screen(screen, (color_t){.uint32 = 0xFF001020});
      }
    }
    
    draw_char_to_computer_screen(cursor_x, cursor_y, *text, color, font_size, screen);
    cursor_x += (FONT_WIDTH + FONT_SPACING) * font_size;
    num_chars_in_this_line++; // increment character count in this line
    text++;
  }
  computer->terminal.cursor_line   = cursor_y / ((FONT_HEIGHT + FONT_SPACING) * font_size);
  computer->terminal.cursor_column = cursor_x / ((FONT_WIDTH + FONT_SPACING) * font_size);
}

void computer_clear_screen(texture_t *screen, color_t color){
  uint32_t total_screen_pixels = screen->texture_width * screen->texture_height;

  for(uint32_t pixel_i = 0; pixel_i < total_screen_pixels; pixel_i++){
    screen->texture[pixel_i] = color;
  }
}