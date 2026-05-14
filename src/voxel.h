#ifndef VOXEL_H
#define VOXEL_H

#include "aabb.h"
#include "material.h"
#include "mesh.h"
#include "light.h"

#define MAP_NUM_VOXELS_X 1024
#define MAP_NUM_VOXELS_Z 1024
#define MAP_NUM_VOXELS_Y 64
#define CHUNK_SIZE 16
#define NUM_CHUNK_DIVISIONS_X (MAP_NUM_VOXELS_X / CHUNK_SIZE)
#define NUM_CHUNK_DIVISIONS_Z (MAP_NUM_VOXELS_Z / CHUNK_SIZE)
#define CHUNK_RENDER_DISTANCE 6 // render this number of chunks around the player in XZ

typedef enum{
  VOXEL_TYPE_AIR   = 0,
  VOXEL_TYPE_STONE = 1,
  VOXEL_TYPE_DIRT  = 2,
  VOXEL_TYPE_GRASS = 3,
} voxel_type_t;

typedef enum{
  VOXEL_QUAD_TOP,
  VOXEL_QUAD_BOTTOM,
  VOXEL_QUAD_LEFT,
  VOXEL_QUAD_RIGHT,
  VOXEL_QUAD_FRONT,
  VOXEL_QUAD_BACK,
} voxel_quad_type_t;

typedef struct{
  char material_name[16];
  // color and light
  color_t base_color;
  color_t emissive_color;   //  object's own emission color */
  float opacity;            // used as a global alpha blending - makes object totally opaque or ghostly */
  bool reflects_light;
  bool dither;              // individual faces can be dithered
  // alpha
  blend_mode_t blend_mode;
  uint8_t alpha_cutoff;
  // texture data
  texture_t *side_texture;
  texture_t *top_texture;
} voxel_material_t;

typedef struct{
  voxel_quad_type_t quad_type;
  vector3_t points[4];
  uv_t uv[6];
  light_intensity_rgb_t total_light_intensity;
} voxel_quad_t;

typedef struct{
  voxel_type_t type;
  aabb_3d_t aabb;
  voxel_material_t *voxel_material;
  float avg_w;
} voxel_t;

typedef struct{
  int hit;
  int voxel_x;
  int voxel_y;
  int voxel_z;
  int normal_x;
  int normal_y;
  int normal_z;
  float distance;
} voxel_raycast_hit_t;

extern voxel_material_t voxel_materials[4];

// Y slowest changing dimension, X is fastest changing
extern voxel_t voxels[MAP_NUM_VOXELS_Y][MAP_NUM_VOXELS_Z][MAP_NUM_VOXELS_X];

void init_voxel_map();
void get_voxel_range_from_aabb(const aabb_3d_t *aabb, int *start_x, int *end_x, int *start_y, int *end_y, int *start_z, int *end_z);
voxel_quad_t generate_voxel_quad(voxel_quad_type_t quad_type, uint32_t y_plane, uint32_t z_row, uint32_t x_col);

#endif