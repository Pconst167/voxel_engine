#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "upng.h"
#include "texture.h"
#include "log.h"
#include "debug.h"

texture_t texture_list[MAX_NUM_TEXTURES];
uint32_t num_textures;

texture_t default_texture = {
  .filename = "",
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
    print_log("[ERROR] could not load PNG file: %s", filename);
    return default_texture;
  }
  
  upng_decode(png_texture);

  if(upng_get_error(png_texture) != UPNG_EOK){
    print_log("[ERROR] could not decode PNG file: %s", filename);
    upng_free(png_texture);
    return default_texture;
  }

  texture_t texture;

  unsigned width  = upng_get_width(png_texture);
  unsigned height = upng_get_height(png_texture);
  unsigned comps  = upng_get_components(png_texture); // key

  const unsigned char *src = upng_get_buffer(png_texture);

  // copy texture filename into new texture slot
  snprintf(texture.filename, sizeof(texture.filename), "%s", filename);
  texture.texture_width  = width;
  texture.texture_height = height;

  texture.texture = (color_t *)malloc(width * height * sizeof(color_t));
  if(texture.texture == NULL){
    print_log("[ERROR] could not allocate texture memory");
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

/*
  create a new texture by loading a texture from file:
    this function checks if a texture with the same filename was loaded before and if so, returns it instead of creating a new one
*/
texture_t *load_texture(char *texture_filename){
  // check if texture by same name already exists and return it
  for(uint32_t i = 0; i < num_textures; i++)
    if(!strcmp(texture_list[i].filename, texture_filename))
      return &texture_list[i];

  if(num_textures >= MAX_NUM_TEXTURES){
    print_log("[ERROR] maximum number of textures reached: %d", num_textures);
    return &default_texture;
  }

  // NOTE: texture filename is written in load_png_texture_data function, so below, the filename is copied char by char into new texture filename
  texture_t texture = load_png_texture_data(texture_filename);
  texture_list[num_textures] = texture;
  num_textures++;

  print_log("loading texture file: %s, w: %d, h: %d", texture_filename, texture.texture_width, texture.texture_height);

  return &texture_list[num_textures - 1];
}

/*
  create a new solid color texture
*/
texture_t *create_blank_texture(
  uint32_t texture_width,
  uint32_t texture_height,
  color_t base_color
){
  if(num_textures >= MAX_NUM_TEXTURES){
    print_log("[ERROR] maximum number of textures reached: %d", num_textures);
    return &default_texture;
  }

  if(texture_width == 0 || texture_height == 0) {
    print_log("[ERROR] invalid texture size: w: %u, h: %u", texture_width, texture_height);
    return &default_texture;
  }

  texture_t *new_texture = &texture_list[num_textures];

  // allocate memory for blank texture
  color_t *texture_memory = malloc(sizeof(color_t) * texture_width * texture_height);
  if(texture_memory == NULL){
    print_log("[ERROR] could not allocate memory for new blank texture");
    return &default_texture;
  }

  // create new texture
  new_texture->filename[0]    = '\0'; // no filename needed
  new_texture->texture_width  = texture_width;
  new_texture->texture_height = texture_height;
  new_texture->texture        = texture_memory;

  uint32_t total_texture_pixels = texture_width * texture_height;
  for(uint32_t pixel_i = 0; pixel_i < total_texture_pixels; pixel_i++)
    new_texture->texture[pixel_i] = base_color;

  num_textures++;

  print_log("[INFO] creating new blank texture: w: %u, h: %u, color: %x", texture_width, texture_height, base_color.uint32);

  return new_texture;
}

texture_t *create_texture_from_file(char *texture_filename){
  if(num_textures >= MAX_NUM_TEXTURES){
    print_log("[ERROR] maximum number of textures reached: %d", num_textures);
    return &default_texture;
  }

  texture_t *new_texture = &texture_list[num_textures];

  // NOTE: texture filename is written in load_png_texture_data function, so below, the filename is copied char by char into new texture filename by struct copying rules
  *new_texture = load_png_texture_data(texture_filename);
  print_log("[INFO] creating new blank texture from file: %s, w: %u, h: %u", texture_filename, new_texture->texture_width, new_texture->texture_height);

  num_textures++;

  return new_texture;
}

texture_t *create_texture_copy(texture_t *from_texture){
  if(num_textures >= MAX_NUM_TEXTURES){
    print_log("[ERROR] maximum number of textures reached: %d", num_textures);
    return &default_texture;
  }

  texture_t *new_texture = &texture_list[num_textures];

  // allocate memory for new texture
  color_t *new_texture_memory = (color_t *)malloc(from_texture->texture_width * from_texture->texture_height * sizeof(color_t));
  if(new_texture_memory == NULL){
    print_log("[ERROR] could not allocate memory for new texture");
    return &default_texture;
  }

  // copy data from old texture to new one
  *new_texture = *from_texture;

  // override pointer
  new_texture->texture = new_texture_memory;

  // copy pixel data from old texture memory into new texture memory
  uint32_t total_texture_pixels = from_texture->texture_width * from_texture->texture_height;
  memcpy(new_texture->texture, from_texture->texture, total_texture_pixels * sizeof(color_t));

  print_log("[INFO] creating a new texture as a copy of: %s, w: %u, h: %u", from_texture->filename, from_texture->texture_width, from_texture->texture_height);

  num_textures++;

  return new_texture;
}