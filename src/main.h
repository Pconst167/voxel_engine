#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "vector.h"
#include "triangle.h"
#include "object.h"
#include "clipping.h"
#include "color.h"
#include "chunk.h"
#include "collision.h"

void render_text();
void free_resources(void);
void object_quicksort(object_sort_key_t *array, int count);
void qs(object_sort_key_t *array, int left, int right);
void chunk_quicksort(chunk_sort_key_t *array, int count);
void qs_chunk(chunk_sort_key_t *array, int left, int right);
void render_sun(void);
void update_state(void);
void move_player(void);
void update_light(void);
void process_input(void);
void setup(void) ;

inline void rasterize_triangle(
  triangle_t *triangle, 
  color_t *mesh_texture, 
  uint32_t texture_width, uint32_t texture_height, 
  blend_mode_t blend_mode,
  uint8_t alpha_cutoff,
  color_t base_color,
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
  color_t base_color,
  float opacity,
  bool reflective,
  color_t emissive_color,
  float emissive_intensity
);

bool chunk_vertex_inside_plane(vector4_t vertex, frustrum_plane_t frustrum_plane);
uint32_t chunk_num_vertices_inside_plane(chunk_t *chunk, frustrum_plane_t frustrum_plane);
int get_chunk_intersection_mode(chunk_t *chunk);
chunk_intersection_mode_t get_chunk_intersection_mode_fast(chunk_t *chunk);
void render_chunk(chunk_t *chunk);
void render_visible_chunks(void);
void draw_chunk_outlines(void);
void draw_collision_aabb_outlines(void);

bool resolve_player_against_aabb_xz(const collision_aabb_t *box);
void move_player_with_collision(vector3_t move);
void resolve_player_collisions();

void load_skybox();
void load_world1(void);
void load_voxel_world();
void load_server_room();
void load_quake_world();
void load_cave_world();

#endif