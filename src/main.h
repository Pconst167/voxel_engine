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
#include "chunk.h"

void render_text();
void free_resources(void);
void update_and_render_color_buffer2(void) ;
void object_quicksort(object_sort_key_t *array, int count);
void qs(object_sort_key_t *array, int left, int right);
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

bool chunk_vertex_inside_plane(vector4_t vertex, frustrum_plane_t frustrum_plane);
uint32_t chunk_num_vertices_inside_plane(chunk_t *chunk, frustrum_plane_t frustrum_plane);
int get_chunk_intersection_mode(chunk_t *chunk);
void render_chunk(chunk_t *chunk);
void render_visible_chunks(void);
void draw_chunk_outlines(void);

void load_skybox();
void load_world1(void);
void load_voxel_world();
void load_server_room();
void load_quake_world();

#endif