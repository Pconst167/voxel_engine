#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "upng.h"
#include "texture.h"
#include "log.h"

texture_t texture_list[MAX_NUM_TEXTURES];
uint32_t num_textures;

texture_t default_texture = {
  .texture = (color_t[]){
    {.uint32=0xFF000000},
    {.uint32=0xFFFF00FF},
    {.uint32=0xFF000000},
    {.uint32=0xFFFF00FF}
  },
  .texture_width  = 2,
  .texture_height = 2
};

texture_t load_png_texture_data(char *filename){
  upng_t *png_texture = upng_new_from_file(filename);
  if(png_texture == NULL){
    print_log("ERROR: could not load PNG file: %s\n", filename);
    return default_texture;
  }
  
  upng_decode(png_texture);

  if(upng_get_error(png_texture) != UPNG_EOK){
    print_log("ERROR: could not decode PNG file: %s\n", filename);
    upng_free(png_texture);
    return default_texture;
  }

  texture_t texture;

  unsigned width  = upng_get_width(png_texture);
  unsigned height = upng_get_height(png_texture);
  unsigned comps  = upng_get_components(png_texture); // key

  const unsigned char *src = upng_get_buffer(png_texture);

  texture.texture_width  = width;
  texture.texture_height = height;

  texture.texture = (color_t *)malloc(width * height * sizeof(color_t));
  if(texture.texture == NULL){
    print_log("ERROR: could not allocate texture memory\n");
    upng_free(png_texture);
    return default_texture;
  }

  // Convert to RGBA always
  for(unsigned i = 0; i < width * height; i++){
    uint8_t r, g, b, a;

    if(comps == 4){
      r = src[i*4 + 0];
      g = src[i*4 + 1];
      b = src[i*4 + 2];
      a = src[i*4 + 3];
    }
    else if(comps == 3){
      r = src[i*3 + 0];
      g = src[i*3 + 1];
      b = src[i*3 + 2];
      a = 0xFF; // force opaque
    }
    else {
      // optional: handle grayscale etc
      r = g = b = src[i];
      a = 0xFF;
    }

    /* ABGR */
    texture.texture[i] = (color_t){
      .rgb.alpha = a,
      .rgb.blue  = b,
      .rgb.green = g,
      .rgb.red   = r
    };
  }

  upng_free(png_texture);

  return texture;
}

texture_t *load_texture(char *texture_filename){
  // check if texture by same name already exists and return it
  for(uint32_t i = 0; i < num_textures; i++)
    if(!strcmp(texture_list[i].filename, texture_filename))
      return &texture_list[i];

  if(num_textures >= MAX_NUM_TEXTURES){
    print_log("ERROR: maximum number of textures reached: %d\n", num_textures);
    return &default_texture;
  }

  texture_t texture = load_png_texture_data(texture_filename);
  texture_list[num_textures] = texture;
  num_textures++;

  print_log("loading texture file: %s, w: %d, h: %d", texture_filename, texture.texture_width, texture.texture_height);

  return &texture_list[num_textures - 1];
}