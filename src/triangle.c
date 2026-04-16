#include <stdint.h>
#include "triangle.h"
#include "display.h"
#include "vector.h"
#include "light.h"

/*
  TODO:
  optimize all pixel drawing by passing a pointer instead of using array indexing:
  uint32_t *row = color_buffer + y * window_width;
  for (int x = x_start; x < x_end; x++) {
    row[x] = color;
  }
*/

__attribute__((optimize("Ofast")))
static inline uint8_t quantize_dither_u8(uint8_t c, int x, int y){
  static const uint8_t bayer4x4[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 }
  };

  /* 0..15 */
  int threshold = bayer4x4[(y) & 3][(x) & 3];

  /* 6-bit quantization with full-range ordered threshold */
  int q = (c * 31 + threshold * 5 ) >> 8;
  if(q > 31) q = 31;

  return (uint8_t)((q * 255) / 31);
}

__attribute__((optimize("Ofast")))
color_t temporal_dither(color_t color, int x, int y){
  return (color_t){
    .rgb.alpha = color.rgb.alpha,
    .rgb.blue  = quantize_dither_u8(color.rgb.blue,  x, y),
    .rgb.green = quantize_dither_u8(color.rgb.green, x, y),
    .rgb.red   = quantize_dither_u8(color.rgb.red,   x, y) 
  };

  return color;
}

inline void int_swap(int *a, int *b){
  int tmp = *b;
  *b = *a;
  *a = tmp;
}

inline void float_swap(float *a, float *b){
  float tmp = *b;
  *b = *a;
  *a = tmp;
}

/* top-left raster convention check */
__attribute__((optimize("Ofast")))
inline bool is_top_left_edge(vector2_t *start, vector2_t *end){
  float dx = end->x - start->x;
  float dy = end->y - start->y;

  return (dy < 0.0f) || (dy == 0.0f && dx > 0.0f);
}

/* top-left raster convention check */
__attribute__((optimize("Ofast")))
inline bool is_top_left_edge_old(vector2_t *start, vector2_t *end){
  vector2_t edge = {end->x - start->x, end->y - start->y};
  bool is_top_edge = edge.y == 0.0f && edge.x > 0.0f;
  bool is_left_edge = edge.y < 0.0f;

  return is_left_edge || is_top_edge;
}

__attribute__((optimize("Ofast")))
inline float edge_cross(
  const vector2_t *restrict a,
  const vector2_t *restrict b,
  const vector2_t *restrict p
){
  float ax = a->x;
  float ay = a->y;

  float abx = b->x - ax;
  float aby = b->y - ay;
  float apx = p->x - ax;
  float apy = p->y - ay;

  return abx * apy - aby * apx;
}

/* perform 2d edge-cross between 2 vertices and a point */
__attribute__((optimize("Ofast")))
float edge_cross_old(vector2_t *a, vector2_t *b, vector2_t *p){
  vector2_t ab = {b->x - a->x, b->y - a->y};
  vector2_t ap = {p->x - a->x, p->y - a->y};

  return ab.x * ap.y - ab.y * ap.x;
}

/* function argument is vector4_t instead of vector3_t for convenience(w is not used) */
__attribute__((optimize("Ofast")))
vector3_t get_triangle_face_normal(vector4_t vertices[3]){
  vector3_t v_a = {vertices[0].x, vertices[0].y, vertices[0].z};  /*   A               */
  vector3_t v_b = {vertices[1].x, vertices[1].y, vertices[1].z};  /*  / \    Clockwise */
  vector3_t v_c = {vertices[2].x, vertices[2].y, vertices[2].z};  /* C---B             */
  /* vectors for cross product */
  vector3_t v_ab = {.x = v_b.x - v_a.x, .y = v_b.y - v_a.y, .z = v_b.z - v_a.z};
  vector3_t v_ac = {.x = v_c.x - v_a.x, .y = v_c.y - v_a.y, .z = v_c.z - v_a.z};
  /* face normal
    cross product between v_ab and v_ac */
  vector3_t v_face_normal = {
    .x = v_ab.y * v_ac.z - v_ab.z * v_ac.y,
    .y = v_ab.z * v_ac.x - v_ab.x * v_ac.z,
    .z = v_ab.x * v_ac.y - v_ab.y * v_ac.x
  };
  vector3_normalize(&v_face_normal);

  return v_face_normal;
}

inline void draw_triangle(
  float x0, float y0, 
  float x1, float y1, 
  float x2, float y2, 
  color_t color
){
  draw_line(x0, y0, x1, y1, color);
  draw_line(x1, y1, x2, y2, color);
  draw_line(x2, y2, x0, y0, color);
}

__attribute__((optimize("Ofast")))
void draw_filled_triangle(
  vector4_t *v0,
  vector4_t *v1,
  vector4_t *v2,
  color_t color,
  float *light_intensities,
  color_t emissive_color,
  float emissive_intensity,
  bool reflective,
  bool dither
){
  float parallelogram_area = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y}
  );

  /* degenerate triangle */
  if(parallelogram_area == 0.0f) return;

  /* backface culling */
  //if(projection_options.backface_culling){
  //  if(parallelogram_area <= 0.0f) return;
  //}

  /* bounding box */
  int x_min = (int)floorf(MIN(MIN(v0->x, v1->x), v2->x));
  int y_min = (int)floorf(MIN(MIN(v0->y, v1->y), v2->y));
  int x_max = (int)ceilf (MAX(MAX(v0->x, v1->x), v2->x));
  int y_max = (int)ceilf (MAX(MAX(v0->y, v1->y), v2->y));

  /* if area is < 0, this means the triangle has opposite winding,
     hence here we swap two vertices to reverse the winding.  this is done so that the edge cross tests for points behaves normally
     again for reverse winded triangles.  because otherwise, whenever the winding is reversed, the edge cross function will always return negative.
     also, negate area instead of recomputing it for the corrected winding.
  */
  if(parallelogram_area < 0.0f){
    vector4_t *tmp_v = v1;
    v1 = v2;
    v2 = tmp_v;

    parallelogram_area = -parallelogram_area;
  }

  /* top-left rule flags for each edge
     edge0 is opposite vertices v1 and v2
     edge1 is opposite vertices v0 and v2
     edge2 is opposite vertices v0 and v1
  */
  bool edge0_top_left = is_top_left_edge(&(vector2_t){v1->x, v1->y}, &(vector2_t){v2->x, v2->y});
  bool edge1_top_left = is_top_left_edge(&(vector2_t){v2->x, v2->y}, &(vector2_t){v0->x, v0->y});
  bool edge2_top_left = is_top_left_edge(&(vector2_t){v0->x, v0->y}, &(vector2_t){v1->x, v1->y});

  /* edge deltas */
  float delta_w0_col = v1->y - v2->y;
  float delta_w1_col = v2->y - v0->y;
  float delta_w2_col = v0->y - v1->y;

  float delta_w0_row = v2->x - v1->x;
  float delta_w1_row = v0->x - v2->x;
  float delta_w2_row = v1->x - v0->x;

  /* edge function at first pixel center */
  vector2_t p = {x_min + 0.5f, y_min + 0.5f};

  float w0_row = edge_cross(
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y},
    &p
  );

  float w1_row = edge_cross(
    &(vector2_t){v2->x, v2->y},
    &(vector2_t){v0->x, v0->y},
    &p
  );

  float w2_row = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &p
  );

  float one_over_parallelogram_area = 1.0f / parallelogram_area;

  /* find the light intensities once per triangle */
  light_intensity_rgb_t intensity = find_total_light_intensity(light_intensities);

  for(int y = y_min; y <= y_max; y++){
    float w0 = w0_row;
    float w1 = w1_row;
    float w2 = w2_row;

    bool was_inside = false;
    for(int x = x_min; x <= x_max; x++){
      /* pixel is inside if w's are positive (point is inside triangle) or point is on a top/left edge(any of the 3 edges could be the top/left edge so we test all) */
      bool inside = (w0 > 0.0f || (w0 == 0.0f && edge0_top_left)) &&
                    (w1 > 0.0f || (w1 == 0.0f && edge1_top_left)) &&
                    (w2 > 0.0f || (w2 == 0.0f && edge2_top_left));

      if(inside){
        was_inside = true;
        float alpha = w0 * one_over_parallelogram_area;
        float beta  = w1 * one_over_parallelogram_area;
        float gamma = w2 * one_over_parallelogram_area;

        float interpolated_reciprocal_w = alpha * (1.0f / v0->w) + beta  * (1.0f / v1->w) + gamma * (1.0f / v2->w);

        if(interpolated_reciprocal_w > zbuffer[y * window_width + x]){
          uint32_t red_total   = 0;
          uint32_t green_total = 0;
          uint32_t blue_total  = 0;

          if(reflective){
            red_total   = (uint32_t)(color.rgb.red   * intensity.red  );
            green_total = (uint32_t)(color.rgb.green * intensity.green);
            blue_total  = (uint32_t)(color.rgb.blue  * intensity.blue );
          }

          /* add emissive color on top */
          if(emissive_intensity > 0.0f){
            red_total   += emissive_color.rgb.red   * emissive_intensity;
            green_total += emissive_color.rgb.green * emissive_intensity;
            blue_total  += emissive_color.rgb.blue  * emissive_intensity;
          }

          /* clamp total amount as results over 255 do not make sense */
          if(red_total   > 255) red_total   = 255;
          if(green_total > 255) green_total = 255;
          if(blue_total  > 255) blue_total  = 255;
          color_t final_color = {
            .rgb.alpha = color.rgb.alpha,
            .rgb.blue  = blue_total,
            .rgb.green = green_total,
            .rgb.red   = red_total
          };
          if(dither) final_color = temporal_dither(final_color, x, y);

          if((unsigned)x < (unsigned)window_width && (unsigned)y < (unsigned)window_height){
            color_buffer[y * window_width + x] = final_color;
            zbuffer[y * window_width + x] = interpolated_reciprocal_w;
          }
        }
      }
      else if(was_inside) break;

      w0 += delta_w0_col;
      w1 += delta_w1_col;
      w2 += delta_w2_col;
    }

    w0_row += delta_w0_row;
    w1_row += delta_w1_row;
    w2_row += delta_w2_row;
  }

  if(projection_options.wireframe_overlay){
    draw_triangle(v0->x, v0->y, v1->x, v1->y, v2->x, v2->y, (color_t){.uint32 = 0xFFFFFFFF});
  }
}

__attribute__((optimize("Ofast")))
void draw_filled_triangle_no_zbuffer_write(
  vector4_t *v0,
  vector4_t *v1,
  vector4_t *v2,
  color_t color,
  float *light_intensities,
  color_t emissive_color,
  float emissive_intensity,
  bool reflective,
  bool dither
){
  float parallelogram_area = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y}
  );

  /* degenerate triangle */
  if(parallelogram_area == 0.0f) return;

  /* backface culling */
  //if(projection_options.backface_culling){
  //  if(parallelogram_area <= 0.0f) return;
  //}

  /* bounding box */
  int x_min = (int)floorf(MIN(MIN(v0->x, v1->x), v2->x));
  int y_min = (int)floorf(MIN(MIN(v0->y, v1->y), v2->y));
  int x_max = (int)ceilf (MAX(MAX(v0->x, v1->x), v2->x));
  int y_max = (int)ceilf (MAX(MAX(v0->y, v1->y), v2->y));

  /* if area is < 0, this means the triangle has opposite winding,
     hence here we swap two vertices to reverse the winding.  this is done so that the edge cross tests for points behaves normally
     again for reverse winded triangles.  because otherwise, whenever the winding is reversed, the edge cross function will always return negative.
     also, negate area instead of recomputing it for the corrected winding.
  */
  if(parallelogram_area < 0.0f){
    vector4_t *tmp_v = v1;
    v1 = v2;
    v2 = tmp_v;

    parallelogram_area = -parallelogram_area;
  }

  /* top-left rule flags for each edge
     edge0 is opposite vertices v1 and v2
     edge1 is opposite vertices v0 and v2
     edge2 is opposite vertices v0 and v1
  */
  bool edge0_top_left = is_top_left_edge(&(vector2_t){v1->x, v1->y}, &(vector2_t){v2->x, v2->y});
  bool edge1_top_left = is_top_left_edge(&(vector2_t){v2->x, v2->y}, &(vector2_t){v0->x, v0->y});
  bool edge2_top_left = is_top_left_edge(&(vector2_t){v0->x, v0->y}, &(vector2_t){v1->x, v1->y});

  /* edge deltas */
  float delta_w0_col = v1->y - v2->y;
  float delta_w1_col = v2->y - v0->y;
  float delta_w2_col = v0->y - v1->y;

  float delta_w0_row = v2->x - v1->x;
  float delta_w1_row = v0->x - v2->x;
  float delta_w2_row = v1->x - v0->x;

  /* edge function at first pixel center */
  vector2_t p = {x_min + 0.5f, y_min + 0.5f};

  float w0_row = edge_cross(
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y},
    &p
  );

  float w1_row = edge_cross(
    &(vector2_t){v2->x, v2->y},
    &(vector2_t){v0->x, v0->y},
    &p
  );

  float w2_row = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &p
  );

  float one_over_parallelogram_area = 1.0f / parallelogram_area;

  /* find the light intensities once per triangle */
  light_intensity_rgb_t intensity = find_total_light_intensity(light_intensities);

  for(int y = y_min; y <= y_max; y++){
    float w0 = w0_row;
    float w1 = w1_row;
    float w2 = w2_row;

    bool was_inside = false;
    for(int x = x_min; x <= x_max; x++){
      /* pixel is inside if w's are positive (point is inside triangle) or point is on a top/left edge(any of the 3 edges could be the top/left edge so we test all) */
      bool inside = (w0 > 0.0f || (w0 == 0.0f && edge0_top_left)) &&
                    (w1 > 0.0f || (w1 == 0.0f && edge1_top_left)) &&
                    (w2 > 0.0f || (w2 == 0.0f && edge2_top_left));

      if(inside){
        was_inside = true;
        float alpha = w0 * one_over_parallelogram_area;
        float beta  = w1 * one_over_parallelogram_area;
        float gamma = w2 * one_over_parallelogram_area;

        float interpolated_reciprocal_w = alpha * (1.0f / v0->w) + beta  * (1.0f / v1->w) + gamma * (1.0f / v2->w);

        if(interpolated_reciprocal_w > zbuffer[y * window_width + x]){
          uint32_t red_total   = 0;
          uint32_t green_total = 0;
          uint32_t blue_total  = 0;

          if(reflective){
            red_total   = (uint32_t)(color.rgb.red   * intensity.red  );
            green_total = (uint32_t)(color.rgb.green * intensity.green);
            blue_total  = (uint32_t)(color.rgb.blue  * intensity.blue );
          }

          /* add emissive color on top */
          if(emissive_intensity > 0.0f){
            red_total   += emissive_color.rgb.red   * emissive_intensity;
            green_total += emissive_color.rgb.green * emissive_intensity;
            blue_total  += emissive_color.rgb.blue  * emissive_intensity;
          }

          /* clamp total amount as results over 255 do not make sense */
          if(red_total   > 255) red_total   = 255;
          if(green_total > 255) green_total = 255;
          if(blue_total  > 255) blue_total  = 255;
          color_t final_color = {
            .rgb.alpha = color.rgb.alpha,
            .rgb.blue  = blue_total,
            .rgb.green = green_total,
            .rgb.red   = red_total
          };
          if(dither) final_color = temporal_dither(final_color, x, y);

          if((unsigned)x < (unsigned)window_width && (unsigned)y < (unsigned)window_height)
            color_buffer[y * window_width + x] = final_color;
        }
      }
      else if(was_inside) break;

      w0 += delta_w0_col;
      w1 += delta_w1_col;
      w2 += delta_w2_col;
    }

    w0_row += delta_w0_row;
    w1_row += delta_w1_row;
    w2_row += delta_w2_row;
  }

  if(projection_options.wireframe_overlay){
    draw_triangle(v0->x, v0->y, v1->x, v1->y, v2->x, v2->y, (color_t){.uint32 = 0xFFFFFFFF});
  }
}

__attribute__((optimize("Ofast")))
void draw_textured_triangle_opaque(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  float *light_intensities,
  color_t emissive_color,
  float emissive_intensity,
  bool reflective,
  bool dither
){
  float parallelogram_area = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y}
  );

  /* degenerate triangle */
  if(parallelogram_area == 0.0f) return;

  /* backface culling */
  //if(projection_options.backface_culling){
  //  if(parallelogram_area <= 0.0f) return;
  //}

  /* bounding box */
  int x_min = (int)floorf(MIN(MIN(v0->x, v1->x), v2->x));
  int y_min = (int)floorf(MIN(MIN(v0->y, v1->y), v2->y));
  int x_max = (int)ceilf (MAX(MAX(v0->x, v1->x), v2->x));
  int y_max = (int)ceilf (MAX(MAX(v0->y, v1->y), v2->y));

  /* if area is < 0, this means the triangle has opposite winding,
     hence here we swap two vertices to reverse the winding.  this is done so that the edge cross tests for points behaves normally
     again for reverse winded triangles.  because otherwise, whenever the winding is reversed, the edge cross function will always return negative.
     also, negate area instead of recomputing it for the corrected winding.
  */
  if(parallelogram_area < 0.0f){
    vector4_t *tmp_v = v1;
    v1 = v2;
    v2 = tmp_v;

    uv_t tmp_uv = v1_uv;
    v1_uv = v2_uv;
    v2_uv = tmp_uv;

    parallelogram_area = -parallelogram_area;
  }

  /* top-left rule flags for each edge
     edge0 is opposite vertices v1 and v2
     edge1 is opposite vertices v0 and v2
     edge2 is opposite vertices v0 and v1
  */
  bool edge0_top_left = is_top_left_edge(&(vector2_t){v1->x, v1->y}, &(vector2_t){v2->x, v2->y});
  bool edge1_top_left = is_top_left_edge(&(vector2_t){v2->x, v2->y}, &(vector2_t){v0->x, v0->y});
  bool edge2_top_left = is_top_left_edge(&(vector2_t){v0->x, v0->y}, &(vector2_t){v1->x, v1->y});

  /* edge deltas */
  float delta_w0_col = v1->y - v2->y;
  float delta_w1_col = v2->y - v0->y;
  float delta_w2_col = v0->y - v1->y;

  float delta_w0_row = v2->x - v1->x;
  float delta_w1_row = v0->x - v2->x;
  float delta_w2_row = v1->x - v0->x;

  /* edge function at first pixel center */
  vector2_t p = {x_min + 0.5f, y_min + 0.5f};

  float w0_row = edge_cross(
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y},
    &p
  );

  float w1_row = edge_cross(
    &(vector2_t){v2->x, v2->y},
    &(vector2_t){v0->x, v0->y},
    &p
  );

  float w2_row = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &p
  );

  /* pre-compute the barycentric interpolation cordinates here instead of inside loops */
  float one_over_v0_w = 1.0f / v0->w;
  float one_over_v1_w = 1.0f / v1->w;
  float one_over_v2_w = 1.0f / v2->w;
  float v0_u_over_v0_w = v0_uv.u * one_over_v0_w;
  float v1_u_over_v1_w = v1_uv.u * one_over_v1_w;
  float v2_u_over_v2_w = v2_uv.u * one_over_v2_w;
  float v0_v_over_v0_w = v0_uv.v * one_over_v0_w;
  float v1_v_over_v1_w = v1_uv.v * one_over_v1_w;
  float v2_v_over_v2_w = v2_uv.v * one_over_v2_w;
  float one_over_parallelogram_area = 1.0f / parallelogram_area;

  /* find the light intensities once per triangle */
  light_intensity_rgb_t intensity = find_total_light_intensity(light_intensities);

  for(int y = y_min; y <= y_max; y++){
    float w0 = w0_row;
    float w1 = w1_row;
    float w2 = w2_row;

    bool was_inside = false; /* if we were inside, and then we are no longer inside, then exit row loop early */
    for(int x = x_min; x <= x_max; x++){
      /* pixel is inside if w's are positive (point is inside triangle) or point is on a top/left edge(any of the 3 edges could be the top/left edge so we test all) */
      bool inside = (w0 > 0.0f || (w0 == 0.0f && edge0_top_left)) &&
                    (w1 > 0.0f || (w1 == 0.0f && edge1_top_left)) &&
                    (w2 > 0.0f || (w2 == 0.0f && edge2_top_left));

      if(inside){
        was_inside = true;
        float alpha = w0 * one_over_parallelogram_area;
        float beta  = w1 * one_over_parallelogram_area;
        float gamma = w2 * one_over_parallelogram_area;

        float interpolated_reciprocal_w = alpha * one_over_v0_w + beta * one_over_v1_w + gamma * one_over_v2_w;

        if(interpolated_reciprocal_w > zbuffer[y * window_width + x]){
          float interpolated_u = alpha * v0_u_over_v0_w + beta  * v1_u_over_v1_w + gamma * v2_u_over_v2_w;
          float interpolated_v = alpha * v0_v_over_v0_w + beta  * v1_v_over_v1_w + gamma * v2_v_over_v2_w;

          interpolated_u /= interpolated_reciprocal_w;
          interpolated_v /= interpolated_reciprocal_w;

          int texture_u = (int)(interpolated_u * texture_width);
          int texture_v = (int)(interpolated_v * texture_height);

          if(texture_u < 0) texture_u = 0;
          if(texture_v < 0) texture_v = 0;
          if(texture_u >= texture_width)  texture_u = texture_width  - 1;
          if(texture_v >= texture_height) texture_v = texture_height - 1;

          color_t texel = texture[texture_v * texture_width + texture_u];
          
          uint32_t red_total   = 0;
          uint32_t green_total = 0;
          uint32_t blue_total  = 0;

          if(reflective){
            red_total   = (uint32_t)(texel.rgb.red   * intensity.red  );
            green_total = (uint32_t)(texel.rgb.green * intensity.green);
            blue_total  = (uint32_t)(texel.rgb.blue  * intensity.blue );
          }

          /* add emissive color on top */
          if(emissive_intensity > 0.0f){
            red_total   += texel.rgb.red   * emissive_color.rgb.red   / 255.0f * emissive_intensity;
            green_total += texel.rgb.green * emissive_color.rgb.green / 255.0f * emissive_intensity;
            blue_total  += texel.rgb.blue  * emissive_color.rgb.blue  / 255.0f * emissive_intensity;
          }

          /* clamp total amount as results over 255 do not make sense */
          if(red_total   > 255) red_total   = 255;
          if(green_total > 255) green_total = 255;
          if(blue_total  > 255) blue_total  = 255;
          color_t final_color = {
            .rgb.alpha = texel.rgb.alpha,
            .rgb.blue  = blue_total,
            .rgb.green = green_total,
            .rgb.red   = red_total
          };
          if(dither) final_color = temporal_dither(final_color, x, y);

          /* draw pixel */
          if((unsigned)x < (unsigned)window_width && (unsigned)y < (unsigned)window_height){
            color_buffer[y * window_width + x] = final_color;
            zbuffer[y * window_width + x] = interpolated_reciprocal_w;
          }
        }
      }
      else if(was_inside) break;

      w0 += delta_w0_col;
      w1 += delta_w1_col;
      w2 += delta_w2_col;
    }

    w0_row += delta_w0_row;
    w1_row += delta_w1_row;
    w2_row += delta_w2_row;
  }

  if(projection_options.wireframe_overlay){
    draw_triangle(v0->x, v0->y, v1->x, v1->y, v2->x, v2->y, (color_t){.uint32 = 0xFFFFFFFF});
  }
}

__attribute__((optimize("Ofast")))
void draw_textured_triangle_opaque_no_zbuffer_write(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  float *light_intensities,
  color_t emissive_color,
  float emissive_intensity,
  bool reflective,
  bool dither
){
  float parallelogram_area = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y}
  );

  /* degenerate triangle */
  if(parallelogram_area == 0.0f) return;

  /* backface culling */
  //if(projection_options.backface_culling){
  //  if(parallelogram_area <= 0.0f) return;
  //}

  /* bounding box */
  int x_min = (int)floorf(MIN(MIN(v0->x, v1->x), v2->x));
  int y_min = (int)floorf(MIN(MIN(v0->y, v1->y), v2->y));
  int x_max = (int)ceilf (MAX(MAX(v0->x, v1->x), v2->x));
  int y_max = (int)ceilf (MAX(MAX(v0->y, v1->y), v2->y));

  /* if area is < 0, this means the triangle has opposite winding,
     hence here we swap two vertices to reverse the winding.  this is done so that the edge cross tests for points behaves normally
     again for reverse winded triangles.  because otherwise, whenever the winding is reversed, the edge cross function will always return negative.
     also, negate area instead of recomputing it for the corrected winding.
  */
  if(parallelogram_area < 0.0f){
    vector4_t *tmp_v = v1;
    v1 = v2;
    v2 = tmp_v;

    uv_t tmp_uv = v1_uv;
    v1_uv = v2_uv;
    v2_uv = tmp_uv;

    parallelogram_area = -parallelogram_area;
  }

  /* top-left rule flags for each edge
     edge0 is opposite vertices v1 and v2
     edge1 is opposite vertices v0 and v2
     edge2 is opposite vertices v0 and v1
  */
  bool edge0_top_left = is_top_left_edge(&(vector2_t){v1->x, v1->y}, &(vector2_t){v2->x, v2->y});
  bool edge1_top_left = is_top_left_edge(&(vector2_t){v2->x, v2->y}, &(vector2_t){v0->x, v0->y});
  bool edge2_top_left = is_top_left_edge(&(vector2_t){v0->x, v0->y}, &(vector2_t){v1->x, v1->y});

  /* edge deltas */
  float delta_w0_col = v1->y - v2->y;
  float delta_w1_col = v2->y - v0->y;
  float delta_w2_col = v0->y - v1->y;

  float delta_w0_row = v2->x - v1->x;
  float delta_w1_row = v0->x - v2->x;
  float delta_w2_row = v1->x - v0->x;

  /* edge function at first pixel center */
  vector2_t p = {x_min + 0.5f, y_min + 0.5f};

  float w0_row = edge_cross(
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y},
    &p
  );

  float w1_row = edge_cross(
    &(vector2_t){v2->x, v2->y},
    &(vector2_t){v0->x, v0->y},
    &p
  );

  float w2_row = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &p
  );

  /* pre-compute the barycentric interpolation cordinates here instead of inside loops */
  float one_over_v0_w = 1.0f / v0->w;
  float one_over_v1_w = 1.0f / v1->w;
  float one_over_v2_w = 1.0f / v2->w;
  float v0_u_over_v0_w = v0_uv.u * one_over_v0_w;
  float v1_u_over_v1_w = v1_uv.u * one_over_v1_w;
  float v2_u_over_v2_w = v2_uv.u * one_over_v2_w;
  float v0_v_over_v0_w = v0_uv.v * one_over_v0_w;
  float v1_v_over_v1_w = v1_uv.v * one_over_v1_w;
  float v2_v_over_v2_w = v2_uv.v * one_over_v2_w;
  float one_over_parallelogram_area = 1.0f / parallelogram_area;

  /* find the light intensities once per triangle */
  light_intensity_rgb_t intensity = find_total_light_intensity(light_intensities);

  for(int y = y_min; y <= y_max; y++){
    float w0 = w0_row;
    float w1 = w1_row;
    float w2 = w2_row;

    bool was_inside = false; /* if we were inside, and then we are no longer inside, then exit row loop early */
    for(int x = x_min; x <= x_max; x++){
      /* pixel is inside if w's are positive (point is inside triangle) or point is on a top/left edge(any of the 3 edges could be the top/left edge so we test all) */
      bool inside = (w0 > 0.0f || (w0 == 0.0f && edge0_top_left)) &&
                    (w1 > 0.0f || (w1 == 0.0f && edge1_top_left)) &&
                    (w2 > 0.0f || (w2 == 0.0f && edge2_top_left));

      if(inside){
        was_inside = true;
        float alpha = w0 * one_over_parallelogram_area;
        float beta  = w1 * one_over_parallelogram_area;
        float gamma = w2 * one_over_parallelogram_area;

        float interpolated_reciprocal_w = alpha * one_over_v0_w + beta * one_over_v1_w + gamma * one_over_v2_w;

        if(zbuffer[y * window_width + x] == 0.0f){
          float interpolated_u = alpha * v0_u_over_v0_w + beta  * v1_u_over_v1_w + gamma * v2_u_over_v2_w;
          float interpolated_v = alpha * v0_v_over_v0_w + beta  * v1_v_over_v1_w + gamma * v2_v_over_v2_w;

          interpolated_u /= interpolated_reciprocal_w;
          interpolated_v /= interpolated_reciprocal_w;

          int texture_u = (int)(interpolated_u * texture_width);
          int texture_v = (int)(interpolated_v * texture_height);

          if(texture_u < 0) texture_u = 0;
          if(texture_v < 0) texture_v = 0;
          if(texture_u >= texture_width)  texture_u = texture_width  - 1;
          if(texture_v >= texture_height) texture_v = texture_height - 1;

          color_t texel = texture[texture_v * texture_width + texture_u];
          
          uint32_t red_total   = 0;
          uint32_t green_total = 0;
          uint32_t blue_total  = 0;

          if(reflective){
            red_total   = (uint32_t)(texel.rgb.red   * intensity.red  );
            green_total = (uint32_t)(texel.rgb.green * intensity.green);
            blue_total  = (uint32_t)(texel.rgb.blue  * intensity.blue );
          }

          /* add emissive color on top */
          if(emissive_intensity > 0.0f){
            red_total   += texel.rgb.red   * emissive_color.rgb.red   / 255.0f * emissive_intensity;
            green_total += texel.rgb.green * emissive_color.rgb.green / 255.0f * emissive_intensity;
            blue_total  += texel.rgb.blue  * emissive_color.rgb.blue  / 255.0f * emissive_intensity;
          }

          /* clamp total amount as results over 255 do not make sense */
          if(red_total   > 255) red_total   = 255;
          if(green_total > 255) green_total = 255;
          if(blue_total  > 255) blue_total  = 255;
          color_t final_color = {
            .rgb.alpha = texel.rgb.alpha,
            .rgb.blue  = blue_total,
            .rgb.green = green_total,
            .rgb.red   = red_total
          };
          if(dither) final_color = temporal_dither(final_color, x, y);

          /* draw pixel */
          if((unsigned)x < (unsigned)window_width && (unsigned)y < (unsigned)window_height)
            color_buffer[y * window_width + x] = final_color;
        }
      }
      else if(was_inside) break;

      w0 += delta_w0_col;
      w1 += delta_w1_col;
      w2 += delta_w2_col;
    }

    w0_row += delta_w0_row;
    w1_row += delta_w1_row;
    w2_row += delta_w2_row;
  }

  if(projection_options.wireframe_overlay){
    draw_triangle(v0->x, v0->y, v1->x, v1->y, v2->x, v2->y, (color_t){.uint32 = 0xFFFFFFFF});
  }
}

__attribute__((optimize("Ofast")))
void draw_textured_triangle_alpha_test(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  float *light_intensities,
  color_t emissive_color,
  float emissive_intensity,
  bool reflective,
  bool dither,
  uint8_t alpha_cutoff
){
  float parallelogram_area = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y}
  );

  /* degenerate triangle */
  if(parallelogram_area == 0.0f) return;

  /* backface culling */
  //if(projection_options.backface_culling){
  //  if(parallelogram_area <= 0.0f) return;
  //}

  /* bounding box */
  int x_min = (int)floorf(MIN(MIN(v0->x, v1->x), v2->x));
  int y_min = (int)floorf(MIN(MIN(v0->y, v1->y), v2->y));
  int x_max = (int)ceilf (MAX(MAX(v0->x, v1->x), v2->x));
  int y_max = (int)ceilf (MAX(MAX(v0->y, v1->y), v2->y));

  /* if area is < 0, this means the triangle has opposite winding,
     hence here we swap two vertices to reverse the winding.  this is done so that the edge cross tests for points behaves normally
     again for reverse winded triangles.  because otherwise, whenever the winding is reversed, the edge cross function will always return negative.
     also, negate area instead of recomputing it for the corrected winding.
  */
  if(parallelogram_area < 0.0f){
    vector4_t *tmp_v = v1;
    v1 = v2;
    v2 = tmp_v;

    uv_t tmp_uv = v1_uv;
    v1_uv = v2_uv;
    v2_uv = tmp_uv;

    parallelogram_area = -parallelogram_area;
  }

  /* top-left rule flags for each edge
     edge0 is opposite vertices v1 and v2
     edge1 is opposite vertices v0 and v2
     edge2 is opposite vertices v0 and v1
  */
  bool edge0_top_left = is_top_left_edge(&(vector2_t){v1->x, v1->y}, &(vector2_t){v2->x, v2->y});
  bool edge1_top_left = is_top_left_edge(&(vector2_t){v2->x, v2->y}, &(vector2_t){v0->x, v0->y});
  bool edge2_top_left = is_top_left_edge(&(vector2_t){v0->x, v0->y}, &(vector2_t){v1->x, v1->y});

  /* edge deltas */
  float delta_w0_col = v1->y - v2->y;
  float delta_w1_col = v2->y - v0->y;
  float delta_w2_col = v0->y - v1->y;

  float delta_w0_row = v2->x - v1->x;
  float delta_w1_row = v0->x - v2->x;
  float delta_w2_row = v1->x - v0->x;

  /* edge function at first pixel center */
  vector2_t p = {x_min + 0.5f, y_min + 0.5f};

  float w0_row = edge_cross(
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y},
    &p
  );

  float w1_row = edge_cross(
    &(vector2_t){v2->x, v2->y},
    &(vector2_t){v0->x, v0->y},
    &p
  );

  float w2_row = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &p
  );

  /* pre-compute the barycentric interpolation cordinates here instead of inside loops */
  float one_over_v0_w = 1.0f / v0->w;
  float one_over_v1_w = 1.0f / v1->w;
  float one_over_v2_w = 1.0f / v2->w;
  float v0_u_over_v0_w = v0_uv.u * one_over_v0_w;
  float v1_u_over_v1_w = v1_uv.u * one_over_v1_w;
  float v2_u_over_v2_w = v2_uv.u * one_over_v2_w;
  float v0_v_over_v0_w = v0_uv.v * one_over_v0_w;
  float v1_v_over_v1_w = v1_uv.v * one_over_v1_w;
  float v2_v_over_v2_w = v2_uv.v * one_over_v2_w;
  float one_over_parallelogram_area = 1.0f / parallelogram_area;

  /* find the light intensities once per triangle */
  light_intensity_rgb_t intensity = find_total_light_intensity(light_intensities);

  for(int y = y_min; y <= y_max; y++){
    float w0 = w0_row;
    float w1 = w1_row;
    float w2 = w2_row;

    bool was_inside = false;
    for(int x = x_min; x <= x_max; x++){
      /* pixel is inside if w's are positive (point is inside triangle) or point is on a top/left edge(any of the 3 edges could be the top/left edge so we test all) */
      bool inside = (w0 > 0.0f || (w0 == 0.0f && edge0_top_left)) &&
                    (w1 > 0.0f || (w1 == 0.0f && edge1_top_left)) &&
                    (w2 > 0.0f || (w2 == 0.0f && edge2_top_left));

      if(inside){
        was_inside = true;
        float alpha = w0 * one_over_parallelogram_area;
        float beta  = w1 * one_over_parallelogram_area;
        float gamma = w2 * one_over_parallelogram_area;

        float interpolated_reciprocal_w = alpha * one_over_v0_w + beta * one_over_v1_w + gamma * one_over_v2_w;

        if(interpolated_reciprocal_w > zbuffer[y * window_width + x]){
          float interpolated_u = alpha * v0_u_over_v0_w + beta  * v1_u_over_v1_w + gamma * v2_u_over_v2_w;
          float interpolated_v = alpha * v0_v_over_v0_w + beta  * v1_v_over_v1_w + gamma * v2_v_over_v2_w;

          interpolated_u /= interpolated_reciprocal_w;
          interpolated_v /= interpolated_reciprocal_w;

          int texture_u = (int)(interpolated_u * texture_width);
          int texture_v = (int)(interpolated_v * texture_height);

          if(texture_u < 0) texture_u = 0;
          if(texture_v < 0) texture_v = 0;
          if(texture_u >= texture_width)  texture_u = texture_width  - 1;
          if(texture_v >= texture_height) texture_v = texture_height - 1;

          color_t texel = texture[texture_v * texture_width + texture_u];
          
          if(texel.rgb.alpha >= alpha_cutoff){
            uint32_t red_total   = 0;
            uint32_t green_total = 0;
            uint32_t blue_total  = 0;

            if(reflective){
              red_total   = (uint32_t)(texel.rgb.red   * intensity.red  );
              green_total = (uint32_t)(texel.rgb.green * intensity.green);
              blue_total  = (uint32_t)(texel.rgb.blue  * intensity.blue );
            }

            /* add emissive color on top */
            if(emissive_intensity > 0.0f){
              red_total   += texel.rgb.red   * emissive_color.rgb.red   / 255.0f * emissive_intensity;
              green_total += texel.rgb.green * emissive_color.rgb.green / 255.0f * emissive_intensity;
              blue_total  += texel.rgb.blue  * emissive_color.rgb.blue  / 255.0f * emissive_intensity;
            }

            /* clamp total amount as results over 255 do not make sense */
            if(red_total   > 255) red_total   = 255;
            if(green_total > 255) green_total = 255;
            if(blue_total  > 255) blue_total  = 255;
            color_t final_color = {
              .rgb.alpha = texel.rgb.alpha,
              .rgb.blue  = blue_total,
              .rgb.green = green_total,
              .rgb.red   = red_total
            };
            if(dither) final_color = temporal_dither(final_color, x, y);

            if((unsigned)x < (unsigned)window_width && (unsigned)y < (unsigned)window_height){
              color_buffer[y * window_width + x] = final_color;
              zbuffer[y * window_width + x] = interpolated_reciprocal_w;
            }
          }
        }
      }
      else if(was_inside) break;

      w0 += delta_w0_col;
      w1 += delta_w1_col;
      w2 += delta_w2_col;
    }

    w0_row += delta_w0_row;
    w1_row += delta_w1_row;
    w2_row += delta_w2_row;
  }

  if(projection_options.wireframe_overlay){
    draw_triangle(v0->x, v0->y, v1->x, v1->y, v2->x, v2->y, (color_t){.uint32 = 0xFFFFFFFF});
  }
}

__attribute__((optimize("Ofast")))
void draw_textured_triangle_alpha_test_no_zbuffer_write(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  float *light_intensities,
  color_t emissive_color,
  float emissive_intensity,
  bool reflective,
  bool dither,
  uint8_t alpha_cutoff
){
  float parallelogram_area = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y}
  );

  /* degenerate triangle */
  if(parallelogram_area == 0.0f) return;

  /* backface culling */
  //if(projection_options.backface_culling){
  //  if(parallelogram_area <= 0.0f) return;
  //}

  /* bounding box */
  int x_min = (int)floorf(MIN(MIN(v0->x, v1->x), v2->x));
  int y_min = (int)floorf(MIN(MIN(v0->y, v1->y), v2->y));
  int x_max = (int)ceilf (MAX(MAX(v0->x, v1->x), v2->x));
  int y_max = (int)ceilf (MAX(MAX(v0->y, v1->y), v2->y));

  /* if area is < 0, this means the triangle has opposite winding,
     hence here we swap two vertices to reverse the winding.  this is done so that the edge cross tests for points behaves normally
     again for reverse winded triangles.  because otherwise, whenever the winding is reversed, the edge cross function will always return negative.
     also, negate area instead of recomputing it for the corrected winding.
  */
  if(parallelogram_area < 0.0f){
    vector4_t *tmp_v = v1;
    v1 = v2;
    v2 = tmp_v;

    uv_t tmp_uv = v1_uv;
    v1_uv = v2_uv;
    v2_uv = tmp_uv;

    parallelogram_area = -parallelogram_area;
  }

  /* top-left rule flags for each edge
     edge0 is opposite vertices v1 and v2
     edge1 is opposite vertices v0 and v2
     edge2 is opposite vertices v0 and v1
  */
  bool edge0_top_left = is_top_left_edge(&(vector2_t){v1->x, v1->y}, &(vector2_t){v2->x, v2->y});
  bool edge1_top_left = is_top_left_edge(&(vector2_t){v2->x, v2->y}, &(vector2_t){v0->x, v0->y});
  bool edge2_top_left = is_top_left_edge(&(vector2_t){v0->x, v0->y}, &(vector2_t){v1->x, v1->y});

  /* edge deltas */
  float delta_w0_col = v1->y - v2->y;
  float delta_w1_col = v2->y - v0->y;
  float delta_w2_col = v0->y - v1->y;

  float delta_w0_row = v2->x - v1->x;
  float delta_w1_row = v0->x - v2->x;
  float delta_w2_row = v1->x - v0->x;

  /* edge function at first pixel center */
  vector2_t p = {x_min + 0.5f, y_min + 0.5f};

  float w0_row = edge_cross(
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y},
    &p
  );

  float w1_row = edge_cross(
    &(vector2_t){v2->x, v2->y},
    &(vector2_t){v0->x, v0->y},
    &p
  );

  float w2_row = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &p
  );

  /* pre-compute the barycentric interpolation cordinates here instead of inside loops */
  float one_over_v0_w = 1.0f / v0->w;
  float one_over_v1_w = 1.0f / v1->w;
  float one_over_v2_w = 1.0f / v2->w;
  float v0_u_over_v0_w = v0_uv.u * one_over_v0_w;
  float v1_u_over_v1_w = v1_uv.u * one_over_v1_w;
  float v2_u_over_v2_w = v2_uv.u * one_over_v2_w;
  float v0_v_over_v0_w = v0_uv.v * one_over_v0_w;
  float v1_v_over_v1_w = v1_uv.v * one_over_v1_w;
  float v2_v_over_v2_w = v2_uv.v * one_over_v2_w;
  float one_over_parallelogram_area = 1.0f / parallelogram_area;

  /* find the light intensities once per triangle */
  light_intensity_rgb_t intensity = find_total_light_intensity(light_intensities);

  for(int y = y_min; y <= y_max; y++){
    float w0 = w0_row;
    float w1 = w1_row;
    float w2 = w2_row;

    bool was_inside = false;
    for(int x = x_min; x <= x_max; x++){
      /* pixel is inside if w's are positive (point is inside triangle) or point is on a top/left edge(any of the 3 edges could be the top/left edge so we test all) */
      bool inside = (w0 > 0.0f || (w0 == 0.0f && edge0_top_left)) &&
                    (w1 > 0.0f || (w1 == 0.0f && edge1_top_left)) &&
                    (w2 > 0.0f || (w2 == 0.0f && edge2_top_left));

      if(inside){
        was_inside = true;
        float alpha = w0 * one_over_parallelogram_area;
        float beta  = w1 * one_over_parallelogram_area;
        float gamma = w2 * one_over_parallelogram_area;

        float interpolated_reciprocal_w = alpha * one_over_v0_w + beta * one_over_v1_w + gamma * one_over_v2_w;

        if(zbuffer[y * window_width + x] == 0.0f){
          float interpolated_u = alpha * v0_u_over_v0_w + beta  * v1_u_over_v1_w + gamma * v2_u_over_v2_w;
          float interpolated_v = alpha * v0_v_over_v0_w + beta  * v1_v_over_v1_w + gamma * v2_v_over_v2_w;

          interpolated_u /= interpolated_reciprocal_w;
          interpolated_v /= interpolated_reciprocal_w;

          int texture_u = (int)(interpolated_u * texture_width);
          int texture_v = (int)(interpolated_v * texture_height);

          if(texture_u < 0) texture_u = 0;
          if(texture_v < 0) texture_v = 0;
          if(texture_u >= texture_width)  texture_u = texture_width  - 1;
          if(texture_v >= texture_height) texture_v = texture_height - 1;

          color_t texel = texture[texture_v * texture_width + texture_u];
          
          if(texel.rgb.alpha >= alpha_cutoff){
            uint32_t red_total   = 0;
            uint32_t green_total = 0;
            uint32_t blue_total  = 0;

            if(reflective){
              red_total   = (uint32_t)(texel.rgb.red   * intensity.red  );
              green_total = (uint32_t)(texel.rgb.green * intensity.green);
              blue_total  = (uint32_t)(texel.rgb.blue  * intensity.blue );
            }

            /* add emissive color on top */
            if(emissive_intensity > 0.0f){
              red_total   += texel.rgb.red   * emissive_color.rgb.red   / 255.0f * emissive_intensity;
              green_total += texel.rgb.green * emissive_color.rgb.green / 255.0f * emissive_intensity;
              blue_total  += texel.rgb.blue  * emissive_color.rgb.blue  / 255.0f * emissive_intensity;
            }

            /* clamp total amount as results over 255 do not make sense */
            if(red_total   > 255) red_total   = 255;
            if(green_total > 255) green_total = 255;
            if(blue_total  > 255) blue_total  = 255;
            color_t final_color = {
              .rgb.alpha = texel.rgb.alpha,
              .rgb.blue  = blue_total,
              .rgb.green = green_total,
              .rgb.red   = red_total
            };
            if(dither) final_color = temporal_dither(final_color, x, y);

            if((unsigned)x < (unsigned)window_width && (unsigned)y < (unsigned)window_height)
              color_buffer[y * window_width + x] = final_color;
          }
        }
      }
      else if(was_inside) break;

      w0 += delta_w0_col;
      w1 += delta_w1_col;
      w2 += delta_w2_col;
    }

    w0_row += delta_w0_row;
    w1_row += delta_w1_row;
    w2_row += delta_w2_row;
  }

  if(projection_options.wireframe_overlay){
    draw_triangle(v0->x, v0->y, v1->x, v1->y, v2->x, v2->y, (color_t){.uint32 = 0xFFFFFFFF});
  }
}

__attribute__((optimize("Ofast")))
void draw_textured_triangle_alpha_blend(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  float *light_intensities,
  color_t emissive_color,
  float emissive_intensity,
  bool reflective,
  bool dither,
  float opacity
){
  float parallelogram_area = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y}
  );

  /* degenerate triangle */
  if(parallelogram_area == 0.0f) return;

  /* backface culling */
  //if(projection_options.backface_culling){
  //  if(parallelogram_area <= 0.0f) return;
  //}

  /* bounding box */
  int x_min = (int)floorf(MIN(MIN(v0->x, v1->x), v2->x));
  int y_min = (int)floorf(MIN(MIN(v0->y, v1->y), v2->y));
  int x_max = (int)ceilf (MAX(MAX(v0->x, v1->x), v2->x));
  int y_max = (int)ceilf (MAX(MAX(v0->y, v1->y), v2->y));

  /* if area is < 0, this means the triangle has opposite winding,
     hence here we swap two vertices to reverse the winding.  this is done so that the edge cross tests for points behaves normally
     again for reverse winded triangles.  because otherwise, whenever the winding is reversed, the edge cross function will always return negative.
     also, negate area instead of recomputing it for the corrected winding.
  */
  if(parallelogram_area < 0.0f){
    vector4_t *tmp_v = v1;
    v1 = v2;
    v2 = tmp_v;

    uv_t tmp_uv = v1_uv;
    v1_uv = v2_uv;
    v2_uv = tmp_uv;

    parallelogram_area = -parallelogram_area;
  }

  /* top-left rule flags for each edge
     edge0 is opposite vertices v1 and v2
     edge1 is opposite vertices v0 and v2
     edge2 is opposite vertices v0 and v1
  */
  bool edge0_top_left = is_top_left_edge(&(vector2_t){v1->x, v1->y}, &(vector2_t){v2->x, v2->y});
  bool edge1_top_left = is_top_left_edge(&(vector2_t){v2->x, v2->y}, &(vector2_t){v0->x, v0->y});
  bool edge2_top_left = is_top_left_edge(&(vector2_t){v0->x, v0->y}, &(vector2_t){v1->x, v1->y});

  /* edge deltas */
  float delta_w0_col = v1->y - v2->y;
  float delta_w1_col = v2->y - v0->y;
  float delta_w2_col = v0->y - v1->y;

  float delta_w0_row = v2->x - v1->x;
  float delta_w1_row = v0->x - v2->x;
  float delta_w2_row = v1->x - v0->x;

  /* edge function at first pixel center */
  vector2_t p = {x_min + 0.5f, y_min + 0.5f};

  float w0_row = edge_cross(
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y},
    &p
  );

  float w1_row = edge_cross(
    &(vector2_t){v2->x, v2->y},
    &(vector2_t){v0->x, v0->y},
    &p
  );

  float w2_row = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &p
  );

  /* pre-compute the barycentric interpolation cordinates here instead of inside loops */
  float one_over_v0_w = 1.0f / v0->w;
  float one_over_v1_w = 1.0f / v1->w;
  float one_over_v2_w = 1.0f / v2->w;
  float v0_u_over_v0_w = v0_uv.u * one_over_v0_w;
  float v1_u_over_v1_w = v1_uv.u * one_over_v1_w;
  float v2_u_over_v2_w = v2_uv.u * one_over_v2_w;
  float v0_v_over_v0_w = v0_uv.v * one_over_v0_w;
  float v1_v_over_v1_w = v1_uv.v * one_over_v1_w;
  float v2_v_over_v2_w = v2_uv.v * one_over_v2_w;
  float one_over_parallelogram_area = 1.0f / parallelogram_area;

  /* find the light intensities once per triangle */
  light_intensity_rgb_t intensity = find_total_light_intensity(light_intensities);

  for(int y = y_min; y <= y_max; y++){
    float w0 = w0_row;
    float w1 = w1_row;
    float w2 = w2_row;

    bool was_inside = false;
    for(int x = x_min; x <= x_max; x++){
      /* pixel is inside if w's are positive (point is inside triangle) or point is on a top/left edge(any of the 3 edges could be the top/left edge so we test all) */
      bool inside = (w0 > 0.0f || (w0 == 0.0f && edge0_top_left)) &&
                    (w1 > 0.0f || (w1 == 0.0f && edge1_top_left)) &&
                    (w2 > 0.0f || (w2 == 0.0f && edge2_top_left));

      if(inside){
        was_inside = true;
        float alpha = w0 * one_over_parallelogram_area;
        float beta  = w1 * one_over_parallelogram_area;
        float gamma = w2 * one_over_parallelogram_area;

        float interpolated_reciprocal_w = alpha * one_over_v0_w + beta * one_over_v1_w + gamma * one_over_v2_w;

        if(interpolated_reciprocal_w > zbuffer[y * window_width + x]){
          float interpolated_u = alpha * v0_u_over_v0_w + beta  * v1_u_over_v1_w + gamma * v2_u_over_v2_w;
          float interpolated_v = alpha * v0_v_over_v0_w + beta  * v1_v_over_v1_w + gamma * v2_v_over_v2_w;

          interpolated_u /= interpolated_reciprocal_w;
          interpolated_v /= interpolated_reciprocal_w;

          int texture_u = (int)(interpolated_u * texture_width);
          int texture_v = (int)(interpolated_v * texture_height);

          if(texture_u < 0) texture_u = 0;
          if(texture_v < 0) texture_v = 0;
          if(texture_u >= texture_width)  texture_u = texture_width  - 1;
          if(texture_v >= texture_height) texture_v = texture_height - 1;

          color_t texel = texture[texture_v * texture_width + texture_u];
          
          uint32_t red_total   = 0;
          uint32_t green_total = 0;
          uint32_t blue_total  = 0;

          if(reflective){
            red_total   = (uint32_t)(texel.rgb.red   * intensity.red  );
            green_total = (uint32_t)(texel.rgb.green * intensity.green);
            blue_total  = (uint32_t)(texel.rgb.blue  * intensity.blue );
          }

          /* add emissive color on top */
          if(emissive_intensity > 0.0f){
            red_total   += texel.rgb.red   * emissive_color.rgb.red   / 255.0f * emissive_intensity;
            green_total += texel.rgb.green * emissive_color.rgb.green / 255.0f * emissive_intensity;
            blue_total  += texel.rgb.blue  * emissive_color.rgb.blue  / 255.0f * emissive_intensity;
          }

          /* clamp total amount as results over 255 do not make sense */
          if(red_total   > 255) red_total   = 255;
          if(green_total > 255) green_total = 255;
          if(blue_total  > 255) blue_total  = 255;
          color_t final_color = {
            .rgb.alpha = texel.rgb.alpha,
            .rgb.blue  = blue_total,
            .rgb.green = green_total,
            .rgb.red   = red_total
          };
          if(dither) final_color = temporal_dither(final_color, x, y);

          draw_pixel_alpha_blend(x, y, opacity, final_color);
        }
      }
      else if(was_inside) break;

      w0 += delta_w0_col;
      w1 += delta_w1_col;
      w2 += delta_w2_col;
    }

    w0_row += delta_w0_row;
    w1_row += delta_w1_row;
    w2_row += delta_w2_row;
  }

  if(projection_options.wireframe_overlay){
    draw_triangle(v0->x, v0->y, v1->x, v1->y, v2->x, v2->y, (color_t){.uint32 = 0xFFFFFFFF});
  }
}

__attribute__((optimize("Ofast")))
void draw_textured_triangle(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  float *light_intensities,
  color_t emissive_color,
  float emissive_intensity,
  bool reflective,
  bool dither,
  blend_mode_t blend_mode,
  uint8_t alpha_cutoff,
  float opacity
){
  float parallelogram_area = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y}
  );

  /* degenerate triangle */
  if(parallelogram_area == 0.0f) return;

  /* backface culling */
  //if(projection_options.backface_culling){
  //  if(parallelogram_area <= 0.0f) return;
  //}

  /* bounding box */
  int x_min = (int)floorf(MIN(MIN(v0->x, v1->x), v2->x));
  int y_min = (int)floorf(MIN(MIN(v0->y, v1->y), v2->y));
  int x_max = (int)ceilf (MAX(MAX(v0->x, v1->x), v2->x));
  int y_max = (int)ceilf (MAX(MAX(v0->y, v1->y), v2->y));

  /* if area is < 0, this means the triangle has opposite winding,
     hence here we swap two vertices to reverse the winding.  this is done so that the edge cross tests for points behaves normally
     again for reverse winded triangles.  because otherwise, whenever the winding is reversed, the edge cross function will always return negative.
     also, negate area instead of recomputing it for the corrected winding.
  */
  if(parallelogram_area < 0.0f){
    vector4_t *tmp_v = v1;
    v1 = v2;
    v2 = tmp_v;

    uv_t tmp_uv = v1_uv;
    v1_uv = v2_uv;
    v2_uv = tmp_uv;

    parallelogram_area = -parallelogram_area;
  }

  /* top-left rule flags for each edge
     edge0 is opposite vertices v1 and v2
     edge1 is opposite vertices v0 and v2
     edge2 is opposite vertices v0 and v1
  */
  bool edge0_top_left = is_top_left_edge(&(vector2_t){v1->x, v1->y}, &(vector2_t){v2->x, v2->y});
  bool edge1_top_left = is_top_left_edge(&(vector2_t){v2->x, v2->y}, &(vector2_t){v0->x, v0->y});
  bool edge2_top_left = is_top_left_edge(&(vector2_t){v0->x, v0->y}, &(vector2_t){v1->x, v1->y});

  /* edge deltas */
  float delta_w0_col = v1->y - v2->y;
  float delta_w1_col = v2->y - v0->y;
  float delta_w2_col = v0->y - v1->y;

  float delta_w0_row = v2->x - v1->x;
  float delta_w1_row = v0->x - v2->x;
  float delta_w2_row = v1->x - v0->x;

  /* edge function at first pixel center */
  vector2_t p = {x_min + 0.5f, y_min + 0.5f};

  float w0_row = edge_cross(
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y},
    &p
  );

  float w1_row = edge_cross(
    &(vector2_t){v2->x, v2->y},
    &(vector2_t){v0->x, v0->y},
    &p
  );

  float w2_row = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &p
  );

  /* pre-compute the barycentric interpolation cordinates here instead of inside loops */
  float one_over_v0_w = 1.0f / v0->w;
  float one_over_v1_w = 1.0f / v1->w;
  float one_over_v2_w = 1.0f / v2->w;
  float v0_u_over_v0_w = v0_uv.u * one_over_v0_w;
  float v1_u_over_v1_w = v1_uv.u * one_over_v1_w;
  float v2_u_over_v2_w = v2_uv.u * one_over_v2_w;
  float v0_v_over_v0_w = v0_uv.v * one_over_v0_w;
  float v1_v_over_v1_w = v1_uv.v * one_over_v1_w;
  float v2_v_over_v2_w = v2_uv.v * one_over_v2_w;
  float one_over_parallelogram_area = 1.0f / parallelogram_area;

  /* find the light intensities once per triangle */
  light_intensity_rgb_t intensity = find_total_light_intensity(light_intensities);

  for(int y = y_min; y <= y_max; y++){
    float w0 = w0_row;
    float w1 = w1_row;
    float w2 = w2_row;

    bool was_inside = false;
    for(int x = x_min; x <= x_max; x++){
      /* pixel is inside if w's are positive (point is inside triangle) or point is on a top/left edge(any of the 3 edges could be the top/left edge so we test all) */
      bool inside = (w0 > 0.0f || (w0 == 0.0f && edge0_top_left)) &&
                    (w1 > 0.0f || (w1 == 0.0f && edge1_top_left)) &&
                    (w2 > 0.0f || (w2 == 0.0f && edge2_top_left));

      if(inside){
        was_inside = true;
        float alpha = w0 * one_over_parallelogram_area;
        float beta  = w1 * one_over_parallelogram_area;
        float gamma = w2 * one_over_parallelogram_area;

        float interpolated_reciprocal_w = alpha * one_over_v0_w + beta * one_over_v1_w + gamma * one_over_v2_w;

        if(interpolated_reciprocal_w > zbuffer[y * window_width + x]){
          float interpolated_u = alpha * v0_u_over_v0_w + beta  * v1_u_over_v1_w + gamma * v2_u_over_v2_w;
          float interpolated_v = alpha * v0_v_over_v0_w + beta  * v1_v_over_v1_w + gamma * v2_v_over_v2_w;

          interpolated_u /= interpolated_reciprocal_w;
          interpolated_v /= interpolated_reciprocal_w;

          int texture_u = (int)(interpolated_u * texture_width);
          int texture_v = (int)(interpolated_v * texture_height);

          if(texture_u < 0) texture_u = 0;
          if(texture_v < 0) texture_v = 0;
          if(texture_u >= texture_width)  texture_u = texture_width  - 1;
          if(texture_v >= texture_height) texture_v = texture_height - 1;

          color_t texel = texture[texture_v * texture_width + texture_u];
          
          uint32_t red_total   = 0;
          uint32_t green_total = 0;
          uint32_t blue_total  = 0;

          if(reflective){
            red_total   = (uint32_t)(texel.rgb.red   * intensity.red  );
            green_total = (uint32_t)(texel.rgb.green * intensity.green);
            blue_total  = (uint32_t)(texel.rgb.blue  * intensity.blue );
          }

          /* add emissive color on top */
          if(emissive_intensity > 0.0f){
            red_total   += texel.rgb.red   * emissive_color.rgb.red   / 255.0f * emissive_intensity;
            green_total += texel.rgb.green * emissive_color.rgb.green / 255.0f * emissive_intensity;
            blue_total  += texel.rgb.blue  * emissive_color.rgb.blue  / 255.0f * emissive_intensity;
          }

          /* clamp total amount as results over 255 do not make sense */
          if(red_total   > 255) red_total   = 255;
          if(green_total > 255) green_total = 255;
          if(blue_total  > 255) blue_total  = 255;
          color_t final_color = {
            .rgb.alpha = texel.rgb.alpha,
            .rgb.blue  = blue_total,
            .rgb.green = green_total,
            .rgb.red   = red_total
          };
          if(dither) final_color = temporal_dither(final_color, x, y);

          draw_pixel(x, y, blend_mode, alpha_cutoff, opacity, final_color, interpolated_reciprocal_w);
        }
      }
      else if(was_inside) break;

      w0 += delta_w0_col;
      w1 += delta_w1_col;
      w2 += delta_w2_col;
    }

    w0_row += delta_w0_row;
    w1_row += delta_w1_row;
    w2_row += delta_w2_row;
  }

  if(projection_options.wireframe_overlay){
    draw_triangle(v0->x, v0->y, v1->x, v1->y, v2->x, v2->y, (color_t){.uint32 = 0xFFFFFFFF});
  }
}

__attribute__((optimize("Ofast")))
void draw_textured_triangle_affine(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  float *light_intensities,
  bool dither,
  blend_mode_t blend_mode,
  uint8_t alpha_cutoff,
  float opacity
){
  float parallelogram_area = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y}
  );

  /* degenerate triangle */
  if(parallelogram_area == 0.0f) return;

  /* backface culling */
  //if(projection_options.backface_culling){
  //  if(parallelogram_area <= 0.0f) return;
  //}

  /* bounding box */
  int x_min = (int)floorf(MIN(MIN(v0->x, v1->x), v2->x));
  int y_min = (int)floorf(MIN(MIN(v0->y, v1->y), v2->y));
  int x_max = (int)ceilf (MAX(MAX(v0->x, v1->x), v2->x));
  int y_max = (int)ceilf (MAX(MAX(v0->y, v1->y), v2->y));

  /* if area is < 0, this means the triangle has opposite winding,
     hence here we swap two vertices to reverse the winding.  this is done so that the edge cross tests for points behaves normally
     again for reverse winded triangles.  because otherwise, whenever the winding is reversed, the edge cross function will always return negative.
     also, negate area instead of recomputing it for the corrected winding.
  */
  if(parallelogram_area < 0.0f){
    vector4_t *tmp_v = v1;
    v1 = v2;
    v2 = tmp_v;

    uv_t tmp_uv = v1_uv;
    v1_uv = v2_uv;
    v2_uv = tmp_uv;

    parallelogram_area = -parallelogram_area;
  }

  /* top-left rule flags for each edge
     edge0 is opposite vertices v1 and v2
     edge1 is opposite vertices v0 and v2
     edge2 is opposite vertices v0 and v1
  */
  bool edge0_top_left = is_top_left_edge(&(vector2_t){v1->x, v1->y}, &(vector2_t){v2->x, v2->y});
  bool edge1_top_left = is_top_left_edge(&(vector2_t){v2->x, v2->y}, &(vector2_t){v0->x, v0->y});
  bool edge2_top_left = is_top_left_edge(&(vector2_t){v0->x, v0->y}, &(vector2_t){v1->x, v1->y});

  /* edge deltas */
  float delta_w0_col = v1->y - v2->y;
  float delta_w1_col = v2->y - v0->y;
  float delta_w2_col = v0->y - v1->y;

  float delta_w0_row = v2->x - v1->x;
  float delta_w1_row = v0->x - v2->x;
  float delta_w2_row = v1->x - v0->x;

  /* edge function at first pixel center */
  vector2_t p = {x_min + 0.5f, y_min + 0.5f};

  float w0_row = edge_cross(
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y},
    &p
  );

  float w1_row = edge_cross(
    &(vector2_t){v2->x, v2->y},
    &(vector2_t){v0->x, v0->y},
    &p
  );

  float w2_row = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &p
  );

  /* pre-compute the barycentric interpolation cordinates here instead of inside loops */
  float v0_u = v0_uv.u;
  float v1_u = v1_uv.u;
  float v2_u = v2_uv.u;
  float v0_v = v0_uv.v;
  float v1_v = v1_uv.v;
  float v2_v = v2_uv.v;
  float one_over_parallelogram_area = 1.0f / parallelogram_area;

  /* find the light intensities once per triangle */
  light_intensity_rgb_t intensity = find_total_light_intensity(light_intensities);

  for(int y = y_min; y <= y_max; y++){
    float w0 = w0_row;
    float w1 = w1_row;
    float w2 = w2_row;

    bool was_inside = false;
    for(int x = x_min; x <= x_max; x++){
      /* pixel is inside if w's are positive (point is inside triangle) or point is on a top/left edge(any of the 3 edges could be the top/left edge so we test all) */
      bool inside = (w0 > 0.0f || (w0 == 0.0f && edge0_top_left)) &&
                    (w1 > 0.0f || (w1 == 0.0f && edge1_top_left)) &&
                    (w2 > 0.0f || (w2 == 0.0f && edge2_top_left));

      if(inside){
        was_inside = true;
        float alpha = w0 * one_over_parallelogram_area;
        float beta  = w1 * one_over_parallelogram_area;
        float gamma = w2 * one_over_parallelogram_area;

        float interpolated_reciprocal_w = alpha * (1.0f / v0->w) + beta  * (1.0f / v1->w) + gamma * (1.0f / v2->w);

        if(interpolated_reciprocal_w > zbuffer[y * window_width + x]){
          float interpolated_u = alpha * v0_u + beta * v1_u + gamma * v2_u;
          float interpolated_v = alpha * v0_v + beta * v1_v + gamma * v2_v;

          int texture_u = (int)(interpolated_u * texture_width);
          int texture_v = (int)(interpolated_v * texture_height);

          if(texture_u < 0) texture_u = 0;
          if(texture_v < 0) texture_v = 0;
          if(texture_u >= texture_width)  texture_u = texture_width  - 1;
          if(texture_v >= texture_height) texture_v = texture_height - 1;

          color_t texel = texture[texture_v * texture_width + texture_u];
          
          uint32_t red_total   = (uint32_t)(texel.rgb.red   * intensity.red  );
          uint32_t green_total = (uint32_t)(texel.rgb.green * intensity.green);
          uint32_t blue_total  = (uint32_t)(texel.rgb.blue  * intensity.blue );

          /* clamp total amount as results over 255 do not make sense */
          if(red_total   > 255) red_total   = 255;
          if(green_total > 255) green_total = 255;
          if(blue_total  > 255) blue_total  = 255;

          color_t final_color = {
            .rgb.alpha = texel.rgb.alpha, 
            .rgb.blue  = blue_total, 
            .rgb.green = green_total, 
            .rgb.red   = red_total
          };

          if(dither) final_color = temporal_dither(final_color, x, y);

          draw_pixel(x, y, blend_mode, alpha_cutoff, opacity, final_color, interpolated_reciprocal_w);
        }
      }
      else if(was_inside) break;

      w0 += delta_w0_col;
      w1 += delta_w1_col;
      w2 += delta_w2_col;
    }

    w0_row += delta_w0_row;
    w1_row += delta_w1_row;
    w2_row += delta_w2_row;
  }

  if(projection_options.wireframe_overlay){
    draw_triangle(v0->x, v0->y, v1->x, v1->y, v2->x, v2->y, (color_t){.uint32 = 0xFFFFFFFF});
  }
}

/*
      B
     / \
    / | \
   /  P  \
  / /   \ \
 A---------C

*/
inline weight_t barycentric_weights(vector2_t A, vector2_t B, vector2_t C, vector2_t P){
  /* vectors between points abc and p */
  vector2_t AC = vector2_sub(C, A);
  vector2_t AB = vector2_sub(B, A);
  vector2_t AP = vector2_sub(P, A);
  vector2_t PC = vector2_sub(C, P);
  vector2_t PB = vector2_sub(B, P);

  /* area of full paralleloram/triangle ABC using 2D cross product (i.e z component of cross product is 0) */
  float area_parallelogram_ABC = AC.x * AB.y - AC.y * AB.x; /* || AC cross AB || */

  /* area of smaller parallelogram PBC divided by full area of larger parallelogram */
  float alpha = (PC.x * PB.y - PC.y * PB.x) / area_parallelogram_ABC;
  /* area of smaller parallelogram APC divided by full area of larger parallelogram */
  float beta = (AC.x * AP.y - AC.y * AP.x) / area_parallelogram_ABC;
  /* alpha + beta + gamma must equal 1.0, so last weight is constrained */
  float gamma = 1.0f - alpha - beta;

  weight_t weights = {alpha, beta, gamma};

  return weights;
}

void draw_rect(int x, int y, int width, int height, color_t color){
  for(int i = 0; i < width; i++){
    for(int j = 0; j < height; j++)
      draw_pixel_absolute(x + i, y + j, color);
  }
}

/* use unsigned comparisons because if number is negative it becomes a huge unsigned number 
  TODO: for transparent objects:
  1) first render all opaque objects, and then transparent ones
  2) for each transparent object, the mesh needs to be sorted back to front. for transparent objects render order matters.
  also, transparent objects don't modify zbuffer.  they zbuffer test, but do not modify!
  function below is a temporary test
*/
__attribute__((optimize("Ofast")))
inline void draw_pixel_opaque(int x, int y, color_t color, float zbuffer_value){
  if((unsigned)x >= (unsigned)window_width || (unsigned)y >= (unsigned)window_height) return;

  color_buffer[y * window_width + x] = color;
  zbuffer[y * window_width + x] = zbuffer_value;
}

__attribute__((optimize("Ofast")))
inline void draw_pixel_alpha_test(int x, int y, uint8_t alpha_cutoff, color_t color, float zbuffer_value){
  if((unsigned)x >= (unsigned)window_width || (unsigned)y >= (unsigned)window_height) return;

  if(color.rgb.alpha < alpha_cutoff) return;

  color_buffer[y * window_width + x] = color;
  zbuffer[y * window_width + x] = zbuffer_value;
}

__attribute__((optimize("Ofast")))
inline void draw_pixel_alpha_blend(int x, int y, float opacity, color_t color){

  if((unsigned)x >= (unsigned)window_width || (unsigned)y >= (unsigned)window_height) return;

  uint8_t alpha = color.rgb.alpha;
  /* combine texture alpha with object opacity */
  alpha = alpha * opacity;

  if(alpha == 0) return;
  if(alpha == 255){
    color.rgb.alpha = 255;
    color_buffer[y * window_width + x] = color;
    return;
  }

  uint8_t inv_alpha = 255 - alpha;

  color_t dst = color_buffer[y * window_width + x];

  uint8_t src_red   = color.rgb.red;
  uint8_t src_green = color.rgb.green;
  uint8_t src_blue  = color.rgb.blue;

  uint8_t dst_red   = dst.rgb.red;
  uint8_t dst_green = dst.rgb.green;
  uint8_t dst_blue  = dst.rgb.blue;

  color_t final_color = {
    .rgb.alpha = 255, 
    .rgb.blue  = (src_blue  * alpha + dst_blue  * inv_alpha) >> 8, 
    .rgb.green = (src_green * alpha + dst_green * inv_alpha) >> 8,
    .rgb.red   = (src_red   * alpha + dst_red   * inv_alpha) >> 8
  };

  color_buffer[y * window_width + x] = final_color;
}

__attribute__((optimize("Ofast")))
inline void draw_pixel(int x, int y, blend_mode_t blend_mode, uint8_t alpha_cutoff, float opacity, color_t color, float zbuffer_value){

  if((unsigned)x >= (unsigned)window_width || (unsigned)y >= (unsigned)window_height) return;

  switch(blend_mode){
    case BLEND_MODE_OPAQUE:{
      color_buffer[y * window_width + x] = color;
      zbuffer[y * window_width + x] = zbuffer_value;
      return;
    }

    case BLEND_MODE_ALPHA_TEST:{
      if(color.rgb.alpha < alpha_cutoff) return;
      color_buffer[y * window_width + x] = color;
      zbuffer[y * window_width + x] = zbuffer_value;
      return;
    }

    case BLEND_MODE_ALPHA_BLEND: {
      uint8_t alpha = color.rgb.alpha;
      /* combine texture alpha with object opacity */
      alpha = alpha * opacity;

      if(alpha == 0) break;
      if(alpha == 255){
        color.rgb.alpha = 255;
        color_buffer[y * window_width + x] = color;
        return;
      }

      uint8_t inv_alpha = 255 - alpha;

      color_t dst = color_buffer[y * window_width + x];

      uint8_t src_red   = color.rgb.red;
      uint8_t src_green = color.rgb.green;
      uint8_t src_blue  = color.rgb.blue;

      uint8_t dst_red   = dst.rgb.red;
      uint8_t dst_green = dst.rgb.green;
      uint8_t dst_blue  = dst.rgb.blue;

      color_t final_color = {
        .rgb.alpha = 255, 
        .rgb.blue  = (src_blue  * alpha + dst_blue  * inv_alpha) >> 8, 
        .rgb.green = (src_green * alpha + dst_green * inv_alpha) >> 8,
        .rgb.red   = (src_red   * alpha + dst_red   * inv_alpha) >> 8
      };

      color_buffer[y * window_width + x] = final_color;
    }
  }
}

inline void draw_pixel_absolute(int x, int y, color_t color){
  if((unsigned)x >= (unsigned)window_width || (unsigned)y >= (unsigned)window_height) return;

  color_buffer[y * window_width + x] = color;
}

void draw_line(float x0, float y0, float x1, float y1, color_t color){
  float delta_x = x1 - x0;
  float delta_y = y1 - y0;

  float longest_side_length = (fabs(delta_x) >= fabs(delta_y)) ? fabs(delta_x) : fabs(delta_y);

  float x_inc = delta_x / longest_side_length;
  float y_inc = delta_y / longest_side_length;

  float curr_x = x0;
  float curr_y = y0;
  for(int i = 0; i <= longest_side_length; i++){
    draw_pixel_absolute(lroundf(curr_x), lroundf(curr_y), color);
    curr_x += x_inc;
    curr_y += y_inc;
  }
}




/*


 ******** OPTIMIZED FUNCTION THAT ONLY DRAWS WHEN ENTERING TRIANGLE, AND LEAVES EARLY WHEN EXITING TRIANGLE
 ******** DIDN'T TOUCH EVEN 1 FPS WHEN TESTED SO NOT USED FOR NOW


void draw_textured_triangle_opaque(
  vector4_t *v0, uv_t v0_uv,
  vector4_t *v1, uv_t v1_uv,
  vector4_t *v2, uv_t v2_uv,
  color_t *texture, int texture_width, int texture_height,
  float *light_intensities,
  color_t emissive_color,
  float emissive_intensity,
  bool reflective,
  bool dither
){
  float parallelogram_area = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y}
  );

  // degenerate triangle
  if(parallelogram_area == 0.0f) return;

  // bounding box 
  int x_min = (int)floorf(MIN(MIN(v0->x, v1->x), v2->x));
  int y_min = (int)floorf(MIN(MIN(v0->y, v1->y), v2->y));
  int x_max = (int)ceilf (MAX(MAX(v0->x, v1->x), v2->x));
  int y_max = (int)ceilf (MAX(MAX(v0->y, v1->y), v2->y));

  // if area is < 0, reverse winding 
  if(parallelogram_area < 0.0f){
    vector4_t *tmp_v = v1;
    v1 = v2;
    v2 = tmp_v;

    uv_t tmp_uv = v1_uv;
    v1_uv = v2_uv;
    v2_uv = tmp_uv;

    parallelogram_area = -parallelogram_area;
  }

  // clip bounding box to screen 
  if(x_min < 0) x_min = 0;
  if(y_min < 0) y_min = 0;
  if(x_max >= window_width)  x_max = window_width  - 1;
  if(y_max >= window_height) y_max = window_height - 1;

  if(x_min > x_max || y_min > y_max) return;

  // top-left rule flags for each edge
  //   edge0 is opposite vertices v1 and v2
  //   edge1 is opposite vertices v0 and v2
  //   edge2 is opposite vertices v0 and v1
  //
  bool edge0_top_left = is_top_left_edge(&(vector2_t){v1->x, v1->y}, &(vector2_t){v2->x, v2->y});
  bool edge1_top_left = is_top_left_edge(&(vector2_t){v2->x, v2->y}, &(vector2_t){v0->x, v0->y});
  bool edge2_top_left = is_top_left_edge(&(vector2_t){v0->x, v0->y}, &(vector2_t){v1->x, v1->y});

  // edge deltas 
  float delta_w0_col = v1->y - v2->y;
  float delta_w1_col = v2->y - v0->y;
  float delta_w2_col = v0->y - v1->y;

  float delta_w0_row = v2->x - v1->x;
  float delta_w1_row = v0->x - v2->x;
  float delta_w2_row = v1->x - v0->x;

  // edge function at first pixel center 
  vector2_t p = {x_min + 0.5f, y_min + 0.5f};

  float w0_row = edge_cross(
    &(vector2_t){v1->x, v1->y},
    &(vector2_t){v2->x, v2->y},
    &p
  );

  float w1_row = edge_cross(
    &(vector2_t){v2->x, v2->y},
    &(vector2_t){v0->x, v0->y},
    &p
  );

  float w2_row = edge_cross(
    &(vector2_t){v0->x, v0->y},
    &(vector2_t){v1->x, v1->y},
    &p
  );

  // pre-compute barycentric interpolation terms 
  float one_over_v0_w = 1.0f / v0->w;
  float one_over_v1_w = 1.0f / v1->w;
  float one_over_v2_w = 1.0f / v2->w;
  float v0_u_over_v0_w = v0_uv.u * one_over_v0_w;
  float v1_u_over_v1_w = v1_uv.u * one_over_v1_w;
  float v2_u_over_v2_w = v2_uv.u * one_over_v2_w;
  float v0_v_over_v0_w = v0_uv.v * one_over_v0_w;
  float v1_v_over_v1_w = v1_uv.v * one_over_v1_w;
  float v2_v_over_v2_w = v2_uv.v * one_over_v2_w;
  float one_over_parallelogram_area = 1.0f / parallelogram_area;

  // find the light intensities once per triangle 
  light_intensity_rgb_t intensity = find_total_light_intensity(light_intensities);

  for(int y = y_min; y <= y_max; y++){
    float w0 = w0_row;
    float w1 = w1_row;
    float w2 = w2_row;

    int x = x_min;

     skip pixels on the left until we enter the triangle 
    for(; x <= x_max; x++){
      bool inside = (w0 > 0.0f || (w0 == 0.0f && edge0_top_left)) &&
                    (w1 > 0.0f || (w1 == 0.0f && edge1_top_left)) &&
                    (w2 > 0.0f || (w2 == 0.0f && edge2_top_left));

      if(inside){
        break;
      }

      w0 += delta_w0_col;
      w1 += delta_w1_col;
      w2 += delta_w2_col;
    }

    // row does not intersect triangle 
    if(x > x_max){
      w0_row += delta_w0_row;
      w1_row += delta_w1_row;
      w2_row += delta_w2_row;
      continue;
    }

    // now fill until we leave triangle, then stop row 
    for(; x <= x_max; x++){
      bool inside = (w0 > 0.0f || (w0 == 0.0f && edge0_top_left)) &&
                    (w1 > 0.0f || (w1 == 0.0f && edge1_top_left)) &&
                    (w2 > 0.0f || (w2 == 0.0f && edge2_top_left));

      if(!inside){
        break;
      }

      float alpha = w0 * one_over_parallelogram_area;
      float beta  = w1 * one_over_parallelogram_area;
      float gamma = w2 * one_over_parallelogram_area;

      float interpolated_reciprocal_w =
        alpha * one_over_v0_w +
        beta  * one_over_v1_w +
        gamma * one_over_v2_w;

      int buffer_index = y * window_width + x;

      if(interpolated_reciprocal_w > zbuffer[buffer_index]){
        float interpolated_u =
          alpha * v0_u_over_v0_w +
          beta  * v1_u_over_v1_w +
          gamma * v2_u_over_v2_w;

        float interpolated_v =
          alpha * v0_v_over_v0_w +
          beta  * v1_v_over_v1_w +
          gamma * v2_v_over_v2_w;

        interpolated_u /= interpolated_reciprocal_w;
        interpolated_v /= interpolated_reciprocal_w;

        int texture_u = (int)(interpolated_u * texture_width);
        int texture_v = (int)(interpolated_v * texture_height);

        if(texture_u < 0) texture_u = 0;
        if(texture_v < 0) texture_v = 0;
        if(texture_u >= texture_width)  texture_u = texture_width  - 1;
        if(texture_v >= texture_height) texture_v = texture_height - 1;

        color_t texel = texture[texture_v * texture_width + texture_u];

        uint32_t red_total   = 0;
        uint32_t green_total = 0;
        uint32_t blue_total  = 0;

        if(reflective){
          red_total   = (uint32_t)(texel.rgb.red   * intensity.red);
          green_total = (uint32_t)(texel.rgb.green * intensity.green);
          blue_total  = (uint32_t)(texel.rgb.blue  * intensity.blue);
        }

        / add emissive color on top 
        if(emissive_intensity > 0.0f){
          red_total   += texel.rgb.red   * emissive_color.rgb.red   / 255.0f * emissive_intensity;
          green_total += texel.rgb.green * emissive_color.rgb.green / 255.0f * emissive_intensity;
          blue_total  += texel.rgb.blue  * emissive_color.rgb.blue  / 255.0f * emissive_intensity;
        }

         clamp total amount 
        if(red_total   > 255) red_total   = 255;
        if(green_total > 255) green_total = 255;
        if(blue_total  > 255) blue_total  = 255;

        color_t final_color = {
          .rgb.alpha = texel.rgb.alpha,
          .rgb.blue  = blue_total,
          .rgb.green = green_total,
          .rgb.red   = red_total
        };

        if(dither){
          final_color = temporal_dither(final_color, x, y);
        }

        color_buffer[buffer_index] = final_color;
        zbuffer[buffer_index] = interpolated_reciprocal_w;
      }

      w0 += delta_w0_col;
      w1 += delta_w1_col;
      w2 += delta_w2_col;
    }

    w0_row += delta_w0_row;
    w1_row += delta_w1_row;
    w2_row += delta_w2_row;
  }

  if(projection_options.wireframe_overlay){
    draw_triangle(v0->x, v0->y, v1->x, v1->y, v2->x, v2->y, (color_t){.uint32 = 0xFFFFFFFF});
  }
}
*/