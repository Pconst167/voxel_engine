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

void generate_game_data();
void render_console();
void free_resources(void);
void object_quicksort(object_sort_key_t *array, int count);
void qs(object_sort_key_t *array, int left, int right);
void chunk_quicksort(chunk_sort_key_t *array, int count);
void qs_chunk(chunk_sort_key_t *array, int left, int right);
void triangle_quicksort(triangle_t *array, int count);
void qs_triangle(triangle_t *array, int left, int right);
void render_sun(void);
void update_state(void);
void move_player(void);
void update_light(void);
void process_input(void);
void process_console_input(SDL_Event *event);
void run_console_command();
void setup(void);

void draw_char_to_color_buffer(int x, int y, char character, color_t color, int scale);
void draw_text_to_color_buffer(int x, int y, const char *text, color_t color, int scale);

inline uint32_t rasterize_triangle(triangle_t *triangle);
inline uint32_t rasterize_triangle_no_zbuffer_write(triangle_t *triangle, material_t face_material, light_intensity_rgb_t total_light_intensity);

bool chunk_vertex_inside_plane(vector4_t vertex, frustrum_plane_t frustrum_plane);
uint32_t chunk_num_vertices_inside_plane(chunk_t *chunk, frustrum_plane_t frustrum_plane);
int get_chunk_intersection_mode(chunk_t *chunk);
chunk_intersection_mode_t get_chunk_intersection_mode_fast(chunk_t *chunk);
void render_chunk_objects(chunk_t *chunk);
void render_chunk_voxels(uint32_t z_row, uint32_t x_col);
void render_visible_chunks(void);
void render_visible_chunks_sorted(void);
void render_voxel_quad(voxel_quad_t voxel_quad, voxel_material_t *voxel_material);
void draw_chunk_outlines(void);
void draw_collision_aabb_outlines(void);

bool resolve_player_against_object_aabb_xz(const collision_aabb_t *box);
void resolve_player_against_object_collisions();

void move_player_with_voxel_collision(vector3_t movement);
void resolve_player_against_voxels_z(float movement_z);
void resolve_player_against_voxels_y(float movement_y);
void resolve_player_against_voxels_x(float movement_x);
void resolve_player_aabb_against_voxel_z(const aabb_3d_t *voxel_aabb, float movement_z);
void resolve_player_aabb_against_voxel_y(const aabb_3d_t *voxel_aabb, float movement_y);
void resolve_player_aabb_against_voxel_x(const aabb_3d_t *voxel_aabb, float movement_x);
void get_voxel_range_from_aabb(const aabb_3d_t *aabb, int *start_x, int *end_x, int *start_y, int *end_y, int *start_z, int *end_z);

void load_skybox();

#endif