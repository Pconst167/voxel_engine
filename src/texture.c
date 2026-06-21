#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "texture.h"
#include "upng.h"
#include "log.h"
#include "debug.h"

texture_t texture_list[MAX_NUM_TEXTURES];
uint32_t num_textures;

static color_t default_texture_pixels[] = {
  {.uint32=0xFF000000},
  {.uint32=0xFFFF00FF},
  {.uint32=0xFF000000},
  {.uint32=0xFFFF00FF}
};

texture_t default_texture = {
  .filename = "",
  .texture = default_texture_pixels,
  .texture_width  = 2,
  .texture_height = 2,
  .mipmaps = {default_texture_pixels},
  .mipmap_widths = {2},
  .mipmap_heights = {2},
  .num_mipmaps = 1
};

void flip_texture_pixels_vertically(color_t *pixels, int texture_width, int texture_height){
  color_t *temporary_row = malloc(sizeof(color_t) * texture_width);
  if(temporary_row == NULL){
    return;
  }

  for(int y = 0; y < texture_height / 2; y++){
    int opposite_y = texture_height - 1 - y;

    color_t *top_row = pixels + y * texture_width;
    color_t *bottom_row = pixels + opposite_y * texture_width;

    memcpy(temporary_row, top_row, sizeof(color_t) * texture_width);
    memcpy(top_row, bottom_row, sizeof(color_t) * texture_width);
    memcpy(bottom_row, temporary_row, sizeof(color_t) * texture_width);
  }

  free(temporary_row);
}


void generate_texture_mipmaps(texture_t *texture){
  texture->mipmaps[0] = texture->texture;
  texture->mipmap_widths[0] = texture->texture_width;
  texture->mipmap_heights[0] = texture->texture_height;
  texture->num_mipmaps = 1;

  while(texture->num_mipmaps < MAX_MIP_LEVELS){
    int source_mipmap_level = texture->num_mipmaps - 1;
    int source_width = texture->mipmap_widths[source_mipmap_level];
    int source_height = texture->mipmap_heights[source_mipmap_level];

    if(source_width <= 4 || source_height <= 4)
      break;

    int destination_width = source_width / 2;
    int destination_height = source_height / 2;

    color_t *source_pixels = texture->mipmaps[source_mipmap_level];
    color_t *destination_pixels = malloc(sizeof(color_t) * destination_width * destination_height);

    if(destination_pixels == NULL)
      break;

    for(int y = 0; y < destination_height; y++){
      for(int x = 0; x < destination_width; x++){
        int source_x = x * 2;
        int source_y = y * 2;

        color_t a = source_pixels[source_y * source_width + source_x];
        color_t b = source_pixels[source_y * source_width + source_x + 1];
        color_t c = source_pixels[(source_y + 1) * source_width + source_x];
        color_t d = source_pixels[(source_y + 1) * source_width + source_x + 1];

        uint32_t average_alpha = (a.rgb.alpha + b.rgb.alpha + c.rgb.alpha + d.rgb.alpha) >> 2;
        uint32_t average_red   = (a.rgb.red   + b.rgb.red   + c.rgb.red   + d.rgb.red)   >> 2;
        uint32_t average_green = (a.rgb.green + b.rgb.green + c.rgb.green + d.rgb.green) >> 2;
        uint32_t average_blue  = (a.rgb.blue  + b.rgb.blue  + c.rgb.blue  + d.rgb.blue)  >> 2;

        color_t center = a;

        int32_t sharpened_red   = average_red   + ((int32_t)center.rgb.red   - (int32_t)average_red)   / 4;
        int32_t sharpened_green = average_green + ((int32_t)center.rgb.green - (int32_t)average_green) / 4;
        int32_t sharpened_blue  = average_blue  + ((int32_t)center.rgb.blue  - (int32_t)average_blue)  / 4;

        if(sharpened_red < 0) sharpened_red = 0;
        if(sharpened_red > 255) sharpened_red = 255;
        if(sharpened_green < 0) sharpened_green = 0;
        if(sharpened_green > 255) sharpened_green = 255;
        if(sharpened_blue < 0) sharpened_blue = 0;
        if(sharpened_blue > 255) sharpened_blue = 255;

        destination_pixels[y * destination_width + x] = (color_t){
          .rgb.alpha = average_alpha,
          .rgb.blue  = sharpened_blue,
          .rgb.green = sharpened_green,
          .rgb.red   = sharpened_red
        };
      }
    }

    int destination_mipmap_level = texture->num_mipmaps++;
    texture->mipmaps[destination_mipmap_level] = destination_pixels;
    texture->mipmap_widths[destination_mipmap_level] = destination_width;
    texture->mipmap_heights[destination_mipmap_level] = destination_height;
  }
}

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

  // png loader has the origin at top left, this converts the origin to bottom left in memory
  flip_texture_pixels_vertically(texture.texture, texture.texture_width, texture.texture_height);
  generate_texture_mipmaps(&texture);

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

  generate_texture_mipmaps(new_texture);

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

  // copy data from old texture to new one
  *new_texture = *from_texture;

  for(int mipmap_level = 0; mipmap_level < from_texture->num_mipmaps; mipmap_level++){
    uint32_t total_texture_pixels = from_texture->mipmap_widths[mipmap_level] * from_texture->mipmap_heights[mipmap_level];
    new_texture->mipmaps[mipmap_level] = malloc(total_texture_pixels * sizeof(color_t));

    if(new_texture->mipmaps[mipmap_level] == NULL){
      print_log("[ERROR] could not allocate memory for new texture");
      return &default_texture;
    }

    memcpy(new_texture->mipmaps[mipmap_level], from_texture->mipmaps[mipmap_level], total_texture_pixels * sizeof(color_t));
  }

  new_texture->texture = new_texture->mipmaps[0];

  print_log("[INFO] creating a new texture as a copy of: %s, w: %u, h: %u", from_texture->filename, from_texture->texture_width, from_texture->texture_height);

  num_textures++;

  return new_texture;
}