#ifndef MATERIAL_H
#define MATERIAL_H

#include <stdint.h>

#define MAX_NUM_TEXTURES 100

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

typedef struct{
  color_t *texture;
  int texture_width; 
  int texture_height;
} texture_t;

extern texture_t texture_list[MAX_NUM_TEXTURES];
extern uint32_t num_textures;

void load_png_texture_data(texture_t *texture, char *filename);

texture_t *create_texture(char *texture_filename);

#endif