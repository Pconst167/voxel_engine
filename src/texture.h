#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"

#define MAX_NUM_TEXTURES 200

typedef struct{
  char filename[256];
  color_t *texture;
  int texture_width; 
  int texture_height;
} texture_t;

extern texture_t texture_list[MAX_NUM_TEXTURES];
extern uint32_t num_textures;

texture_t load_png_texture_data(char *filename);
texture_t *load_texture(char *texture_filename);
texture_t *create_blank_texture(
  uint32_t texture_width,
  uint32_t texture_height,
  color_t base_color
);
texture_t *create_texture_from_file(char *texture_filename);
texture_t *create_texture_copy(texture_t *from_texture);

#endif