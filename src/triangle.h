#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <stdint.h>
#include <stdbool.h>
#include "vector.h"
#include "light.h"
#include "color.h"
#include "display.h"
#include "material.h"
#include "misc.h"
#include "mesh.h"

typedef struct{
  vector4_t points[3];
  uv_t uv[3];
  float avg_w;
  uint32_t material_id; // triangle inherits either mesh material id, or object material override id
} triangle_t;

typedef struct{
  float alpha;
  float beta;
  float gamma;
} weight_t;

/* general */
void int_swap(int *a, int *b);
void float_swap(float *a, float *b);

/* triangle support */
vector3_t get_triangle_face_normal(vector4_t vertices[3]);
bool is_top_left_edge(vector2_t *start, vector2_t *end);
float edge_cross_old(vector2_t *a, vector2_t *b, vector2_t *p);
float edge_cross(
  const vector2_t *a,
  const vector2_t *b,
  const vector2_t *p
);
weight_t barycentric_weights(vector2_t A, vector2_t B, vector2_t C, vector2_t P);

/* drawing */
void draw_pixel(int x, int y, blend_mode_t blend_mode, uint8_t alpha_cutoff, float opacity, color_t color, float zbuffer_value);
void draw_pixel_absolute(int x, int y, color_t color);
void draw_pixel_alpha_test(int x, int y, uint8_t alpha_cutoff, color_t color, float zbuffer_value);
void draw_pixel_alpha_blend(int x, int y, float opacity, color_t color);
void draw_pixel_opaque(int x, int y, color_t color, float zbuffer_value);
void draw_rect(int x, int y, int width, int height, color_t color);
void draw_line(float x0, float y0, float x1, float y1, color_t color);
color_t apply_dither(color_t color, int x, int y);

/* triangles */
void draw_triangle(float x0, float y0, float x1, float y1, float x2, float y2, color_t color);

/* subpixel triangle rasterizers */
void draw_filled_triangle(
  vector4_t *v0, vector4_t *v1, vector4_t *v2, 
  color_t color, 
  light_intensity_rgb_t total_light_intensity,
  color_t emissive_color,
  bool reflects_light,
  bool dither
);
void draw_filled_triangle_no_zbuffer_write(
  vector4_t *v0, vector4_t *v1, vector4_t *v2, 
  color_t color, 
  light_intensity_rgb_t total_light_intensity,
  color_t emissive_color,
  bool reflects_light,
  bool dither
);

void draw_textured_triangle_opaque(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  light_intensity_rgb_t total_light_intensity,
  color_t emissive_color,
  bool reflects_light,
  bool dither
);
void draw_textured_triangle_opaque_float(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  light_intensity_rgb_t total_light_intensity,
  color_t emissive_color,
  bool reflects_light,
  bool dither
);
void draw_textured_triangle_opaque_no_zbuffer_write(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  light_intensity_rgb_t total_light_intensity,
  color_t emissive_color,
  bool reflects_light,
  bool dither
);
void draw_textured_triangle_alpha_test(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  light_intensity_rgb_t total_light_intensity,
  color_t emissive_color,
  bool reflects_light,
  bool dither,
  uint8_t alpha_cutoff
);
void draw_textured_triangle_alpha_test_no_zbuffer_write(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  light_intensity_rgb_t total_light_intensity,
  color_t emissive_color,
  bool reflects_light,
  bool dither,
  uint8_t alpha_cutoff
);
void draw_textured_triangle_alpha_blend(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  light_intensity_rgb_t total_light_intensity,
  color_t emissive_color,
  bool reflects_light,
  bool dither,
  float opacity
);

void draw_textured_triangle(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  light_intensity_rgb_t total_light_intensity,
  color_t emissive_color,
  bool reflects_light,
  bool dither,
  blend_mode_t blend_mode,
  uint8_t alpha_cutoff,
  float opacity
);

void draw_textured_triangle_affine(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  light_intensity_rgb_t total_light_intensity,
  bool dither,
  blend_mode_t blend_mode,
  uint8_t alpha_cutoff,
  float opacity
);

#endif