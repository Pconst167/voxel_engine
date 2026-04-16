#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <math.h>
#include "display.h"
#include "vector.h"
#include "mesh.h"
#include "triangle.h"
#include "array.h"
#include "matrix.h"
#include "light.h"
#include "misc.h"
#include "upng.h"
#include "camera.h"
#include "text.h"
#include "clipping.h"
#include "object.h"
#include "color.h"
#include "object.h"

void render_text();
void free_resources(void);
void update_and_render_color_buffer(void) ;
void object_quicksort(object_sort_key_t *array, int count);
void qs(object_sort_key_t *array, int left, int right);
void object_render_debug();
void render_sun(void);
void update_state(void);
void update_light(void);
void process_input(void);
void setup(void) ;

inline void rasterize_triangle(
  triangle_t *triangle, 
  color_t *mesh_texture, 
  uint32_t texture_width, uint32_t texture_height, 
  blend_mode_t blend_mode,
  uint8_t alpha_cutoff,
  color_t solid_color,
  float opacity,
  bool reflective,
  color_t emissive_color,
  float emissive_intensity
);

inline void rasterize_triangle_no_zbuffer_write(
  triangle_t *triangle, 
  color_t *mesh_texture, 
  uint32_t texture_width, uint32_t texture_height, 
  blend_mode_t blend_mode,
  uint8_t alpha_cutoff,
  color_t solid_color,
  float opacity,
  bool reflective,
  color_t emissive_color,
  float emissive_intensity
);

#endif