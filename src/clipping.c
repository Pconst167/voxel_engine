#include <math.h>
#include <stdint.h>
#include "clipping.h"

#define NUM_VIEW_FRUSTRUM_PLANES 6
plane_t view_frustrum_planes[NUM_VIEW_FRUSTRUM_PLANES];

void initialize_view_frustrum_planes(float fov_x, float fov_y, float znear, float zfar){
  float cos_fov_x_over_2 = cosf(fov_x / 2.0f);
  float sin_fov_x_over_2 = sinf(fov_x / 2.0f);
  
  float cos_fov_y_over_2 = cosf(fov_y / 2.0f);
  float sin_fov_y_over_2 = sinf(fov_y / 2.0f);

  vector3_t origin = {
    .x = 0.0f, 
    .y = 0.0f, 
    .z = 0.0f
  };

  view_frustrum_planes[FRUSTRUM_LEFT].p = origin;
  view_frustrum_planes[FRUSTRUM_LEFT].normal = (vector3_t){
    .x = cos_fov_x_over_2,
    .y = 0.0f,
    .z = sin_fov_x_over_2
  };

  view_frustrum_planes[FRUSTRUM_RIGHT].p = origin;
  view_frustrum_planes[FRUSTRUM_RIGHT].normal = (vector3_t){
    .x = -cos_fov_x_over_2,
    .y = 0.0f,
    .z = sin_fov_x_over_2
  };

  view_frustrum_planes[FRUSTRUM_BOTTOM].p = origin;
  view_frustrum_planes[FRUSTRUM_BOTTOM].normal = (vector3_t){
    .x = 0.0f,
    .y = cos_fov_y_over_2,
    .z = sin_fov_y_over_2
  };

  view_frustrum_planes[FRUSTRUM_TOP].p = origin;
  view_frustrum_planes[FRUSTRUM_TOP].normal = (vector3_t){
    .x = 0.0f,
    .y = -cos_fov_y_over_2,
    .z = sin_fov_y_over_2
  };

  view_frustrum_planes[FRUSTRUM_NEAR].p = (vector3_t){
    .x = 0.0f, 
    .y = 0.0f, 
    .z = znear
  };
  view_frustrum_planes[FRUSTRUM_NEAR].normal = (vector3_t){
    .x = 0.0f,
    .y = 0.0f,
    .z = 1.0f
  };

  view_frustrum_planes[FRUSTRUM_FAR].p = (vector3_t){
    .x = 0.0f,  
    .y = 0.0f,  
    .z = zfar
  };
  view_frustrum_planes[FRUSTRUM_FAR].normal = (vector3_t){
    .x =  0.0f,
    .y =  0.0f,
    .z = -1.0f
  };
}

polygon_t create_polygon_from_triangle(
  vector3_t v0, 
  vector3_t v1, 
  vector3_t v2, 
  tex2_t t0, 
  tex2_t t1, 
  tex2_t t2
){
  polygon_t polygon = {
    .vertices     = {v0, v1, v2},
    .text_coords  = {t0, t1, t2},
    .num_vertices = 3
  };

  return polygon;
}

void create_triangles_from_polygon(polygon_t *polygon, triangle_t *triangle_list){
  for(int i = 0; i < polygon->num_vertices - 2; i++){
    triangle_list[i].points[0] = (vector4_t){
      .x = polygon->vertices[0].x,
      .y = polygon->vertices[0].y,
      .z = polygon->vertices[0].z,
      .w = 1.0f
    };
    triangle_list[i].points[1] = (vector4_t){
      .x = polygon->vertices[i + 1].x,
      .y = polygon->vertices[i + 1].y,
      .z = polygon->vertices[i + 1].z,
      .w = 1.0f
    };
    triangle_list[i].points[2] = (vector4_t){
      .x = polygon->vertices[i + 2].x,
      .y = polygon->vertices[i + 2].y,
      .z = polygon->vertices[i + 2].z,
      .w = 1.0f
    };
    triangle_list[i].uv[0] = polygon->text_coords[0];
    triangle_list[i].uv[1] = polygon->text_coords[i + 1];
    triangle_list[i].uv[2] = polygon->text_coords[i + 2];
  }
}

/* a point is "inside" if (Q - P) dot normal > 0
  a point is "outside" if (Q - P) dot normal < 0
  a point is on the plane if (Q - P) dot normal == 0
  inside means a point is to the side where the normal points to
  outside means a point is to the side opposite where the normal points to
*/
void clip_polygon_against_plane2(polygon_t *polygon, frustrum_plane_t plane){
  vector3_t plane_p = view_frustrum_planes[plane].p;
  vector3_t plane_normal = view_frustrum_planes[plane].normal;

  vector3_t inside_vertices[MAX_NUM_POLYGON_VERTICES];
  tex2_t inside_text_coords[MAX_NUM_POLYGON_VERTICES];
  uint8_t num_inside_vertices = 0;

  vector3_t *curr_vertex     = &(polygon->vertices[0]);
  tex2_t    *curr_text_coord = &(polygon->text_coords[0]);
  vector3_t *prev_vertex     = &(polygon->vertices[polygon->num_vertices - 1]);
  tex2_t    *prev_text_coord = &(polygon->text_coords[polygon->num_vertices - 1]);

  float curr_dot;
  /* dot product between vector from P to previous vertex, and the plane's normal */
  float prev_dot = (prev_vertex->x - plane_p.x) * plane_normal.x + (prev_vertex->y - plane_p.y) * plane_normal.y + (prev_vertex->z - plane_p.z) * plane_normal.z;

  while(curr_vertex != &(polygon->vertices[polygon->num_vertices])){
    /* dot product between vector from P to current vertex, and the plane's normal */
    curr_dot = (curr_vertex->x - plane_p.x) * plane_normal.x + (curr_vertex->y - plane_p.y) * plane_normal.y + (curr_vertex->z - plane_p.z) * plane_normal.z;

    if((curr_dot < 0.0f) != (prev_dot < 0.0f)){ /* check if the signs of each dot product are different */
      float t = prev_dot / (prev_dot - curr_dot);
      /* intersection point I = Q_prev + t(Q_curr - Q_prev) */
      vector3_t I = (vector3_t){
        .x = prev_vertex->x + t * (curr_vertex->x - prev_vertex->x), 
        .y = prev_vertex->y + t * (curr_vertex->y - prev_vertex->y), 
        .z = prev_vertex->z + t * (curr_vertex->z - prev_vertex->z)
      }; 
      tex2_t T = (tex2_t){
        .u = prev_text_coord->u + t * (curr_text_coord->u - prev_text_coord->u), 
        .v = prev_text_coord->v + t * (curr_text_coord->v - prev_text_coord->v)
      }; 
      inside_vertices[num_inside_vertices] = I;
      inside_text_coords[num_inside_vertices] = T;
      num_inside_vertices++;
    }

    /* if current vertex is inside then add to list */
    /* TODO: 0 dot result case is when point is ON the plane, but there is a case where polygon is totally outside but with a few points ON the plane. need to make sure to think about that more 
       we test >= 0 instead of > 0 so that we include vertices that are just on the plane.  there is a danger however if the polygon is totally outside the frustrum and we still include those vertices,
       BUT when we do include, then the maximum number of vertices in the resulting included polygon is 2, and that being the case, the main loop won't execute for that polygon because we test num_polygon_vertices - 2 = 0.      
       so we should be safe.
    */
    if(curr_dot >= 0.0f){
      inside_vertices[num_inside_vertices] = *curr_vertex;
      inside_text_coords[num_inside_vertices] = *curr_text_coord;
      num_inside_vertices++;
    }

    /* move to next vertex */
    prev_dot = curr_dot;
    prev_vertex = curr_vertex;
    prev_text_coord = curr_text_coord;
    curr_vertex++;
    curr_text_coord++;
  }

  /* copy list of inside vertices into the polygon output result */
  polygon->num_vertices = num_inside_vertices;
  for(uint8_t i = 0; i < num_inside_vertices; i++){
    polygon->vertices[i]    = inside_vertices[i];
    polygon->text_coords[i] = inside_text_coords[i];
  }
}

static inline void clip_polygon_against_plane(polygon_t *polygon, frustrum_plane_t plane){
  vector3_t *vertices  = polygon->vertices;
  tex2_t *text_coords  = polygon->text_coords;
  uint8_t num_vertices = polygon->num_vertices;

  float px = view_frustrum_planes[plane].p.x;
  float py = view_frustrum_planes[plane].p.y;
  float pz = view_frustrum_planes[plane].p.z;

  float nx = view_frustrum_planes[plane].normal.x;
  float ny = view_frustrum_planes[plane].normal.y;
  float nz = view_frustrum_planes[plane].normal.z;

  vector3_t inside_vertices[MAX_NUM_POLYGON_VERTICES];
  tex2_t inside_text_coords[MAX_NUM_POLYGON_VERTICES];
  uint8_t num_inside_vertices = 0;

  uint8_t prev_i = num_vertices - 1;

  float prev_dot = (vertices[prev_i].x - px) * nx + (vertices[prev_i].y - py) * ny + (vertices[prev_i].z - pz) * nz;

  bool any_inside  = prev_dot >= 0.0f;
  bool any_outside = prev_dot <  0.0f;

  for(uint8_t curr_i = 0; curr_i < num_vertices; curr_i++){
    float curr_dot = (vertices[curr_i].x - px) * nx + (vertices[curr_i].y - py) * ny + (vertices[curr_i].z - pz) * nz;

    bool curr_inside = curr_dot >= 0.0f;
    bool prev_inside = prev_dot >= 0.0f;

    any_inside  |=  curr_inside;
    any_outside |= !curr_inside;

    if(curr_inside != prev_inside){
      float t = prev_dot / (prev_dot - curr_dot);

      inside_vertices[num_inside_vertices].x = vertices[prev_i].x + t * (vertices[curr_i].x - vertices[prev_i].x);
      inside_vertices[num_inside_vertices].y = vertices[prev_i].y + t * (vertices[curr_i].y - vertices[prev_i].y);
      inside_vertices[num_inside_vertices].z = vertices[prev_i].z + t * (vertices[curr_i].z - vertices[prev_i].z);

      inside_text_coords[num_inside_vertices].u = text_coords[prev_i].u + t * (text_coords[curr_i].u - text_coords[prev_i].u);
      inside_text_coords[num_inside_vertices].v = text_coords[prev_i].v + t * (text_coords[curr_i].v - text_coords[prev_i].v);

      num_inside_vertices++;
    }

    if(curr_inside){
      inside_vertices[num_inside_vertices]    = vertices[curr_i];
      inside_text_coords[num_inside_vertices] = text_coords[curr_i];
      num_inside_vertices++;
    }

    prev_i   = curr_i;
    prev_dot = curr_dot;
  }

  /* early-out: fully outside */
  if(!any_inside){
    polygon->num_vertices = 0;
    return;
  }

  /* early-out: fully inside */
  if(!any_outside) return;

  polygon->num_vertices = num_inside_vertices;
  for(uint8_t i = 0; i < num_inside_vertices; i++){
    vertices[i]    = inside_vertices[i];
    text_coords[i] = inside_text_coords[i];
  }
}

void frustrum_clip_polygon(polygon_t *polygon){
  clip_polygon_against_plane(polygon, FRUSTRUM_LEFT);
  clip_polygon_against_plane(polygon, FRUSTRUM_RIGHT);
  clip_polygon_against_plane(polygon, FRUSTRUM_BOTTOM);
  clip_polygon_against_plane(polygon, FRUSTRUM_TOP);
  clip_polygon_against_plane(polygon, FRUSTRUM_NEAR);
  clip_polygon_against_plane(polygon, FRUSTRUM_FAR);
}

__attribute__((optimize("Ofast")))
inline bool frustrum_cull_object(vector4_t *bsphere_center, float bsphere_radius, float znear, float zfar){
  float distance;

  if(bsphere_center->z + bsphere_radius < znear) return true;
  if(bsphere_center->z - bsphere_radius > zfar)  return true;

  /* LEFT */
  distance = bsphere_center->x * view_frustrum_planes[FRUSTRUM_LEFT].normal.x +
             bsphere_center->y * view_frustrum_planes[FRUSTRUM_LEFT].normal.y +
             bsphere_center->z * view_frustrum_planes[FRUSTRUM_LEFT].normal.z;

  if(distance < -bsphere_radius) return true;

  /* RIGHT */
  distance = bsphere_center->x * view_frustrum_planes[FRUSTRUM_RIGHT].normal.x +
             bsphere_center->y * view_frustrum_planes[FRUSTRUM_RIGHT].normal.y +
             bsphere_center->z * view_frustrum_planes[FRUSTRUM_RIGHT].normal.z;

  if(distance < -bsphere_radius) return true;

  /* BOTTOM */
  distance = bsphere_center->x * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.x +
             bsphere_center->y * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.y +
             bsphere_center->z * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.z;

  if(distance < -bsphere_radius) return true;

  /* TOP */
  distance = bsphere_center->x * view_frustrum_planes[FRUSTRUM_TOP].normal.x +
             bsphere_center->y * view_frustrum_planes[FRUSTRUM_TOP].normal.y +
             bsphere_center->z * view_frustrum_planes[FRUSTRUM_TOP].normal.z;

  if(distance < -bsphere_radius) return true;


  return false;
}

inline int frustrum_clip_triangle_and_re_triangularize(
  vector3_t v0, 
  vector3_t v1, 
  vector3_t v2, 
  tex2_t t0, 
  tex2_t t1, 
  tex2_t t2,
  triangle_t *triangles_after_clipping
){
  /* create a polygon from the original triangle */
  polygon_t polygon = {
    .vertices     = {v0, v1, v2},
    .text_coords  = {t0, t1, t2},
    .num_vertices = 3
  };

  /* apply frustrum clipping with early exit */
  clip_polygon_against_plane(&polygon, FRUSTRUM_LEFT);
  if (polygon.num_vertices < 3) return 0;

  clip_polygon_against_plane(&polygon, FRUSTRUM_RIGHT);
  if (polygon.num_vertices < 3) return 0;

  clip_polygon_against_plane(&polygon, FRUSTRUM_BOTTOM);
  if (polygon.num_vertices < 3) return 0;

  clip_polygon_against_plane(&polygon, FRUSTRUM_TOP);
  if (polygon.num_vertices < 3) return 0;

  clip_polygon_against_plane(&polygon, FRUSTRUM_NEAR);
  if (polygon.num_vertices < 3) return 0;

  clip_polygon_against_plane(&polygon, FRUSTRUM_FAR);
  if (polygon.num_vertices < 3) return 0;

  /* break polygon apart back into triangles */
  int num_polygon_vertices = polygon.num_vertices;
  int num_triangles_after_clipping = num_polygon_vertices - 2;

  vector3_t *polygon_vertices = polygon.vertices;
  tex2_t *polygon_text_coords = polygon.text_coords;

  /* cache base vertex and uv */
  vector4_t base_point = (vector4_t){
    .x = polygon_vertices[0].x,
    .y = polygon_vertices[0].y,
    .z = polygon_vertices[0].z,
    .w = 1.0f
  };

  tex2_t base_text_coord = polygon_text_coords[0];

  for (int i = 0; i < num_triangles_after_clipping; i++){
    triangle_t *triangle = &triangles_after_clipping[i];

    triangle->points[0] = base_point;
    triangle->points[1] = (vector4_t){
      .x = polygon_vertices[i + 1].x,
      .y = polygon_vertices[i + 1].y,
      .z = polygon_vertices[i + 1].z,
      .w = 1.0f
    };
    triangle->points[2] = (vector4_t){
      .x = polygon_vertices[i + 2].x,
      .y = polygon_vertices[i + 2].y,
      .z = polygon_vertices[i + 2].z,
      .w = 1.0f
    };

    triangle->uv[0] = base_text_coord;
    triangle->uv[1] = polygon_text_coords[i + 1];
    triangle->uv[2] = polygon_text_coords[i + 2];
  }

  return num_triangles_after_clipping;
}

/* old version */
inline int frustrum_clip_triangle_and_re_triangularize2(
  vector3_t v0, 
  vector3_t v1, 
  vector3_t v2, 
  tex2_t t0, 
  tex2_t t1, 
  tex2_t t2,
  triangle_t *triangles_after_clipping
){

  /* create a polygon from the original triangle */
  polygon_t polygon = {
    .vertices     = {v0, v1, v2},
    .text_coords  = {t0, t1, t2},
    .num_vertices = 3
  };

  /* apply frustrum clipping against all the frustrum planes */
  clip_polygon_against_plane(&polygon, FRUSTRUM_LEFT);
  clip_polygon_against_plane(&polygon, FRUSTRUM_RIGHT);
  clip_polygon_against_plane(&polygon, FRUSTRUM_BOTTOM);
  clip_polygon_against_plane(&polygon, FRUSTRUM_TOP);
  clip_polygon_against_plane(&polygon, FRUSTRUM_NEAR);
  clip_polygon_against_plane(&polygon, FRUSTRUM_FAR);

  /* break polygon apart back into triangles */
  int num_polygon_vertices = polygon.num_vertices;

  /* the number of triangles generated from a polygon is always the number of vertices - 2 */
  for(int i = 0; i < num_polygon_vertices - 2; i++){
    triangles_after_clipping[i].points[0] = (vector4_t){
      .x = polygon.vertices[0].x,
      .y = polygon.vertices[0].y,
      .z = polygon.vertices[0].z,
      .w = 1.0f
    };
    triangles_after_clipping[i].points[1] = (vector4_t){
      .x = polygon.vertices[i + 1].x,
      .y = polygon.vertices[i + 1].y,
      .z = polygon.vertices[i + 1].z,
      .w = 1.0f
    };
    triangles_after_clipping[i].points[2] = (vector4_t){
      .x = polygon.vertices[i + 2].x,
      .y = polygon.vertices[i + 2].y,
      .z = polygon.vertices[i + 2].z,
      .w = 1.0f
    };
    triangles_after_clipping[i].uv[0] = polygon.text_coords[0];
    triangles_after_clipping[i].uv[1] = polygon.text_coords[i + 1];
    triangles_after_clipping[i].uv[2] = polygon.text_coords[i + 2];
  }

  /* the number of triangles generated from a polygon is always the number of vertices - 2 */
  return num_polygon_vertices - 2;
}