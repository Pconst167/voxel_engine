#ifndef CLIPPINH_H
#define CLIPPINH_H

#include "vector.h"
#include "triangle.h"

#define MAX_NUM_POLYGON_VERTICES 10

typedef enum{
  FRUSTRUM_LEFT,
  FRUSTRUM_RIGHT,
  FRUSTRUM_BOTTOM,
  FRUSTRUM_TOP,
  FRUSTRUM_NEAR,
  FRUSTRUM_FAR
} frustrum_plane_t;

typedef struct{
  vector3_t p;
  vector3_t normal;
} plane_t;

typedef struct{
  vector3_t vertices[MAX_NUM_POLYGON_VERTICES];
  tex2_t text_coords[MAX_NUM_POLYGON_VERTICES];
  int num_vertices;
} polygon_t;

void initialize_view_frustrum_planes(float fov_x, float fov_y, float znear, float zfar);
polygon_t create_polygon_from_triangle(vector3_t v0, vector3_t v1, vector3_t v2, tex2_t t0, tex2_t t1, tex2_t t2);
void create_triangles_from_polygon(polygon_t *polygon, triangle_t *triangle_list);
void frustrum_clip_polygon(polygon_t *polygon);
bool frustrum_cull_object(vector4_t *bsphere_center, float bsphere_radius, float znear, float zfar);

/* combined function for creating a polygon from triangle, clipping against frustrum, and re-triangularizing 
   returns the number of triangles that were generated from the polygon */
int frustrum_clip_triangle_and_re_triangularize(
  vector3_t v0, 
  vector3_t v1, 
  vector3_t v2, 
  tex2_t t0, 
  tex2_t t1, 
  tex2_t t2,
  triangle_t *triangles_after_clipping
);

#endif