#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"

#define MAX_NUM_TEXTURES 100

typedef struct{
  color_t *texture;
  int texture_width; 
  int texture_height;
} texture_t;

extern texture_t texture_list[MAX_NUM_TEXTURES];
extern uint32_t num_textures;

void load_png_texture_data(texture_t *texture, char *filename);
texture_t *load_texture(char *texture_filename);

#endif