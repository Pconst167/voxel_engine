#ifndef CLIPPINH_H
#define CLIPPINH_H

#include <math.h>
#include <stdint.h>
#include "vector.h"
#include "triangle.h"

#define MAX_NUM_POLYGON_VERTICES 10
#define NUM_VIEW_FRUSTRUM_PLANES 6

typedef enum{
  FRUSTRUM_LEFT   = 0,
  FRUSTRUM_RIGHT  = 1,
  FRUSTRUM_BOTTOM = 2,
  FRUSTRUM_TOP    = 3,
  FRUSTRUM_NEAR   = 4,
  FRUSTRUM_FAR    = 5
} frustrum_plane_t;

typedef struct{
  vector3_t p;
  vector3_t normal;
} plane_t;

typedef struct{
  vector3_t vertices[MAX_NUM_POLYGON_VERTICES];
  uv_t text_coords[MAX_NUM_POLYGON_VERTICES];
  int num_vertices;
} polygon_t;

extern plane_t view_frustrum_planes[NUM_VIEW_FRUSTRUM_PLANES];

void initialize_view_frustrum_planes(float fov_x, float fov_y, float znear, float zfar);
polygon_t create_polygon_from_triangle(vector3_t v0, vector3_t v1, vector3_t v2, uv_t t0, uv_t t1, uv_t t2);
void create_triangles_from_polygon(polygon_t *polygon, triangle_t *triangle_list);
void frustrum_clip_polygon(polygon_t *polygon);
bool frustrum_cull_object(vector4_t *bsphere_center, float bsphere_radius, float znear, float zfar);

/* combined function for creating a polygon from triangle, clipping against frustrum, and re-triangularizing 
   returns the number of triangles that were generated from the polygon */
int frustrum_clip_triangle_and_re_triangularize(
  vector3_t v0, 
  vector3_t v1, 
  vector3_t v2, 
  uv_t t0, 
  uv_t t1, 
  uv_t t2,
  triangle_t *triangles_after_clipping
);

bool chunk_any_face_vertex_outside_frustrum(vector4_t v0, vector4_t v1, vector4_t v2, float znear, float zfar);

#endif